#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

extern char _end;
extern char __heap_end;

caddr_t _sbrk(int incr)
{
    static char *heap_end;
    char *previous;
    if (heap_end == NULL) heap_end = &_end;
    previous = heap_end;
    if ((incr < 0) || (heap_end + incr > &__heap_end)) {
        errno = ENOMEM;
        return (caddr_t)-1;
    }
    heap_end += incr;
    return (caddr_t)previous;
}
