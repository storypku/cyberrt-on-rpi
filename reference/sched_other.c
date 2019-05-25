/**
 * nice(1)/renice(1)/setpriority(2)/getpriority(2)
 * Build:
 *  gcc sched_other.c -O2 -pthread -o sched
 * Run:
 *  ./sched 5 &
 *  ps -fl
 *  nice -n 3 ./sched 5 &
 *  ps -fl
 */
#include <sched.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/resource.h>

#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)


static void display_sched_attr(int policy, struct sched_param* param) {
    printf("    policy=%s, priority=%d\n",
           (policy == SCHED_FIFO)  ? "SCHED_FIFO" :
           (policy == SCHED_RR)    ? "SCHED_RR" :
           (policy == SCHED_OTHER) ? "SCHED_OTHER" :
           "???",
           param->sched_priority);
}

static void
display_thread_sched_attr(char* msg) {
    int policy, s;
    struct sched_param param;

    s = pthread_getschedparam(pthread_self(), &policy, &param);

    if (s != 0) {
        handle_error_en(s, "pthread_getschedparam");
    }

    printf("%s\n", msg);
    display_sched_attr(policy, &param);
}


int main(int argc, char *argv[]) {
    int prio = 0;
    if (argc >= 2) {
        prio = atoi(argv[1]);
    }
    int tid = syscall(SYS_gettid);
    int s = setpriority(PRIO_PROCESS, tid, prio);
    // int s = setpriority(PRIO_PROCESS, 0, prio);
    if (s != 0) {
        handle_error_en(errno, "setpriority");
    }

    display_thread_sched_attr("main");
    sleep(30); // 30s
    return 0;
}
