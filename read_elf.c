#include "read_elf.h"

#include "utils.h"
#include <stdlib.h>

void read_hdr(void *memory)
{
    Elf32_Ehdr *header = (Elf32_Ehdr *)memory;
    /* check if this is a valid elf for this conversion */
    if (header->e_type == ET_REL &&
        header->e_ident[EI_MAG0] == ELFMAG0 && /* 0x7f */
        header->e_ident[EI_MAG1] == ELFMAG1 && /* E */
        header->e_ident[EI_MAG2] == ELFMAG2 && /* L */
        header->e_ident[EI_MAG3] == ELFMAG3 && /* F */
        header->e_machine == EM_386)
    {
        return;
    }
    handle_fatal("Invalid elf header");
}
