#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/wait.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("Pet elevator scheduling kernel module");
MODULE_VERSION("1.0");

#define ENTRY_NAME   "elevator"
#define PERMS        0444

#define MIN_FLOOR    1
#define MAX_FLOOR    5
#define MAX_PETS     5
#define MAX_WEIGHT   50
#define NUM_TYPES    4
#define MOVE_MS      2000
#define LOAD_MS      1000
#define SAFETY_MS    10

extern long (*start_elevator_ptr)(void);
extern long (*issue_request_ptr)(int, int, int);
extern long (*stop_elevator_ptr)(void);

enum elevator_state { OFFLINE, IDLE, LOADING, UP, DOWN };
enum step_action { ACT_NONE, ACT_LOAD, ACT_MOVE };

static const char * const state_name[] = { "OFFLINE", "IDLE", "LOADING", "UP", "DOWN" };

/* Pet type: 0 = Chihuahua, 1 = Pug, 2 = Pughuahua, 3 = Dachshund */
static const int pet_weight[NUM_TYPES] = { 3, 14, 10, 16 };
static const char pet_letter[NUM_TYPES] = { 'C', 'P', 'H', 'D' };

struct pet {
	int type;
	int dest;
	struct list_head node;
};

static struct {
	enum elevator_state state;
	int floor;
	int direction;		/* +1 = up, -1 = down */
	int load;		/* total weight on board, in lbs */
	int count;		/* pets on board */
	int waiting;		/* pets waiting on all floors */
	int serviced;		/* pets delivered */
	bool stopping;		/* stop requested, draining riders */
	struct list_head riders;
	struct list_head queue[MAX_FLOOR + 1];	/* index 0 unused */
} elev;

static DEFINE_MUTEX(elev_lock);
static DECLARE_WAIT_QUEUE_HEAD(elev_wq);
static struct task_struct *elev_thread;
static struct proc_dir_entry *proc_entry;

static bool pet_fits(const struct pet *p)
{
	return elev.count < MAX_PETS && elev.load + pet_weight[p->type] <= MAX_WEIGHT;
}

static bool floor_wants_stop(int f)
{
	struct pet *p;

	list_for_each_entry(p, &elev.riders, node)
		if (p->dest == f)
			return true;

	if (!elev.stopping && !list_empty(&elev.queue[f])) {
		p = list_first_entry(&elev.queue[f], struct pet, node);
		if (pet_fits(p))
			return true;
	}
	return false;
}

static bool target_ahead(int dir)
{
	int f;

	for (f = elev.floor + dir; f >= MIN_FLOOR && f <= MAX_FLOOR; f += dir)
		if (floor_wants_stop(f))
			return true;
	return false;
}

static void unload_pets(void)
{
	struct pet *p, *tmp;

	list_for_each_entry_safe(p, tmp, &elev.riders, node) {
		if (p->dest != elev.floor)
			continue;
		list_del(&p->node);
		elev.load -= pet_weight[p->type];
		elev.count--;
		elev.serviced++;
		kfree(p);
	}
}

static void load_pets(void)
{
	struct list_head *q = &elev.queue[elev.floor];
	struct pet *p;

	if (elev.stopping)
		return;

	while (!list_empty(q)) {
		p = list_first_entry(q, struct pet, node);
		if (!pet_fits(p))
			break;
		list_move_tail(&p->node, &elev.riders);
		elev.load += pet_weight[p->type];
		elev.count++;
		elev.waiting--;
	}
}

static void free_all_pets(void)
{
	struct pet *p, *tmp;
	int f;

	list_for_each_entry_safe(p, tmp, &elev.riders, node) {
		list_del(&p->node);
		kfree(p);
	}
	for (f = MIN_FLOOR; f <= MAX_FLOOR; f++) {
		list_for_each_entry_safe(p, tmp, &elev.queue[f], node) {
			list_del(&p->node);
			kfree(p);
		}
	}
	elev.load = 0;
	elev.count = 0;
	elev.waiting = 0;
}

static bool elevator_has_work(void)
{
	return READ_ONCE(elev.state) != OFFLINE &&
	       (READ_ONCE(elev.waiting) > 0 || READ_ONCE(elev.count) > 0 ||
		READ_ONCE(elev.stopping) || READ_ONCE(elev.state) == LOADING);
}

static enum step_action elevator_step(void)
{
	if (elev.stopping && elev.count == 0) {
		elev.state = OFFLINE;
		elev.stopping = false;
		return ACT_NONE;
	}

	if (floor_wants_stop(elev.floor)) {
		elev.state = LOADING;
		unload_pets();
		load_pets();
		return ACT_LOAD;
	}

	if (!target_ahead(elev.direction)) {
		elev.direction = -elev.direction;
		if (!target_ahead(elev.direction)) {
			elev.state = IDLE;
			return ACT_NONE;
		}
	}

	elev.state = (elev.direction > 0) ? UP : DOWN;
	return ACT_MOVE;
}

static int elevator_thread_fn(void *data)
{
	enum step_action action;

	while (!kthread_should_stop()) {
		wait_event_interruptible(elev_wq,
					 kthread_should_stop() || elevator_has_work());
		if (kthread_should_stop())
			break;

		mutex_lock(&elev_lock);
		action = elevator_step();
		mutex_unlock(&elev_lock);

		switch (action) {
		case ACT_LOAD:
			msleep(LOAD_MS);
			break;
		case ACT_MOVE:
			msleep(MOVE_MS);
			mutex_lock(&elev_lock);
			elev.floor += elev.direction;
			mutex_unlock(&elev_lock);
			break;
		default:
			msleep(SAFETY_MS);	/* never spin if no progress was made */
			break;
		}
	}
	return 0;
}

static long elevator_start(void)
{
	long ret = 0;

	mutex_lock(&elev_lock);
	if (elev.state != OFFLINE) {
		ret = 1;		/* already active (or deactivating) */
	} else {
		elev.state = IDLE;
		elev.floor = MIN_FLOOR;
		elev.direction = 1;
		elev.stopping = false;
	}
	mutex_unlock(&elev_lock);

	wake_up(&elev_wq);
	return ret;
}

static long elevator_issue_request(int start, int dest, int type)
{
	struct pet *p;

	if (start < MIN_FLOOR || start > MAX_FLOOR ||
	    dest < MIN_FLOOR || dest > MAX_FLOOR ||
	    start == dest || type < 0 || type >= NUM_TYPES)
		return 1;

	p = kmalloc(sizeof(*p), GFP_KERNEL);
	if (!p)
		return -ENOMEM;
	p->type = type;
	p->dest = dest;

	mutex_lock(&elev_lock);
	list_add_tail(&p->node, &elev.queue[start]);
	elev.waiting++;
	mutex_unlock(&elev_lock);

	wake_up(&elev_wq);
	return 0;
}

static long elevator_stop(void)
{
	long ret = 0;

	mutex_lock(&elev_lock);
	if (elev.stopping)
		ret = 1;		/* already deactivating */
	else if (elev.state != OFFLINE)
		elev.stopping = true;
	mutex_unlock(&elev_lock);

	wake_up(&elev_wq);
	return ret;
}

static int elevator_proc_show(struct seq_file *m, void *v)
{
	struct pet *p;
	int f;

	mutex_lock(&elev_lock);

	seq_printf(m, "Elevator state: %s\n", state_name[elev.state]);
	seq_printf(m, "Current floor: %d\n", elev.floor);
	seq_printf(m, "Current load: %d lbs\n", elev.load);

	seq_puts(m, "Elevator status:");
	list_for_each_entry(p, &elev.riders, node)
		seq_printf(m, " %c%d", pet_letter[p->type], p->dest);
	seq_putc(m, '\n');

	for (f = MAX_FLOOR; f >= MIN_FLOOR; f--) {
		int n = 0;

		list_for_each_entry(p, &elev.queue[f], node)
			n++;
		seq_printf(m, "[%c] Floor %d: %d", (f == elev.floor) ? '*' : ' ', f, n);
		list_for_each_entry(p, &elev.queue[f], node)
			seq_printf(m, " %c%d", pet_letter[p->type], p->dest);
		seq_putc(m, '\n');
	}

	seq_printf(m, "Number of pets: %d\n", elev.count);
	seq_printf(m, "Number of pets waiting: %d\n", elev.waiting);
	seq_printf(m, "Number of pets serviced: %d\n", elev.serviced);

	mutex_unlock(&elev_lock);
	return 0;
}

static int elevator_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, elevator_proc_show, NULL);
}

static const struct proc_ops elevator_proc_ops = {
	.proc_open	= elevator_proc_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static int __init elevator_init(void)
{
	int f;

	elev.state = OFFLINE;
	elev.floor = MIN_FLOOR;
	elev.direction = 1;
	INIT_LIST_HEAD(&elev.riders);
	for (f = MIN_FLOOR; f <= MAX_FLOOR; f++)
		INIT_LIST_HEAD(&elev.queue[f]);

	proc_entry = proc_create(ENTRY_NAME, PERMS, NULL, &elevator_proc_ops);
	if (!proc_entry)
		return -ENOMEM;

	elev_thread = kthread_run(elevator_thread_fn, NULL, "kelevator");
	if (IS_ERR(elev_thread)) {
		int err = PTR_ERR(elev_thread);
		proc_remove(proc_entry);
		return err;
	}

	start_elevator_ptr = elevator_start;
	issue_request_ptr = elevator_issue_request;
	stop_elevator_ptr = elevator_stop;
	return 0;
}

static void __exit elevator_exit(void)
{
	start_elevator_ptr = NULL;
	issue_request_ptr = NULL;
	stop_elevator_ptr = NULL;

	kthread_stop(elev_thread);
	proc_remove(proc_entry);

	mutex_lock(&elev_lock);
	free_all_pets();
	mutex_unlock(&elev_lock);
}

module_init(elevator_init);
module_exit(elevator_exit);
