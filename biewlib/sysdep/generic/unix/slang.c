#include <slang.h>

void slang_SetCursorPos(int x, int y) {
    SLsmg_gotorc(y, x);
}
