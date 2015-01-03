#ifndef ARSC_COMMON_H
#define ARSC_COMMON_H
#include <unistd.h>

/*
 * map_file: mmap a file
 */
int map_file(const char *path, void **out_p, size_t *size);
void unmap_file(int fd, void *map, size_t size);

/*
 * die: abort program with minimal stack trace on stderr
 */
#define die(fmt, ...) \
	do { \
		__die(__FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); \
	} while (0)

void __die(const char *file, unsigned int line, const char *func,
	   const char *fmt, ...) \
	     __attribute__((__noreturn__, __format__(__printf__, 4, 5)));

#define die_if(cond, fmt, ...) \
	do { \
		if ((cond)) { \
			__die(__FILE__, __LINE__, __FUNCTION__, \
			      fmt, ##__VA_ARGS__); \
		} \
	} while (0)

/*
 * Functions to convert from device to host order.
 * Use dtohl for uint32_t, dtohs for uint16_t.
 */
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
# define dtohl(x) (x)
# define dtohs(x) (x)
#else
# include <arpa/inet.h>
# define dtohl(x) htonl(x)
# define dtohs(x) htons(x)
#endif

/*
 * xfunc: wrappers around standard functions.
 * Program execution is terminated on errors.
 */
static inline void *xmalloc(size_t size) {
	void *p = malloc(size);
	if (!p)
		die("malloc");
	return p;
}

static inline void *xcalloc(size_t n, size_t size) {
	void *p = calloc(n, size);
	if (!p)
		die("calloc");
	return p;
}

static inline void *xrealloc(void *p, size_t size) {
	void *q = realloc(p, size);
	if (!q)
		die("realloc");
	return q;
}

#endif
