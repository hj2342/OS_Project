#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s N\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "N must be positive\n");
        return 1;
    }

    for (int i = 1; i <= n; i++) {
        printf("Demo %d/%d\n", i, n);
        fflush(stdout);  // Ensure output is displayed immediately
        sleep(1);       // Sleep for 1 second between iterations
    }
    return 0;
}