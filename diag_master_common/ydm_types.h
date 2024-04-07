#ifndef __YOM_TYPES_H__
#define __YOM_TYPES_H__

typedef unsigned int yuint32;
typedef unsigned short yuint16;
typedef unsigned char yuint8;

typedef signed int yint32;
typedef signed short yint16;
typedef signed char yint8;

typedef unsigned char BOOLEAN;
#ifdef _WIN32
#ifndef F_OK
#define F_OK (0)
#endif /* #ifndef F_OK */
typedef int ssize_t;
#endif /* _WIN32 */
#ifndef true
#define true (1)
#endif /* #ifndef true */
#ifndef false
#define false (!true)
#endif /* #ifndef false */ 

#ifndef NULL
#define NULL ((void *)0)
#endif /* NULL */

#ifndef MIN
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#endif

#ifdef _WIN32

#else /* _WIN32 */
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t)(&((TYPE *)0)->MEMBER))
#endif
#ifndef container_of
#define container_of(ptr, type, member) ({ \
            const typeof(*ptr) *__ptr = (ptr); \
            (type *) ((char*)__ptr - offsetof(type, member));})
#endif
#endif /* _WIN32 */

#define Y_UNUSED(v) (void)v

#define ymalloc(s) malloc(s)
#define yfree(p) free(p)
#define ycalloc(s, c) calloc(s, c)

#define VAR_RANGE_LIMIT(var, min, max) \
    do {\
        if (min > max) { \
            (var) = (var) < (max) ? (max) : (var); \
            (var) = (var) > (min) ? (min) : (var); \
        } \
        else { \
            (var) = (var) < (min) ? (min) : (var); \
            (var) = (var) > (max) ? (max) : (var); \
        } \
    } while (0)

#endif /* __YOM_TYPES_H__ */
