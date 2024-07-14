#include "threadtools.h"

/*
 * Print out the signal you received.
 * If SIGALRM is received, reset the alarm here.
 * This function should not return. Instead, call longjmp(sched_buf, 1).
 */
void sighandler(int signo) {
    if(signo == SIGALRM){
        printf("caught SIGALRM\n");
        alarm(timeslice);
    }
    else if(signo == SIGTSTP){
        printf("caught SIGTSTP\n");
    }
    siglongjmp(sched_buf, 1);
    
}

/*
 * Prior to calling this function, both SIGTSTP and SIGALRM should be blocked.
 */
//struct pollfd fd_set[THREAD_MAX];
void move_wait_to_ready() {
    struct pollfd fd_set[THREAD_MAX];
    for(int i = 0; i < wq_size; i++){
        fd_set[i].fd = waiting_queue[i]->fd;
        fd_set[i].events = POLLIN;
    }
    poll(fd_set, sizeof(fd_set)/sizeof(fd_set[0]), 0);
    int count_hole_in_waiting = 0;
    for(int i = 0; i < wq_size; i++){
        if(fd_set[i].revents & POLLIN){
            ready_queue[rq_size] = waiting_queue[i];
            waiting_queue[i] = NULL;
            rq_size++; count_hole_in_waiting++;
        }
    }
    for(int i = 0; i < wq_size; i++){
        int cur = i;
        if(waiting_queue[i] == NULL){
            while(waiting_queue[cur] == NULL && cur < wq_size){
                cur++;
            }
            if(cur < wq_size){
                waiting_queue[i] = waiting_queue[cur];
                waiting_queue[cur] = NULL;
            }
        }
    }
    wq_size -= count_hole_in_waiting;
}

void scheduler() {
    switch(sigsetjmp(sched_buf, 1)){
        case 1:
            move_wait_to_ready();
            rq_current++;
            //printf("rq_cur = %d\n", rq_current);
            if(rq_current == rq_size) rq_current = 0;
            while(!rq_size && wq_size > 0){
                move_wait_to_ready();
            }
            siglongjmp(ready_queue[rq_current]->environment, 1);
            break;
        case 2:
            move_wait_to_ready();
            waiting_queue[wq_size++] = RUNNING;
            rq_size--;
            if(rq_size == 0){
                while(!rq_size){
                    move_wait_to_ready();
                }
            }
            if(rq_current == rq_size){
                rq_current = 0;
            }
            else{
                RUNNING = ready_queue[rq_size];
            }
            siglongjmp(ready_queue[rq_current]->environment, 1);
            break;
        case 3:
            move_wait_to_ready();
            free(ready_queue[rq_current]);
            rq_size--;
            if(rq_size == 0 && wq_size == 0) return;
            while(!rq_size){
                move_wait_to_ready();
            }
            if(rq_current == rq_size){
                rq_current = 0;
            }
            else{
                ready_queue[rq_current] = ready_queue[rq_size];
            }
            siglongjmp(ready_queue[rq_current]->environment, 1);
            break;
        case 0:
            siglongjmp(ready_queue[rq_current]->environment, 1);
            break;
    }

}
