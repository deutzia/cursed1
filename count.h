#ifndef ZSO_COUNT
#define ZSO_COUNT

#include <elf.h>
#include <stdlib.h>

/* if returned value is not zero then it errored */
int count_things(
    void *elf,
    int *sections_to_copy, /* number of sections to copy wo touching them */
    off_t *size_total,      /* size of stuff before section headers */
    off_t *strtab_len,      /* length of STRTAB entries that get copied */
    int *symbols,          /* count symbols */
    int *first_nonlocal_symbol, /* used for st_info in symtab header */
    int **sections_reorder,     /* mapping of old section numbers to new ones */
    off_t *
        *sections_offsets, /* mapping of old section numbers to new offsets */
    off_t *symbol_names_offset,  /* offset of preexisting symbol names */
    off_t *section_names_offset, /* offset of preexisting section names */
    int *symtabidx               /* index of symtab */
);

#endif /* ZSO_COUNT */
