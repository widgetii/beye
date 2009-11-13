/*
  this file was created for testing purposes only
*/
#include <stdio.h>

extern long var;

char * string = "This is multline string
was typed for testing
purposes only", ch;
const char *multiline = "This is another multiline string
was type for testing escapes: \" characters and other
escapes like this: \' and so
on";
void func(void) {
// C++ commentaries are allowed by modern C compilers
    wchar_t single_byte='\'';
/* we should avoid to use reserved C++ keyword: */
    int* public;
    char *protected;
    long __restricted__ *virtual;
/* GCC extensions of C: */
    __extension__ {  int a = typeof b; }
    fprintf(stderr,"string to be print\n",var);
    fprintf(stdout,"we should accurately distinguish \" characters withing of comments\n");
    printf("we also should distinguish \\ characters withing of comments\n");
    return (2+2)*4+-~var;
}