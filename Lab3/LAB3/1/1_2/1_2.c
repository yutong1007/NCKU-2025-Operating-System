#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/*Note: Value of LOCK is 0 and value of UNLOCK is 1.*/
#define LOCK 0
#define UNLOCK 1

volatile int a = 0;
volatile int lock = UNLOCK;
pthread_mutex_t mutex;

// arm64 assembly code present spinlock
void spin_lock() {
    asm volatile(
        "wait:\n\t"
        "ldaxr w1, [%[addr]]\n\t"        // read lock to w1 (register)
        "cmp   w1, #1\n\t"               // check lock is/not 1
        "b.ne  wait\n\t"                 // if locked (w1 != 1), back to wait
        "mov   w1, #0\n\t"               // if unlock, lock it (w1 change to 0)
        "stxr  w2, w1, [%[addr]]\n\t"    // store result to w2, if w2 == 0 (get spinlock)
        "cbnz  w2, wait\n\t"             // if w2 == 1, fail to get spinlock, try again
        :
        : [addr] "r" (&lock)
        : "w1", "w2", "memory", "cc"
    );
}

// ldaxr/stxr make sure only one can write into

void spin_unlock() {
    asm volatile(
        "mov  w1, #1\n\t"                // set lock (w1) to 1 (unlock) 
        "stlr w1, [%[addr]]\n\t"         // release store, critical section complete
        :
        : [addr] "r" (&lock)
        : "w1", "memory"
    );
}


void *thread(void *arg) {

    for(int i=0; i<10000; i++){

        spin_lock();
        a = a + 1;
        spin_unlock();
    }
    return NULL;
}

int main() {
    FILE *fptr;
    fptr = fopen("1.txt", "a");
    pthread_t t1, t2;

    pthread_mutex_init(&mutex, 0);
    pthread_create(&t1, NULL, thread, NULL);
    pthread_create(&t2, NULL, thread, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_mutex_destroy(&mutex);

    fprintf(fptr, "%d ", a);
    fclose(fptr);
}

