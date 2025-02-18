#ifndef THREADING_H
#define THREADING_H

#include <stdio.h>
#include <pthread.h>
#include "params.h"

typedef struct ThreadArgs {
    size_t start;
    void (*f)(size_t);
} ThreadArgs;

void* thread_worker(void* arg) {
    ThreadArgs args = *(ThreadArgs*)arg;
    for (size_t i = 0; i < particles_per_thread; i++) {
        (*args.f)(args.start + i);
    }
    pthread_exit(NULL);
    return NULL;
}

void thread_master(void (*f)(size_t)) {
    //for (size_t i = 0; i < particle_count; i++) {
    //    (*f)(i);
    //}
    pthread_t threads[thread_count];
    ThreadArgs args[thread_count];
    for (size_t i = 0; i < thread_count; i++) {
        args[i].start = i*particles_per_thread;
        args[i].f = f;
        if (pthread_create(threads+i, NULL, thread_worker, args+i) != 0) {
            perror("Failed to create thread");
        }
    }
    for (size_t i = 0; i < thread_count; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
        }
    }
}

#endif // !THREADING_H
#define THREADING_H
