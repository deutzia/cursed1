#include "handle_flist.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#include "utils.h"

typedef struct flist
{
    char *name;
    struct flist *next;
} flist;

static flist *flist_head = NULL;

static void destruct_flist_internal(flist *f)
{
    if (f != NULL)
    {
        destruct_flist_internal(f->next);
        free(f->name);
        free(f);
    }
}

void initialize_flist(FILE *file)
{
    char *buffer = NULL;
    size_t size;
    while ((getline(&buffer, &size, file) != -1))
    {
        int word_begining = 0;
        flist *node = NULL;
        for (int i = 0; buffer[i] != '\0'; ++i)
        {
            if (isspace(buffer[i]))
            {
                buffer[i] = '\0';
                if (i == 0 || buffer[i - 1] == '\0')
                {
                    word_begining = i;
                    continue;
                }
                node = calloc(1, sizeof(flist));
                if (node == NULL)
                {
                    handle_fatal(mem);
                }
                node->name = malloc((i + 1) * sizeof(char));
                if (node->name == NULL)
                {
                    handle_fatal(mem);
                }
                strcpy(node->name, buffer + word_begining);
            }
        }
        if (node != NULL)
        { /* empty line */
            node->next = flist_head;
            flist_head = node;
        }
    }
    free(buffer);
}

// get type of function with given name or NULL if not found
bool lookup_flist(const char *fname)
{
    flist *current_node = flist_head;
    while (current_node != NULL)
    {
        if (strcmp(current_node->name, fname) == 0)
        {
            return true;
        }
        current_node = current_node->next;
    }
    return false;
}

void destruct_flist()
{
    destruct_flist_internal(flist_head);
}

size_t get_flist_size()
{
    flist *current_node = flist_head;
    size_t result = 0;
    while (current_node != NULL)
    {
        result++;
        current_node = current_node->next;
    }
    return result;
}
