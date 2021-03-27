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

int create_strtab(FILE *f, void *elf)
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
    size_t s = (total | 0xf) + 1 - total;
    if (fwrite_helper(zeroes, 1, s, f) != s)
    {
        return 1;
    }

    return 0;
}

int create_symtab(FILE *f, void *elf, off_t str_offset, int *sections_reorder,
                  int sections64)
{
    Elf32_Ehdr *e32hdr = (Elf32_Ehdr *)elf;
    Elf32_Shdr *e32shdr = (Elf32_Shdr *)(elf + e32hdr->e_shoff);
    size_t total = 0;
    Elf64_Sym e64sym_tmp;

    /* write 0th symbol */
    e64sym_tmp.st_name = 0;
    e64sym_tmp.st_info = 0;
    e64sym_tmp.st_other = 0;
    e64sym_tmp.st_shndx = 0; /* TODO? */
    e64sym_tmp.st_value = 0;
    e64sym_tmp.st_size = 0;
    if (fwrite_helper(&e64sym_tmp, 1, sizeof(Elf64_Sym), f) !=
        sizeof(Elf64_Sym))
    {
        return 1;
    }
    total += sizeof(Elf64_Sym);

    /* create a symbol for every section */
    for (int i = 0; i < sections64; ++i)
    {
        e64sym_tmp.st_name = 0;
        e64sym_tmp.st_info = ELF64_ST_INFO(STB_LOCAL, STT_SECTION);
        e64sym_tmp.st_other = STV_DEFAULT;
        e64sym_tmp.st_shndx = i; /* TODO? */
        e64sym_tmp.st_value = 0;
        e64sym_tmp.st_size = 0;
        if (fwrite_helper(&e64sym_tmp, 1, sizeof(Elf64_Sym), f) !=
            sizeof(Elf64_Sym))
        {
            return 1;
        }
        total += sizeof(Elf64_Sym);
    }

    for (int i = 0; i < e32hdr->e_shnum; ++i)
    {
        if (e32shdr[i].sh_type == SHT_SYMTAB)
        {
            int num_entries = e32shdr[i].sh_size / e32shdr[i].sh_entsize;
            fprintf(stderr, "num entries=  %d\n", num_entries);

            Elf32_Sym *e32sym = (Elf32_Sym *)(elf + e32shdr[i].sh_offset);
            for (int j = 0; j < num_entries; ++j)
            {
                switch (ELF32_ST_TYPE(e32sym[j].st_info))
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
                }
                switch (ELF32_ST_TYPE(e32sym[j].st_info))
                {
                case STT_FUNC:
                case STT_OBJECT: {
                    Elf64_Sym *e64sym = convert_symbol(e32sym + j, str_offset,
                                                       sections_reorder);
                    if (e64sym == NULL)
                    {
                        return 1;
                    }
                    if (fwrite_helper(e64sym, 1, sizeof(Elf64_Sym), f) !=
                        sizeof(Elf64_Sym))
                    {
                        free(e64sym);
                        return 1;
                    }
                    total += sizeof(Elf64_Sym);
                    free(e64sym);
                    break;
                }
                default: {
                    /* ignore all the others */
                    break;
                }
                }
            }
        }
    }

    size_t s = (total | 0xf) + 1 - total;
    if (fwrite_helper(zeroes, 1, s, f) != s)
    {
        return 1;
    }

    return 0;
}

int write_headers(FILE *f, Elf64_Shdr *e64, int sections64)
{
    if (fwrite_helper(e64, 1, sizeof(Elf64_Shdr) * sections64, f) !=
        sizeof(Elf64_Shdr) * sections64)
    {
        return 1;
    }

    return 0;
}
