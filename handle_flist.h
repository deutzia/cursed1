#ifndef ZSO_HANDLE_FLIST
#define ZSO_HANDLE_FLIST

#include <stdbool.h>
#include <stdio.h>

bool initialize_flist(FILE *);

// look up a symbol in flist
bool lookup_flist(const char *);

void destruct_flist();

size_t get_flist_size();

#endif /* ZSO_HANDLE_FLIST */
