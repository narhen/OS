#ifndef __KPANIC_H
#define __KPANIC_H

extern void _kpanic(const char *file, const char *function, 
        int line, const char *msg);
#define kpanic(msg) _kpanic(__FILE__, __FUNCTION__, __LINE__, msg)

#endif
