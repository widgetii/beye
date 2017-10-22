#include <stdio.h>
#include <string.h>
#include "libbeye/sysdep/generic/unix/console.h"

void enumConsoleDriverList() {
    char buf[10000];

    consoleDriversList(buf, sizeof(buf));

    char *ptr = buf;
    while (*ptr != 0) {
        printf("%s\n", ptr);
        ptr += strlen(ptr) + 1;
    }
}

int main() {
    consoleInitialize();

    enumConsoleDriverList();
}

