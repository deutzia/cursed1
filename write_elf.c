#include "write_elf.h"

#include "convert_elf.h"
#include <stdlib.h>

static const char zeroes[16] = {};

size_t write_hdr(FILE *f, Elf64_Ehdr *header)
{
    return fwrite(header, 1, sizeof(Elf64_Ehdr), f);
}

int copy_sections(FILE *f, void *elf)
{
    Elf32_Ehdr *e32hdr = (Elf32_Ehdr *)elf;
    Elf32_Shdr *e32shdr = (Elf32_Shdr *)(elf + e32hdr->e_shoff);

    for (int i = 0; i < e32hdr->e_shnum; ++i)
    {
        switch (e32shdr[i].sh_type)
        {
        case SHT_NULL: /* explicitly copy 0th element of section table */
        case SHT_SYMTAB:
        case SHT_STRTAB:
        case SHT_NOTE: /* explicitly don't copy this */
        case SHT_REL: {
            break;
        }
        default: {
            if (e32shdr[i].sh_flags & SHF_ALLOC)
            {
                if (fwrite(elf + e32shdr[i].sh_offset, 1, e32shdr[i].sh_size,
                           f) != e32shdr[i].sh_size)
                {
                    return 1;
                }
                size_t s = (e32shdr[i].sh_size | 0xf) + 1 - e32shdr[i].sh_size;
                if (fwrite(zeroes, 1, s, f) != s)
                {
                    return 1;
                }
            }
        }
        }
    }

    return 0;
}

int write_relocations(FILE *f, Elf64_Rela *e64, size_t rel_size)
{
    if (fwrite(e64, 1, rel_size, f) != rel_size)
    {
        return 1;
    }
    return 0;
}

int create_strtab(FILE *f, void *elf, char *trampoline_strtab,
                  size_t trampoline_strtab_len)
{
    Elf32_Ehdr *e32hdr = (Elf32_Ehdr *)elf;
    Elf32_Shdr *e32shdr = (Elf32_Shdr *)(elf + e32hdr->e_shoff);

    size_t total = 0;

    for (int i = 0; i < e32hdr->e_shnum; ++i)
    {
        if (e32shdr[i].sh_type == SHT_STRTAB)
        {
            if (fwrite(elf + e32shdr[i].sh_offset, 1, e32shdr[i].sh_size, f) !=
                e32shdr[i].sh_size)
            {
                return 1;
            }
            total += e32shdr[i].sh_size;
        }
    }
    if (fwrite(trampoline_strtab, 1, trampoline_strtab_len, f) !=
        trampoline_strtab_len)
    {
        return 1;
    }
    total += trampoline_strtab_len;

    size_t s = (total | 0xf) + 1 - total;
    if (fwrite(zeroes, 1, s, f) != s)
    {
        return 1;
    }

    return 0;
}

int write_symtab(FILE *f, Elf64_Sym *e64, int symbols)
{
    size_t s = sizeof(Elf64_Sym) * symbols;
    if (fwrite(e64, 1, s, f) != s)
    {
        return 1;
    }
    s = ((s | 0xf) + 1) - s;
    if (fwrite(zeroes, 1, s, f) != s)
    {
        return 1;
    }
    return 0;
}

int write_headers(FILE *f, Elf64_Shdr *e64, int sections)
{
    if (fwrite(e64, 1, sizeof(Elf64_Shdr) * sections, f) !=
        sizeof(Elf64_Shdr) * sections)
    {
        return 1;
    }

    return 0;
}
