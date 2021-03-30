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

void convert_symbol(Elf32_Sym *e32, Elf64_Sym *e64, int str_offset)
{
    e64->st_name = e32->st_name + str_offset;
    e64->st_info = e32->st_info;
    e64->st_other = e32->st_other;
    e64->st_shndx = e32->st_shndx; /* will be reordered later */
    e64->st_value = e32->st_value;
    e64->st_size = e32->st_size;
}

void convert_shdr(Elf32_Shdr *e32, Elf64_Shdr *e64, off_t section_offset,
                  off_t name_offset)
{
    e64->sh_name = e32->sh_name + name_offset;
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
        e32->sh_entsize; /* has to be changed separately for REL */
}

void convert_relocation(Elf32_Rel *e32, Elf64_Rela *e64, void *section_contents)
{
    int sym = ELF32_R_SYM(e32->r_info); /*symbol indices don't change */
    int type = ELF32_R_TYPE(e32->r_info);
    if (type == R_386_32)
    {
        type = R_X86_64_32;
    }
    else /* assume there is no other rellocation type */
    {
        type = R_X86_64_PC32;
    }
    e64->r_offset = e32->r_offset;
    e64->r_info = ELF64_R_INFO(sym, type);
    int64_t addend = *(int32_t *)(section_contents + e32->r_offset);
    *(int32_t *)(section_contents + e32->r_offset) = 0;
    e64->r_addend = addend;
}
