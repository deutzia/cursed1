#ifndef ZSO_CONVERT_ELF
#define ZSO_CONVERT_ELF

#include <elf.h>
#include <stdlib.h>

Elf64_Ehdr *convert_hdr(Elf32_Ehdr *);

Elf64_Sym *convert_symbol(Elf32_Sym *, int str_offset, int *sections_reorder);

Elf64_Shdr *convert_shdr(Elf32_Shdr *, off_t str_offset, int *sections_reorder,
                         off_t section_offset);

#endif /* ZSO_CONVERT_ELF */