#include "write_elf.h"

#include "convert_elf.h"
#include <stdlib.h>

static const char zeroes[16] = {};

size_t fwrite_helper(const void *ptr, size_t s1, size_t s2, FILE *f)
{
    size_t result = fwrite(ptr, s1, s2, f);
    fprintf(stderr, "Written %lu bytes\n", result);
    return result;
}

size_t write_hdr(FILE *f, Elf64_Ehdr *header)
{
    return fwrite_helper(header, 1, sizeof(Elf64_Ehdr), f);
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
                if (fwrite_helper(elf + e32shdr[i].sh_offset, 1,
                                  e32shdr[i].sh_size, f) != e32shdr[i].sh_size)
                {
                    return 1;
                }
                size_t s = (e32shdr[i].sh_size | 0xf) + 1 - e32shdr[i].sh_size;
                if (fwrite_helper(zeroes, 1, s, f) != s)
                {
                    return 1;
                }
            }
        }
        }
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
            fprintf(stderr, "First byte of STRTAB: %x\n",
                    *(char *)(elf + e32shdr[i].sh_offset));
            if (fwrite_helper(elf + e32shdr[i].sh_offset, 1, e32shdr[i].sh_size,
                              f) != e32shdr[i].sh_size)
            {
                return 1;
            }
            total += e32shdr[i].sh_size;
        }
    }
    if (fwrite_helper(trampoline_strtab, 1, trampoline_strtab_len, f) !=
        trampoline_strtab_len)
    {
        return 1;
    }
    total += trampoline_strtab_len;

    size_t s = (total | 0xf) + 1 - total;
    if (fwrite_helper(zeroes, 1, s, f) != s)
    {
        return 1;
    }

    return 0;
}

int write_symtab(FILE *f, Elf64_Sym *e64, int symbols)
{
    fprintf(stderr, "there are %d symbols, size of one is %lu\n", symbols,
            sizeof(Elf64_Sym));
    if (fwrite_helper(e64, 1, sizeof(Elf64_Sym) * symbols, f) !=
        sizeof(Elf64_Sym) * symbols)
    {
        return 1;
    }
    return 0;
}

int write_headers(FILE *f, Elf64_Shdr *e64, int sections)
{
    if (fwrite_helper(e64, 1, sizeof(Elf64_Shdr) * sections, f) !=
        sizeof(Elf64_Shdr) * sections)
    {
        return 1;
    }

    return 0;
}
