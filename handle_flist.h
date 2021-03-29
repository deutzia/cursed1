#ifndef ZSO_HANDLE_FLIST
#define ZSO_HANDLE_FLIST

#include <stdbool.h>
#include <stdio.h>

typedef enum
{
    ZSO_none, /* no argument */
    ZSO_void,
    ZSO_int,   /* 32bit signed */
    ZSO_uint,  /* 32bit unsigned */
    ZSO_long,  /*32bit signed on 32bit side, 64bit signed on 64bit side */
    ZSO_ulong, /* 32bit unsigned on 32bit side, 64bit unsigned on 64bit side */
    ZSO_longlong,  /* 64bit signed */
    ZSO_ulonglong, /* 64bit unsigned */
    ZSO_ptr        /* pointer - 32bit on 32bit side, 64bit on 64bit side */
} ZSO_type;

/* last one will be empty, it's there to prevernt buffer overflow while parsing
 */
#define TLIST_MAX_SIZE 8

typedef struct
{
    ZSO_type t[TLIST_MAX_SIZE];
} tlist;

bool initialize_flist(FILE *);

// get type of function with given name or NULL if not found
tlist *lookup_flist(const char *);

void destruct_flist();

size_t get_flist_size();

#endif /* ZSO_HANDLE_FLIST */
