#ifndef _UTILS_H
#define _UTILS_H

// deps
#include <libirecovery.h>

// std c library
#include <stdio.h>

irecv_client_t get_client();
int set_env(const char *key, const char *value);

#endif /* _UTILS_H */