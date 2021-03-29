#include <errno.h>
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
#include "write_elf.h"

typedef void (*error_handler)(const char *);

void handle_syserr(const char *msg)
{
    fprintf(stderr, "ERROR: %s (%s)\n", msg, strerror(errno));
    exit(1);
}

void handle_fatal(const char *msg)
{
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    error_handler handler = NULL;
    char *msg = NULL;
    int err = 0;
    int elf_infile = -1;
    void *mapped_elf = MAP_FAILED;
    struct stat stat;
    FILE *flist_infile = NULL;
    FILE *elf_outfile = NULL;
    Elf32_Ehdr *e32hdr = NULL;
    Elf64_Ehdr *e64hdr = NULL;
    int *sections_reorder = NULL;
    off_t *sections_offsets = NULL;
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
        handler = handle_syserr;
        msg = "Failed to open input file with elf";
        goto handle_err;
    }

    if ((fstat(elf_infile, &stat)) == -1)
    {
        handler = handle_syserr;
        msg = "Failed to stat elf file";
        goto handle_err;
    }

    mapped_elf = mmap(NULL, stat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE,
                      elf_infile, 0);
    if (mapped_elf == MAP_FAILED)
    {
        handler = handle_syserr;
        msg = "Failed to map elf to memory";
        goto handle_err;
    }

    flist_infile = fopen(argv[2], "r");
    if (flist_infile == NULL)
    {
        handler = handle_syserr;
        msg = "Failed to open input file with flist";
        goto handle_err;
    }

    if (!initialize_flist(flist_infile))
    {
        handler = handle_fatal;
        msg = "Failed to parse flist file";
        goto handle_err;
    }

    if (!read_hdr(mapped_elf))
    {
        handler = handle_fatal;
        msg = "Failed to read elf header";
        goto handle_err;
    }

    e32hdr = mapped_elf;
    off_t shdr_off = 0;
    int sections64_count = 0;
    off_t strings_len;
    int symbols, symtabidx;
    off_t symbol_names_offset, section_names_offset;
    size_t trampoline_strtab_len = 0;
    size_t rel_size = 0;

    if (count_things(mapped_elf, &sections64_count, &e64shdr, &shdr_off,
                     &strings_len, &symbols, &sections_reorder,
                     &sections_offsets, &symbol_names_offset,
                     &section_names_offset, &symtabidx, &trampoline_strtab,
                     &trampoline_strtab_len, &e64sym, &e64rel, &rel_size) != 0)
    {
        handler = handle_fatal;
        msg = "Failed to do counting of objects in the elf";
        goto handle_err;
    }

    e64hdr = convert_hdr(e32hdr);
    if (e64hdr == NULL)
    {
        handler = handle_fatal;
        msg = "Failed to convert elf header";
        goto handle_err;
    }

    fprintf(stderr, "shdr_off = %ld\n", shdr_off);
    e64hdr->e_shoff = shdr_off;
    e64hdr->e_shnum = sections64_count;
    e64hdr->e_shstrndx = sections64_count - 2;
    /* TODO to ^ sie zmieni jak beda relokacje */

    elf_outfile = fopen(argv[3], "wb");
    if (elf_outfile == NULL)
    {
        handler = handle_syserr;
        msg = "Failed to open output file";
        goto handle_err;
    }

    if ((write_hdr(elf_outfile, e64hdr)) != sizeof(Elf64_Ehdr))
    {
        handler = handle_fatal;
        msg = "Failed to write to output file\n";
        goto handle_err;
    }

    if (copy_sections(elf_outfile, mapped_elf) != 0)
    {
        handler = handle_fatal;
        msg = "Failed to copy elf sections";
        goto handle_err;
    }

    if (write_relocations(elf_outfile, e64rel, rel_size) != 0)
    {
        handler = handle_fatal;
        msg = "Failed to write relocations";
        goto handle_err;
    }

    if (create_strtab(elf_outfile, mapped_elf, trampoline_strtab,
                      trampoline_strtab_len) != 0)
    {
        handler = handle_fatal;
        msg = "Failed to produce strtab section";
        goto handle_err;
    }

    if (write_symtab(elf_outfile, e64sym, symbols) != 0)
    {
        handler = handle_fatal;
        msg = "Failed to produce symtab section";
        goto handle_err;
    }

    if (write_headers(elf_outfile, e64shdr, sections64_count) != 0)
    {
        handler = handle_fatal;
        msg = "Failed to write section headers";
        goto handle_err;
    }

    destruct_flist();
    free(e64rel);
    free(trampoline_strtab);
    free(e64sym);
    free(e64shdr);
    free(sections_offsets);
    free(sections_reorder);
    fclose(elf_outfile);
    free(e64hdr);
    fclose(flist_infile);
    munmap(mapped_elf, stat.st_size);
    close(elf_infile);

    return 0;

handle_err:
    err = errno;
    destruct_flist();
    if (e64rel != NULL)
    {
        free(e64rel);
    }
    if (trampoline_strtab != NULL)
    {
        free(trampoline_strtab);
    }
    if (e64sym != NULL)
    {
        free(e64sym);
    }
    if (e64shdr != NULL)
    {
        free(e64shdr);
    }
    if (sections_offsets != NULL)
    {
        free(sections_offsets);
    }
    if (sections_reorder != NULL)
    {
        free(sections_reorder);
    }
    if (elf_outfile != NULL)
    {
        fclose(elf_outfile);
    }
    if (e64hdr != NULL)
    {
        free(e64hdr);
    }
    if (flist_infile != NULL)
    {
        fclose(flist_infile);
    }
    if (mapped_elf != MAP_FAILED)
    {
        munmap(mapped_elf, stat.st_size);
    }
    if (elf_infile != -1)
    {
        close(elf_infile);
    }
    errno = err;
    handler(msg);
}
