#ifndef ZSO_WRITE_ELF
#define ZSO_WRITE_ELF

#include <elf.h>
#include <stdio.h>

size_t write_hdr(FILE *, Elf64_Ehdr *);

int copy_sections(FILE *, void *);

int write_relocations(FILE *, Elf64_Rela *, size_t rel_size);

int create_strtab(FILE *, void *, char *trampoline_strtab,
                  size_t trampoline_strtab_len);

int write_symtab(FILE *, Elf64_Sym *, int symbols);

int write_headers(FILE *, Elf64_Shdr *, int sections64);

#endif /* ZSO_WRITE_ELF */
