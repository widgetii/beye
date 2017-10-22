#include <assert.h>
#include <string.h>
#include "libbeye/sysdep/generic/unix/console.h"

// TODO - remove all - use only libbeye.h
#include "bconfig.h"
#include <stdio.h>
#include <stdbool.h>
// TODO - remove all

static size_t consoleDriversNum = 0;

const ConsoleDriver drivers[] = {

#ifdef _SLANG_
    {.driverName = "SLang", .driver = NULL},
#endif

#ifdef _CURSES_
    {.driverName = "NCurses", .driver = NULL},
#endif

#ifdef _VT100_
    {.driverName = "VT100", .driver = NULL},
#endif

    // Anybody knows how to elegancy construct such structure?
    {.driverName = "dumb", .driver = NULL}

};


void consoleInitialize() {

   
    consoleDriversNum = sizeof(drivers) / sizeof(ConsoleDriver)
        // because dumb driver we need to dec num
        - 1;

    assert(sizeof(drivers) > 0);
    printf("We have %zu console drivers included\n", consoleDriversNum);

}

bool consoleDriversList(char* buf, size_t nSize) {
    size_t size = 0;

    for (size_t i = 0; i < consoleDriversNum; i++) {
        size_t strSize = strlen(drivers[i].driverName);
        if (size + strSize + 2 > nSize) { 
            // buffer too small for results
            return false;
        }
        strcpy(buf, drivers[i].driverName);
        buf += strSize + 1;
        size += strSize;
    }
    *buf = 0;
    
    return true;
}
