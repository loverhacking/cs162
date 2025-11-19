#include <stdio.h>
#include <sys/resource.h>

int main() {
    struct rlimit lim;
    if (getrlimit(RLIMIT_STACK, &lim) == 0) {
        printf("stack size: %ld\n", (long)lim.rlim_cur);
    } else {
        printf("stack size: unknown\n");
    }

    if (getrlimit(RLIMIT_NPROC, &lim) == 0) {
        printf("process limit: %ld\n", (long)lim.rlim_cur);
    } else {
        printf("process limit: unknown\n");
    }

    if (getrlimit(RLIMIT_NOFILE, &lim) == 0) {
        printf("max file descriptors: %ld\n", (long)lim.rlim_cur);
    } else {
        printf("max file descriptors: unknown\n");
    }
    return 0;
}
