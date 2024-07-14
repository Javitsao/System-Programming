#include "threadtools.h"

void fibonacci(int id, int arg) {
    thread_setup(id, arg);
    //printf("fib\n");
    for (RUNNING->i = 1; ; RUNNING->i++) {
        if (RUNNING->i <= 2)
            RUNNING->x = RUNNING->y = 1;
        else {
            /* We don't need to save tmp, so it's safe to declare it here. */
            int tmp = RUNNING->y;  
            RUNNING->y = RUNNING->x + RUNNING->y;
            RUNNING->x = tmp;
        }
        printf("%d %d\n", RUNNING->id, RUNNING->y);
        sleep(1);

        if (RUNNING->i == RUNNING->arg) {
            thread_exit();
        }
        else {
            thread_yield();
            //printf("in fibonacci\n");
        }
    }
}

void collatz(int id, int arg) {
    thread_setup(id, arg);
    //printf("coll\n");
    RUNNING->i = RUNNING->arg;
    while (1) {
        if(RUNNING->i > 1){
            if(RUNNING->i % 2 == 0){
                RUNNING->i /= 2;
            }
            else if(RUNNING->i % 2 == 1){
                RUNNING->i = RUNNING->i * 3 + 1;
            }
            printf("%d %d\n", RUNNING->id, RUNNING->i);
            sleep(1);
            //printf("in collatz\n");
        }
        if(RUNNING->i > 1){
            thread_yield();
        }
        else{
           thread_exit();
        }
    }
}

void max_subarray(int id, int arg) {
    thread_setup(id, arg);
    //printf("max\n");
    RUNNING->x = 0;
    RUNNING->y = 0;
    for (RUNNING->i = 1; ; RUNNING->i++) {
        async_read(5);
        //printf("aaaaaa\n");
        int num = atoi(RUNNING->buf);
        RUNNING->x = (num > RUNNING->x + num)? num: RUNNING->x + num;
        if(RUNNING->x > RUNNING->y) RUNNING->y = RUNNING->x;
        printf("%d %d\n", RUNNING->id, RUNNING->y);
        sleep(1);
        if(RUNNING->i == RUNNING->arg){
            thread_exit();
        }
        else{
            //printf("in max_sub\n");
            thread_yield();
        }
    }
}
