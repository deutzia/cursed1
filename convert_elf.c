#include "convert_elf.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Elf64_Ehdr *convert_hdr(Elf32_Ehdr *e32)
{
    const size_t n = sizeof(Elf64_Ehdr);
    Elf64_Ehdr *e64 = malloc(n);
    if (e64 == NULL)
    {
        return NULL;
    }

    memcpy(e64->e_ident, e32->e_ident, EI_NIDENT);
    e64->e_ident[EI_CLASS] = ELFCLASS64;
    e64->e_type = e32->e_type;
    e64->e_machine = EM_X86_64;
    e64->e_version = e32->e_version;
    e64->e_entry = (Elf64_Addr)(e32->e_entry);
    e64->e_phoff = (Elf64_Off)(e32->e_phoff);
    e64->e_shoff = (Elf64_Off)(e32->e_shoff); /* will be changed later */
    e64->e_flags = e32->e_flags;
    e64->e_ehsize = n;
    e64->e_phentsize = e32->e_phentsize;
    e64->e_phnum = e32->e_phnum;
    e64->e_shentsize = sizeof(Elf64_Shdr);
    e64->e_shnum = e32->e_shnum;       /* will be changed later */
    e64->e_shstrndx = e32->e_shstrndx; /* will be changed later */

    return e64;
}

Elf64_Sym *convert_symbol(Elf32_Sym *e32, int str_offset, int *sections_reorder)
{
    Elf64_Sym *e64 = malloc(sizeof(Elf64_Sym));
    if (e64 == NULL)
    {
        return NULL;
    }

    e64->st_name = e32->st_name + str_offset;
    e64->st_info = e32->st_info;
    e64->st_other = e32->st_other;
    e64->st_shndx = sections_reorder[e32->st_shndx];
    e64->st_value = e32->st_value;
    e64->st_size = e32->st_size;
    fprintf(stderr, "symbol size = %lu symbol_type = ", e64->st_size);

    switch (ELF32_ST_TYPE(e32->st_info))
    {
    case STT_NOTYPE: {
        fprintf(stderr, "STT_NOTYPE\n");
        break;
    }
    case STT_FUNC: {
        fprintf(stderr, "STT_FUNC\n");
        break;
    }
    case STT_OBJECT: {
        fprintf(stderr, "STT_OBJECT\n");
        break;
    }
    case STT_SECTION: {
        fprintf(stderr, "STT_SECTION\n");
        break;
    }
    case STT_FILE: {
        fprintf(stderr, "STT_FILE\n");
        break;
    }
    default: {
        fprintf(stderr, "OTHER TYPE\n");
        break;
    }
        fprintf(stderr, "\n");
    }

    return e64;
}

void convert_shdr(Elf32_Shdr *e32, Elf64_Shdr *e64, off_t section_offset)
{
    e64->sh_name = e32->sh_name;
    e64->sh_type = e32->sh_type;
    e64->sh_flags = e32->sh_flags;
    e64->sh_addr = e32->sh_addr;
    e64->sh_offset = section_offset;
    e64->sh_size = e32->sh_size;
    e64->sh_link = e32->sh_link;
    e64->sh_info =
        e32->sh_info; /* has to be changed separately for REL and SYMTAB */
    e64->sh_addralign = 16; /* fixed 16 for simplicity */
    e64->sh_entsize =
        e32->sh_entsize; /* has to be changed separately for REL and SYMTAB */
}
