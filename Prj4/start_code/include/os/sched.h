/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking, 
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE. 
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_

#include "type.h"
#include "queue.h"
#include "lock.h"
#include "cond.h"
#include "barrier.h"
#include "sem.h"

#define NUM_MAX_TASK 16
#define MAX_LOCKS 20
#define KERNEL_STACK_TOP 0xa0f00000
#define USER_STACK_TOP KERNEL_STACK_TOP - 0x2000

typedef struct list{
    short page;
    short pte;
} mem_t;

/* used to save register infomation */
typedef struct regs_context
{
    /* Saved main processor registers.*/
    /* 32 * 4B = 128B */
    uint32_t regs[32];

    /* Saved special registers. */
    /* 7 * 4B = 28B */
    uint32_t cp0_status;
    uint32_t hi;
    uint32_t lo;
    uint32_t cp0_badvaddr;
    uint32_t cp0_cause;
    uint32_t cp0_epc;
    uint32_t pc;

} regs_context_t; /* 128 + 28 = 156B */

typedef enum {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_EXITED,
} task_status_t;

typedef enum {
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;

/* Process Control Block */
typedef struct pcb
{
    /* register context */
    regs_context_t kernel_context;
    regs_context_t user_context;
    
    uint32_t kernel_stack_top;
    uint32_t user_stack_top;

    /* previous, next pointer */
    void *prev;
    void *next;

    /* process id */
    pid_t pid;

    /* kernel/user thread/process */
    task_type_t type;

    /* BLOCK | READY | RUNNING */
    task_status_t status;
	
	/* process/thread priority */
	int priority;
	int bonus;
	
	/* sleep check information */
	int current_counter;
	int target_counter;

    /* cursor position */
    int cursor_x;
    int cursor_y;
    int line;
    
    /* current queue */
    queue_t* position;
    
    /* waiting queue */
    queue_t wait;
    
    /* lock info */
    mutex_lock_t* locks[MAX_LOCKS];
    
    /* sem info */
    semaphore_t* sems[MAX_LOCKS];
    
    /* cond info */
    condition_t* pcond;
    
    /* sem info */
    barrier_t* pbarrier;
    
    /* mem info */
    mem_t mems[MAX_LOCKS];

} pcb_t;

/* task information, used to init PCB */
typedef struct task_info
{
    char *name;
    uint32_t entry_point;
    task_type_t type;
    int priority;
} task_info_t;

/* ready queue to run */
extern queue_t ready_queue;

/* block queue to wait */
//extern queue_t block_queue;

/* block queue in sleep*/
extern queue_t sleep_queue;

/* current running task PCB */
extern pcb_t *current_running;
extern pcb_t *next_running;
extern pid_t process_id;

extern pcb_t pcb[NUM_MAX_TASK];
extern uint32_t initial_cp0_status;

void do_scheduler(void);
void do_sleep(uint32_t);

void do_block(queue_t *);
void do_unblock_one(queue_t *);
void do_unblock_all(queue_t *);

void do_spawn(task_info_t *, unsigned long*);
void do_kill(pid_t);
void do_exit();
void do_wait(pid_t);
pid_t do_getpid();

void do_ps();

extern unsigned long a[6];

#endif