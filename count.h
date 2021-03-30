#ifndef ZSO_COUNT
#define ZSO_COUNT

#include <elf.h>
#include <stdlib.h>

/* if returned value is not zero then it errored */
int count_things(
    void *elf,              /* mapped mempory */
    int *sections64,        /* number of sections that will be generated */
    Elf64_Shdr **e64shdr,   /*headers of sections that will be created */
    off_t *size_total,      /* size of stuff before section headers */
    off_t *strtab_len,      /* length of STRTAB entries that get copied */
    int *symbols,           /* count symbols */
    int **sections_reorder, /* mapping of old section numbers to new ones */
    off_t *
        *sections_offsets, /* mapping of old section numbers to new offsets */
    char **
        trampoline_strtab, /* new strtab that needs to be glued into old ones */
    size_t *trampoline_strtab_len, /* length of that strtab */
    Elf64_Sym **e64sym,  /* symbols that need to be pasted into resulting elf */
    Elf64_Rela **e64rel, /* relocations that need to be pasted */
    size_t *rel_size     /* size of such relocations */
);

#endif /* ZSO_COUNT */
