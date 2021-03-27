#ifndef ZSO_WRITE_ELF
#define ZSO_WRITE_ELF

#include <elf.h>
#include <stdio.h>

size_t write_hdr(FILE *, Elf64_Ehdr *);

int copy_sections(FILE *, void *);

int create_strtab(FILE *, void *);

int create_symtab(FILE *, void *, off_t str_offset, int *sections_reorder,
                  int sections64);

int write_headers(FILE *, Elf64_Shdr *, int sections64);

#endif /* ZSO_WRITE_ELF */
