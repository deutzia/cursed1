#include "count.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int count_things(
    void *elf,
    int *sections_to_copy, /* number of sections to copy wo touching them */
    off_t *size_total,      /* size of stuff before section headers */
    off_t *strtab_len,      /* length of STRTAB entries that get copied */
    int *symbols,          /* count symbols */
    int *first_nonlocal_symbol, /* used for st_info in symtab header */
    int **sections_reorder,     /* mapping of old section numbers to new ones */
    off_t *
        *sections_offsets, /* mapping of old section numbers to new offsets */
    off_t *symbol_names_offset,  /* offset of preexisting symbol names */
    off_t *section_names_offset, /* offset of preexisting section names */
    int *symtabidx               /* index of symtab */
)
{
    Elf32_Ehdr *e32hdr = (Elf32_Ehdr *)elf;
    Elf32_Shdr *e32shdr = (Elf32_Shdr *)(elf + e32hdr->e_shoff);
    // TODO ifologia jak czegos nie ma to error
    /* shstrtab or shrtab, actually */
    int shstrndx = e32hdr->e_shstrndx;
    int symstrndx = 0;
    for (int i = 0; i < e32hdr->e_shnum; ++i)
    {
        if (e32shdr[i].sh_type == SHT_SYMTAB)
        {
            symstrndx = e32shdr[i].sh_link;
            *symtabidx = i;
            break;
        }
    }
    char *shstrtab = (char *)(elf + e32shdr[e32hdr->e_shstrndx].sh_offset);
    *sections_reorder = calloc(e32hdr->e_shnum, sizeof(int));
    if (*sections_reorder == NULL)
    {
        return 1;
    }
    *sections_offsets = calloc(e32hdr->e_shnum, sizeof(off_t));
    if (*sections_offsets == NULL)
    {
        free(*sections_reorder);
        return 1;
    }
    *sections_to_copy = 0;
    *size_total = sizeof(Elf64_Ehdr);
    *strtab_len = 0;
    *symbols = 0;
    *symbol_names_offset = 0;
    *section_names_offset = 0;

    for (int i = 0; i < e32hdr->e_shnum; ++i)
    {
        printf("%s:\t", shstrtab + e32shdr[i].sh_name);
        switch (e32shdr[i].sh_type)
        {
        case SHT_NULL: /* explicitly copy 0th element of section table */
        {
            (*sections_reorder)[i] = (*sections_to_copy)++;
            break;
        }
        case SHT_SYMTAB: {
            printf("SHT_SYMTAB");
            int num_entries = e32shdr[i].sh_size / e32shdr[i].sh_entsize;
            fprintf(stderr, "num entries=  %d\n", num_entries);

            Elf32_Sym *e32sym = (Elf32_Sym *)(elf + e32shdr[i].sh_offset);
            for (int j = 0; j < num_entries; ++j)
            {
                switch (ELF32_ST_TYPE(e32sym[j].st_info))
                {
                case STT_NOTYPE:
                case STT_FUNC:
                case STT_OBJECT:
                case STT_SECTION: {
                    if (ELF32_ST_BIND(e32sym[j].st_info) == STB_LOCAL)
                    {
                        // assume all local ones are before nonlocal ones */
                        (*first_nonlocal_symbol)++;
                    }
                    (*symbols)++;
                    break;
                }
                default: {
                    /* ignore all the others */
                    break;
                }
                }
            }
            printf("(%d)", num_entries);
            break;
        }
        case SHT_STRTAB: {
            if (i == shstrndx)
            {
                *section_names_offset += *strtab_len;
            }
            if (i == symstrndx)
            {
                *symbol_names_offset += *strtab_len;
            }
            *strtab_len += e32shdr[i].sh_size;
            printf("SHT_STRTAB");
            break;
        }
        case SHT_NOTE: /* explicitly don't copy this */
        {
            break;
        }
        case SHT_REL: {
            printf("SHT_REL");
            //                exit(1); /* TODO */
            break;
        }
        default: {
            if (e32shdr[i].sh_flags & SHF_ALLOC)
            {
                (*sections_reorder)[i] = (*sections_to_copy)++;
                (*sections_offsets)[i] = *size_total;
                *size_total += (e32shdr[i].sh_size | 0xf) +
                              1; /* or-ing here aligns size to 16 */
                fprintf(stderr, "Changing size_total to %ld\n", *size_total);
            }
            printf("OTHER SECTION: %d", e32shdr[i].sh_type);
        }
        }
        printf("\talloc: %d num: %d link:%d\n", e32shdr[i].sh_flags & SHF_ALLOC,
               i, e32shdr[i].sh_link);
    }

    for (int i = 0; i < e32hdr->e_shnum; ++i)
    {
        switch (e32shdr[i].sh_type)
        {
        case SHT_SYMTAB: {
            (*sections_reorder)[i] = *sections_to_copy + 1;
            (*sections_offsets)[i] =
                ((*size_total + *strtab_len) | 0xf) + 1;
            break;
        }
        case SHT_STRTAB: {
            (*sections_reorder)[i] = *sections_to_copy;
            (*sections_offsets)[i] = *size_total;
            break;
        }
        }
    }
    *size_total += ((*strtab_len) | 0xf) + 1;
    fprintf(stderr, "Changing size_total to %ld\n", *size_total);
    fprintf(stderr, "there are %d symbols\n", *symbols);
    *size_total += (((*symbols) * sizeof(Elf64_Sym)) | 0xf) + 1;
    fprintf(stderr, "Changing size_total to %ld\n", *size_total);

    return 0;
}
