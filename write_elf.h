#ifndef ZSO_WRITE_ELF
#define ZSO_WRITE_ELF

#include <elf.h>
#include <stdio.h>

void write_hdr(FILE *, Elf64_Ehdr *);

void copy_sections(FILE *, void *);

void write_relocations(FILE *, Elf64_Rela *, size_t rel_size);

void create_strtab(FILE *, void *, char *trampoline_strtab,
                   size_t trampoline_strtab_len);

void write_symtab(FILE *, Elf64_Sym *, int symbols);

void write_headers(FILE *, Elf64_Shdr *, int sections64);

#endif /* ZSO_WRITE_ELF */
