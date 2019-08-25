#ifndef DEPRECATE_H
#define DEPRECATE_H

#ifdef HIDE_DEPRECATED
    #define DEPRECATE(x)
#else
    #define DEPRECATE(x) x
#endif

#endif
