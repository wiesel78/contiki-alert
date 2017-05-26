#include <stdio.h>
#include <stdarg.h>

char* bcprintf(char *buf, int *remaining, const char *data, ...)
{
    va_list argptr;
    int len;
    int r = (*remaining);

    va_start(argptr, data);
    len = vsnprintf(buf, r, data, argptr);
    va_end(argptr);

    if(len < 0 || len >= r){
        printf("Buffer to short. Have %d, need %d + \\0\n", r, len);
        return NULL;
    }

    r -= len;
    (*remaining) = r;
    return buf + len;
}
