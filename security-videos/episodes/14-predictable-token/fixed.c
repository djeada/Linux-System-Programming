#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/random.h>

int main(void) {
    uint32_t token = 0;
    unsigned char *out = (unsigned char *)&token;
    size_t done = 0;
    while (done < sizeof token) {
        ssize_t n = getrandom(out + done, sizeof token - done, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { perror("getrandom"); return 1; }
        done += (size_t)n;
    }
    printf("token=%08x\n", token);
    return 0;
}
