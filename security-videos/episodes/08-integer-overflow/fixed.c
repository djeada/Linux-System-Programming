#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    unsigned long input = strtoul(argv[1], NULL, 10);
    if (input == 0 || input > 4096) {
        fputs("rejected: record count out of range\n", stderr);
        return 1;
    }
    size_t count = (size_t)input;
    if (count > SIZE_MAX / 16) return 1;
    size_t bytes = count * 16;
    char *records = malloc(bytes);
    if (!records) return 1;
    memset(records, 'X', bytes);
    printf("allocated=%zu\n", bytes);
    free(records);
    return 0;
}
