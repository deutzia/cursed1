#ifndef ZSO_CONVERT_ELF
#define ZSO_CONVERT_ELF

#include <elf.h>
#include <stdlib.h>

Elf64_Ehdr *convert_hdr(Elf32_Ehdr *);

void convert_symbol(Elf32_Sym *, Elf64_Sym *, int str_offset);

void convert_shdr(Elf32_Shdr *, Elf64_Shdr *, off_t section_offset,
                  off_t name_offset);

#endif /* ZSO_CONVERT_ELF */
