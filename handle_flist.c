#include "handle_flist.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <stdlib.h>

typedef struct flist
{
    char *name;
    tlist f;
    struct flist *next;
} flist;

static flist *flist_head = NULL;

static void destruct_flist_internal(flist *f)
{
    if (f != NULL)
    {
        destruct_flist_internal(f->next);
        free(f);
    }
}

static ZSO_type parse_type(const char *t)
{
    if (strcmp(t, "void") == 0)
    {
        return ZSO_void;
    }
    if (strcmp(t, "int") == 0)
    {
        return ZSO_int;
    }
    if (strcmp(t, "uint") == 0)
    {
        return ZSO_uint;
    }
    if (strcmp(t, "long") == 0)
    {
        return ZSO_long;
    }
    if (strcmp(t, "ulong") == 0)
    {
        return ZSO_ulong;
    }
    if (strcmp(t, "longlong") == 0)
    {
        return ZSO_longlong;
    }
    if (strcmp(t, "ulonglong") == 0)
    {
        return ZSO_ulonglong;
    }
    if (strcmp(t, "ptr") == 0)
    {
        return ZSO_ptr;
    }
    return ZSO_none;
}

bool initialize_flist(FILE *file)
{
    char *buffer = NULL;
    size_t size;
    while ((getline(&buffer, &size, file) != -1))
    {
        int word_begining = 0;
        int word_count = 0;
        flist *node = NULL;
        for (int i = 0; buffer[i] != '\0'; ++i)
        {
            if (isspace(buffer[i]))
            {
                buffer[i] = '\0';
                if (i == 0 || buffer[i - 1] == '\0')
                {
                    continue;
                }
                if (word_count == 0)
                {
                    node = malloc(sizeof(flist));
                    if (node == NULL)
                    {
                        free(buffer);
                        destruct_flist();
                        return false;
                    }
                    node->name = malloc((i - word_begining + 1) * sizeof(char));
                    if (node->name == NULL)
                    {
                        free(buffer);
                        free(node);
                        destruct_flist();
                        return false;
                    }
                    strcpy(node->name, buffer + word_begining);
                    word_begining = i + 1;
                }
                else
                {
                    node->f.t[word_count - 1] =
                        parse_type(buffer + word_begining);
                    if (node->f.t[word_count - 1] == ZSO_none ||
                        (word_count > 1 &&
                         node->f.t[word_count - 1] == ZSO_void))
                    {
                        free(buffer);
                        free(node->name);
                        free(node);
                        destruct_flist();
                        return false;
                    }
                }
                if (++word_count == TLIST_MAX_SIZE + 1)
                {
                    free(buffer);
                    free(node->name);
                    free(node);
                    destruct_flist();
                    return false;
                }
            }
        }
        if (node != NULL)
        { /* empty line */
            node->next = flist_head;
            flist_head = node;
        }
    }
    free(buffer);
    return true;
}

// get type of function with given name or NULL if not found
tlist *lookup_flist(const char *fname)
{
    flist *current_node = flist_head;
    while (current_node != NULL)
    {
        if (strcmp(current_node->name, fname) == 0)
        {
            return &(current_node->f);
        }
    }
    return NULL;
}

void destruct_flist()
{
    destruct_flist_internal(flist_head);
}
