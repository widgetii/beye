#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define VTMP_LEN 100

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

#define twrite(x)	write(out_fd, (x), strlen(x))

static int out_fd; 

// VT100 cursor repositioning
// https://vt100.net/docs/vt100-ug/chapter3.html#CUP
void vt100_SetCursorPos(int x, int y) {
    char vtmp[VTMP_LEN];

    sprintf(vtmp, "\033[%d;%dH", y + 1, x + 1);
    twrite(vtmp);
}

