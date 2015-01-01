#ifndef ARSC_BLOB_H
#define ARSC_BLOB_H
#include <unistd.h>

struct blob;

void blob_init(struct blob **blob, const void *map, size_t size);
void blob_dump(const struct blob *blob);

#endif
