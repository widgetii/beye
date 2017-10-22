#ifndef __CON_VT100_H_
#define __CON_VT100_H_

#include <unistd.h>

ssize_t vt100_twrite(const char* ptr);

void vt100_SetCursorPos(int x, int y);
void vt100_initialize();
void vt100_terminate();

#endif
