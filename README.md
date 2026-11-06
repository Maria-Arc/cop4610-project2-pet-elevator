# Elevator

We modified the linux kernel and built a pet elevator within it. 
COP4610 Project 2: Pet Elevator

## Group Members
- **Rebecca Gorman**: rkg23a@fsu.edu
- **Maria Arcis Pineda**: mea23a@fsu.edu
- **Nicholas Brion**: npb22c@fsu.edu
## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**: Trace and analyze sysem calls
- **Assigned to**: Maria Arcis Pineda

### Part 2: Timer Kernel Module
- **Responsibilities**: Implementation of the timer kernel module
- **Assigned to**: Maria Arcis Pineda, Rebecca Gorman

### Part 3a: Adding System Calls
- **Responsibilities**: Added the elevator system calls to the kernel
- **Assigned to**: Nicolas Brion, Maria Arcis Pineda

### Part 3b: Kernel Compilation
- **Responsibilities**: compiled the modified kernel
- **Assigned to**: Maria Arcis Pineda

### Part 3c: Threads
- **Responsibilities**: implemented kernel threads for elevator operations
- **Assigned to**: Nicolas Brion

### Part 3d: Linked List
- **Responsibilities**: implemented linked lists to manage elevator requests
- **Assigned to**: Nicolas Brion, Rebecca Gorman

### Part 3e: Mutexes
- **Responsibilities**: implemented mutexes to synchronize shared data
- **Assigned to**: Nicolas Brion

### Part 3f: Scheduling Algorithm
- **Responsibilities**: Implemented and debugged the elevator scheduling algorithm 
- **Assigned to**: Nicolas Brion, Rebecca Gorman

### Testing and Documentation 
- **Responsibilities**: Tested all functionality and fixed bugs. Completed project documentation. 
- **Assigned to**: Rebecca Gorman

## File Listing
```
elevator/
├── part1/
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   ├── part1.trace
├── part2/
│   ├── my_timer.c
│   └── Makefile
├── part3/
│   ├── src/
|   |      ├──elevator.c
|   |      └── Makefile
│   ├── tests/
|   |      ├── elevator-test
|   |      └──system-calls-test
│   ├── Makefile
│   └── sys_call.c
├── .gitignore
└── README.md

```
# How to Compile & Execute

### Requirements
- **Compiler**: GCC
- **Dependencies**: stdio

## Part 1

### Compilation
gcc -o empty empty.c
gcc -o part1 part1.c 

### Execution
strace -o empty.trace ./empty 
strace -o part1.trace ./part1

## Part 2

### Compilation
make
sudo insmod my_timer.ko

### Execution
cat /proc/timer


## Part 3

### Compilation
make
sudo insmod elevator.ko

### Execution
cat /proc/elevator

## Development Log

### [Rebecca Gorman]

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-11-01 | Set up elevator system calls and initialized repo |
| 2026-11-02 | worked on part 2 and organied the project folder structure |
| 2026-05-2026 | testing and documentation |

### [Maria Arcis Pineda]

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-11-01 |Set up elevator system calls and initialized repo  |
| 2026-11-02 | worked on part 2 and organied the project folder structure   |
| 2026-11-05 | merged part 2 and 3 merge  |


### [Nicolas Brion]

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-11-04 | updated elevator.c and contributed to part 3 implementation |
| 2026-11-05 | part 3 contributions were merged into the main branch |



## Meetings

| Date       | Attendees            | Topics Discussed | Outcomes / Decisions |
|------------|----------------------|------------------|-----------------------|
| 2026-10-27 | Rebecca Maria Nico            | talk about division of labor and when we will be using the vm to not override anyone   | rebecca and maria to do parts 1 and 2  |
| 2026-11-02 | Rebecca Maria Nico| status updates and discussion on final steps  | nico finishing implementation and rebecca testing with maria |



## Considerations
Rebecca and Maria pairprogrammed for parts 1 and 2. We tested the elevator for 5, 10, 100, and 10000. 

Each of us used 3 days, final due date is 11/5 @ 11:59 p.m.