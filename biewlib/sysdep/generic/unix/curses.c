#include <curses.h>

void curses_SetCursorPos(int x, int y) {
    move(y, x);
}
