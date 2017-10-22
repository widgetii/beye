#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif

const int VTMP_LEN=100;

#define twrite(x)	write(out_fd, (x), strlen(x))

// static inline twrite?

static int out_fd; 

// VT100 cursor repositioning
// https://vt100.net/docs/vt100-ug/chapter3.html#CUP
void vt100_SetCursorPos(int x, int y) {
    char vtmp[VTMP_LEN];

    sprintf(vtmp, "\x1b[%d;%dH", y + 1, x + 1);
    twrite(vtmp);
}

static void vt100_clearscreen() {
    // clear screen on exit
    twrite("\x1b[2J");
    // set cursor's position to top-left corner
    twrite("\x1b[H");
}

static int vt100_getWindowSize(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(out_fd, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void vt100_initialize() {
    out_fd = open(ttyname(STDOUT_FILENO), O_WRONLY);
    if (out_fd < 0) { 
        out_fd = STDOUT_FILENO;
    }

    vt100_clearscreen();
}

void vt100_terminate() {
    vt100_clearscreen();

    close(out_fd);
}

