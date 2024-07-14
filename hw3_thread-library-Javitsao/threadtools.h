#ifndef THREADTOOL
#define THREADTOOL
#include<setjmp.h>
#include<poll.h>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<fcntl.h>
#include<sys/signal.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/fcntl.h>
#include<errno.h>
#define THREAD_MAX 16  // maximum number of threads created
#define BUF_SIZE 512
struct tcb {
    int id;  // the thread id
    jmp_buf environment;  // where the scheduler should jump to
    int arg;  // argument to the function
    int fd;  // file descriptor for the thread
    char buf[BUF_SIZE];  // buffer for the thread
    int i, x, y;  // declare the variables you wish to keep between switches
};

extern int timeslice;
extern jmp_buf sched_buf;
extern struct tcb *ready_queue[THREAD_MAX], *waiting_queue[THREAD_MAX];
/*
 * rq_size: size of the ready queue
 * rq_current: current thread in the ready queue
 * wq_size: size of the waiting queue
 */
extern int rq_size, rq_current, wq_size;
/*
* base_mask: blocks both SIGTSTP and SIGALRM
* tstp_mask: blocks only SIGTSTP
* alrm_mask: blocks only SIGALRM
*/
extern sigset_t base_mask, tstp_mask, alrm_mask;
/*
 * Use this to access the running thread.
 */

#define RUNNING (ready_queue[rq_current])

void sighandler(int signo);
void scheduler();

#define thread_create(func, id, arg) {\
    func(id, arg);\
}
// fd_set[rq_size].fd = fifo_fd;
// fd_set[rq_size].events = POLLIN;
#define thread_setup(id, arg) {\
    ready_queue[rq_size] = (struct tcb*)malloc(sizeof(struct tcb));\
    ready_queue[rq_size]->arg = arg;\
    ready_queue[rq_size]->id = id;\
    char fifo_name[100] = {};\
    sprintf(fifo_name, "%d_%s", id, __func__);\
    mkfifo(fifo_name, 0755);\
    int fifo_fd = open(fifo_name, O_RDONLY | O_NONBLOCK);\
    ready_queue[rq_size]->fd = fifo_fd;\
    if(sigsetjmp(ready_queue[rq_size]->environment, 1) == 0){\
        rq_size++;\
        return;\
    }\
}

#define thread_exit() {\
    char fifo_name[100] = {};\
    sprintf(fifo_name, "%d_%s", ready_queue[rq_current]->id, __func__);\
    unlink(fifo_name);\
    siglongjmp(sched_buf, 3);\
}

#define thread_yield() {\
    if(sigsetjmp(RUNNING->environment, 1) == 0){\
        sigprocmask(SIG_SETMASK, &alrm_mask, NULL);\
        sigprocmask(SIG_SETMASK, &tstp_mask, NULL);\
        sigprocmask(SIG_SETMASK, &base_mask, NULL);\
    }\
}

#define async_read(count){\
    if(sigsetjmp(ready_queue[rq_current]->environment, 2) == 0){\
        siglongjmp(sched_buf, 2);\
    }\
    read(ready_queue[rq_current]->fd, ready_queue[rq_current]->buf, count);\
}

#endif // THREADTOOL
