#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    unsigned char packet[4] = {0, 'c', 'a', 't'};
    packet[0] = (unsigned char)strtoul(argv[1], NULL, 10);
    char output[256];
    size_t claimed = packet[0];
    size_t available = sizeof packet - 1;
    if (claimed > available || claimed > sizeof output) {
        fputs("rejected: truncated packet\n", stderr);
        return 1;
    }
    memcpy(output, packet + 1, claimed);
    printf("copied=%zu\n", claimed);
    return 0;
}
