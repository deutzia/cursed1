#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "convert_elf.h"
#include "count.h"
#include "handle_flist.h"
#include "read_elf.h"
#include "utils.h"
#include "write_elf.h"

int main(int argc, char *argv[])
{
    int elf_infile = -1;
    void *mapped_elf = MAP_FAILED;
    struct stat stat;
    FILE *flist_infile = NULL;
    FILE *elf_outfile = NULL;
    Elf32_Ehdr *e32hdr = NULL;
    Elf64_Ehdr *e64hdr = NULL;
    Elf64_Shdr *e64shdr = NULL;
    char *trampoline_strtab = NULL;
    Elf64_Sym *e64sym = NULL;
    Elf64_Rela *e64rel = NULL;

    if (argc != 4)
    {
        handle_fatal("Incorrect usage, please use\n\t./converter <ET_REL file> "
                     "<file with function list> <output ET_REL file>\n");
    }

    elf_infile = open(argv[1], O_RDONLY);
    if (elf_infile == -1)
    {
        handle_syserr("Failed to open input file with elf");
    }

    if ((fstat(elf_infile, &stat)) == -1)
    {
        handle_syserr("Failed to stat elf file");
    }

    mapped_elf = mmap(NULL, stat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE,
                      elf_infile, 0);
    if (mapped_elf == MAP_FAILED)
    {
        handle_syserr("Failed to map elf to memory");
    }

    flist_infile = fopen(argv[2], "r");
    if (flist_infile == NULL)
    {
        handle_syserr("Failed to open input file with flist");
    }

    initialize_flist(flist_infile);

    read_hdr(mapped_elf);

    e32hdr = mapped_elf;
    off_t shdr_off = 0;
    int sections64_count = 0;
    int symbols;
    size_t trampoline_strtab_len = 0;
    size_t rel_size = 0;

    count_things(mapped_elf, &sections64_count, &e64shdr, &shdr_off, &symbols,
                 &trampoline_strtab, &trampoline_strtab_len, &e64sym, &e64rel,
                 &rel_size);

    e64hdr = convert_hdr(e32hdr);
    if (e64hdr == NULL)
    {
        handle_fatal("Failed to convert elf header");
    }

    e64hdr->e_shoff = shdr_off;
    e64hdr->e_shnum = sections64_count;
    e64hdr->e_shstrndx = sections64_count - 2;

    elf_outfile = fopen(argv[3], "wb");
    if (elf_outfile == NULL)
    {
        handle_syserr("Failed to open output file");
    }

    write_hdr(elf_outfile, e64hdr);

    copy_sections(elf_outfile, mapped_elf);

    write_relocations(elf_outfile, e64rel, rel_size);

    create_strtab(elf_outfile, mapped_elf, trampoline_strtab,
                  trampoline_strtab_len);

    write_symtab(elf_outfile, e64sym, symbols);

    write_headers(elf_outfile, e64shdr, sections64_count);

    destruct_flist();
    free(e64rel);
    free(trampoline_strtab);
    free(e64sym);
    free(e64shdr);
    fclose(elf_outfile);
    free(e64hdr);
    fclose(flist_infile);
    munmap(mapped_elf, stat.st_size);
    close(elf_infile);

    return 0;
}
