#ifndef ERR_H
#define ERR_H

#ifdef __cplusplus

extern "C" {
void syserr(const char *fmt, ...);
void fatal(const char *fmt, ...);
};

#else

extern void syserr(const char *fmt, ...);
extern void fatal(const char *fmt, ...);

#endif


#endif /* ERR_H */
