#include "count.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "convert_elf.h"
#include "handle_flist.h"

const char *TRAMPOLINE_PREFIX = "_ZSO_trampoline_";

int count_things(
    void *elf,
    int *sections64,        /* number of sections to copy wo touching them */
    Elf64_Shdr **e64shdr,   /* headers of sections that will be created */
    off_t *size_total,      /* size of stuff before section headers */
    off_t *strtab_len,      /* length of STRTAB entries that get copied */
    int *symbols,           /* count symbols */
    int **sections_reorder, /* mapping of old section numbers to new ones */
    off_t *
        *sections_offsets, /* mapping of old section numbers to new offsets */
    char **
        trampoline_strtab, /* new strtab that needs to be glued into old ones */
    size_t *trampoline_strtab_len, /* length of that strtab */
    Elf64_Sym **e64sym,  /* symbols that need to be pasted into resulting elf */
    Elf64_Rela **e64rel, /* relocations that need to be pasted */
    size_t *rel_size     /* size of such relocations */
)
{
    Elf32_Ehdr *e32hdr = (Elf32_Ehdr *)elf;
    Elf32_Shdr *e32shdr = (Elf32_Shdr *)(elf + e32hdr->e_shoff);
    off_t symbol_names_offset = 0;
    off_t section_names_offset = 0;
    /* shstrtab or shrtab, actually */
    int symtabidx = 0;
    int shstrndx = e32hdr->e_shstrndx;
    int symstrndx = 0;
    for (int i = 0; i < e32hdr->e_shnum; ++i)
    {
        if (e32shdr[i].sh_type == SHT_SYMTAB)
        {
            symstrndx = e32shdr[i].sh_link;
            symtabidx = i;
            break; /*assume there is exactly one symtab */
        }
    }
    *sections_reorder = calloc(e32hdr->e_shnum, sizeof(int));
    if (*sections_reorder == NULL)
    {
        return 1;
    }
    *sections_offsets = calloc(e32hdr->e_shnum, sizeof(off_t));
    if (*sections_offsets == NULL)
    {
        return 1;
    }
    *e64shdr = calloc(e32hdr->e_shnum + 8, sizeof(Elf64_Shdr));
    if (*e64shdr == NULL)
    {
        return 1;
    }
    *sections64 = 0;
    *size_total = sizeof(Elf64_Ehdr);
    *strtab_len = 0;
    *symbols = 0;
    *trampoline_strtab_len = 0;
    size_t trampoline_strtab_allocated =
        get_flist_size() * (strlen(TRAMPOLINE_PREFIX) + 10);
    *trampoline_strtab = malloc(trampoline_strtab_allocated);
    *rel_size = 0;
    *e64rel = NULL;
    if (*trampoline_strtab == NULL)
    {
        return 1;
    }

    for (int i = 0; i < e32hdr->e_shnum; ++i)
    {
        if (e32shdr[i].sh_type == SHT_STRTAB)
        {
            if (i == shstrndx)
            {
                section_names_offset += *strtab_len;
            }
            if (i == symstrndx)
            {
                symbol_names_offset += *strtab_len;
            }
            *strtab_len += e32shdr[i].sh_size;
        }
        else
        {
            if (e32shdr[i].sh_type == SHT_SYMTAB)
            {
                *symbols = e32shdr[i].sh_size / e32shdr[i].sh_entsize;
            }
        }
    }

    *e64sym = calloc(*symbols, sizeof(Elf64_Sym));
    if (*e64sym == NULL)
    {
        return 1;
    }

    /* after this loop the following have to be fixed:
     * links need to be reordered
     * section symbols have to be fixed
     */
    for (int i = 0; i < e32hdr->e_shnum; ++i)
    {
        switch (e32shdr[i].sh_type)
        {
        case SHT_NULL: /* explicitly copy 0th element of section table */
        {
            (*sections_reorder)[i] = (*sections64);
            convert_shdr(e32shdr + i, (*e64shdr) + (*sections64), 0, 0);
            (*sections64)++;
            break;
        }
        case SHT_SYMTAB: {
            int num_entries = e32shdr[i].sh_size / e32shdr[i].sh_entsize;

            Elf32_Sym *e32sym = (Elf32_Sym *)(elf + e32shdr[i].sh_offset);
            if (e32shdr[i].sh_link == 0)
            {
                /* no name info, considering this incorrect */
                return 1;
            }
            char *strtab = elf + e32shdr[e32shdr[i].sh_link].sh_offset;
            for (int j = 0; j < num_entries; ++j)
            {
                switch (ELF32_ST_TYPE(e32sym[j].st_info))
                {
                case STT_NOTYPE:
                case STT_FUNC: { /* symbols that may be found in flist */
                    char *symbol_name = strtab + e32sym[j].st_name;
                    if (lookup_flist(symbol_name))
                    {
                        /* trampoline will be generated for this symbol */
                        size_t new_len =
                            strlen(TRAMPOLINE_PREFIX) + strlen(symbol_name) + 1;
                        convert_symbol(e32sym + j, (*e64sym) + j,
                                       symbol_names_offset);
                        (*e64sym)[j].st_name =
                            (*trampoline_strtab_len) + (*strtab_len);
                        while ((*trampoline_strtab_len) + new_len >
                               trampoline_strtab_allocated)
                        {
                            void *new_mem =
                                realloc(*trampoline_strtab,
                                        2 * trampoline_strtab_allocated);
                            if (new_mem == NULL)
                            {
                                return -1;
                            }
                            *trampoline_strtab = new_mem;
                            trampoline_strtab_allocated *= 2;
                        }
                        strcpy((*trampoline_strtab) + (*trampoline_strtab_len),
                               TRAMPOLINE_PREFIX);
                        (*trampoline_strtab_len) += strlen(TRAMPOLINE_PREFIX);
                        strcpy((*trampoline_strtab) + (*trampoline_strtab_len),
                               symbol_name);
                        (*trampoline_strtab_len) += strlen(symbol_name) + 1;
                        /* write which side trampolines should be generated to
                         * stdout */
                        if (ELF32_ST_TYPE(e32sym[j].st_info) == STT_NOTYPE)
                        {
                            printf("%s out\n", symbol_name);
                        }
                        else
                        {
                            printf("%s in\n", symbol_name);
                        }
                    }
                    else
                    {
                        convert_symbol(e32sym + j, (*e64sym) + j,
                                       symbol_names_offset);
                    }
                    break;
                }
                case STT_OBJECT: {
                    if (ELF32_ST_BIND(e32sym[j].st_info) == STB_LOCAL)
                    {
                        // assume all local ones are before nonlocal ones */
                    }
                    convert_symbol(e32sym + j, (*e64sym) + j,
                                   symbol_names_offset);
                    break;
                }
                case STT_SECTION: {
                    /* 0 as third arg bc those symbols don't have names */
                    convert_symbol(e32sym + j, (*e64sym) + j, 0);
                }
                default: {
                    /* ignore all the others */
                    break;
                }
                }
            }
            break;
        }
        case SHT_STRTAB: {
            break;
        }
        case SHT_NOTE: /* explicitly don't copy this */
        {
            break;
        }
        case SHT_REL: {
            break;
        }
        default: {
            if (e32shdr[i].sh_flags & SHF_ALLOC)
            {
                (*sections_reorder)[i] = (*sections64);
                convert_shdr(e32shdr + i, (*e64shdr) + (*sections64),
                             *size_total, section_names_offset);
                (*sections64)++;
                (*sections_offsets)[i] = *size_total;
                *size_total += (e32shdr[i].sh_size | 0xf) +
                               1; /* or-ing here aligns size to 16 */
            }
        }
        }
    }

    for (int i = 0; i < e32hdr->e_shnum; ++i)
    {
        /* checking sh_info here assumes that there are no relocations to strtab
         * or symtab */
        if (e32shdr[i].sh_type == SHT_REL &&
            (*sections_reorder)[e32shdr[i].sh_info] != 0)
        {
            int num_entries = e32shdr[i].sh_size / e32shdr[i].sh_entsize;
            size_t new_size = (*rel_size) + num_entries * sizeof(Elf64_Rela);
            new_size = (new_size | 0xf) + 1;
            void *new_mem = realloc(*e64rel, new_size);
            if (new_mem == NULL)
            {
                return 1;
            }
            *e64rel = new_mem;
            void *target_section = elf + e32shdr[e32shdr[i].sh_info].sh_offset;
            Elf32_Rel *e32rel = (Elf32_Rel *)(elf + e32shdr[i].sh_offset);
            for (int j = 0; j < num_entries; ++j)
            {
                convert_relocation(e32rel + j,
                                   (Elf64_Rela *)(((void *)(*e64rel)) +
                                                  (*rel_size) +
                                                  j * sizeof(Elf64_Rela)),
                                   target_section);
            }
            convert_shdr(e32shdr + i, (*e64shdr) + (*sections64), *size_total,
                         section_names_offset);
            (*sections_offsets)[i] = *size_total;
            (*sections_reorder)[i] = *sections64;
            *size_total += ((num_entries * sizeof(Elf64_Rela)) | 0xf) + 1;
            (*e64shdr)[*sections64].sh_type = SHT_RELA;
            (*e64shdr)[*sections64].sh_info =
                (*sections_reorder)[(*e64shdr)[*sections64].sh_info];
            (*e64shdr)[*sections64].sh_entsize = sizeof(Elf64_Rela);
            (*e64shdr)[*sections64].sh_size = sizeof(Elf64_Rela) * num_entries;
            (*sections64)++;
            *rel_size += num_entries * sizeof(Elf64_Rela);
            int missing = ((*rel_size) | 0xf) + 1 - *rel_size;
            memset(((void *)(*e64rel)) + (*rel_size), '\0', missing);
            *rel_size += missing;
        }
    }

    for (int i = 0; i < e32hdr->e_shnum; ++i)
    {
        switch (e32shdr[i].sh_type)
        {
        case SHT_SYMTAB: {
            (*sections_reorder)[i] = *sections64 + 1;
            (*sections_offsets)[i] = ((*size_total + *strtab_len) | 0xf) + 1;
            break;
        }
        case SHT_STRTAB: {
            (*sections_reorder)[i] = *sections64;
            (*sections_offsets)[i] = *size_total;
            break;
        }
        }
    }

    *strtab_len += *trampoline_strtab_len;

    /* Reorder section numbers in section symbols */
    for (int i = 1; i < *symbols; ++i)
    {
        if (ELF32_ST_TYPE((*e64sym)[i].st_info) == STT_SECTION)
        {
            if ((*sections_reorder)[(*e64sym)[i].st_shndx] == 0)
            {
//                (*e64sym)[i].st_info = ELF64_ST_INFO(STB_LOCAL, STT_NOTYPE);
                memset((*e64sym) + i, '\0', sizeof(Elf64_Sym));
            }
            else
            {
                (*e64sym)[i].st_shndx =
                    (*sections_reorder)[(*e64sym)[i].st_shndx];
            }
        }
        else
        {
            (*e64sym)[i].st_shndx = (*sections_reorder)[(*e64sym)[i].st_shndx];
        }
    }

    int strtabidx = e32shdr[symtabidx].sh_link;
    convert_shdr(e32shdr + strtabidx, (*e64shdr) + (*sections64), *size_total,
                 section_names_offset);
    (*e64shdr)[*sections64].sh_size = *strtab_len;
    *size_total += ((*strtab_len) | 0xf) + 1;
    (*sections64)++;

    convert_shdr(e32shdr + symtabidx, (*e64shdr) + (*sections64), *size_total,
                 section_names_offset);
    (*e64shdr)[*sections64].sh_entsize = sizeof(Elf64_Sym);
    (*e64shdr)[*sections64].sh_size = (*symbols) * sizeof(Elf64_Sym);
    (*sections64)++;
    *size_total += (((*symbols) * sizeof(Elf64_Sym)) | 0xf) + 1;

    for (int i = 0; i < *sections64; ++i)
    {
        (*e64shdr)[i].sh_link = (*sections_reorder)[(*e64shdr)[i].sh_link];
    }

    return 0;
}
