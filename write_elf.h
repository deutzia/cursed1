#ifndef ZSO_WRITE_ELF
#define ZSO_WRITE_ELF

#include <elf.h>
#include <stdio.h>

size_t write_hdr(FILE *, Elf64_Ehdr *);

int copy_sections(FILE *, void *);

int create_strtab(FILE *, void *);

int create_symtab(FILE *, void *, off_t str_offset, int *sections_reorder);

int create_headers(FILE *, void *, off_t str_offset, int *sections_reorder,
                   off_t *sections_offsets, int symtabidx,
                   int first_nonlocal_symbol, off_t strings_len, int symbols);

#endif /* ZSO_WRITE_ELF */
