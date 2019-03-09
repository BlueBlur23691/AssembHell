#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"
#include "memory.h"
#include "files.h"
#include "error.h"

#define START 100

extern int DC, IC;
extern err_node_t *error_list;
int main(int argc, char *argv[])
{
    FILE *file = NULL;
    char *name = NULL, *tok = NULL, *label = NULL, *line_s=NULL;
    unsigned int line_index = 0;
    int i, error, flag = 0;
    label_t *labels = (label_t *)malloc(sizeof(label_t));
    labels->next=NULL;
    label_t *point = labels;
    error_list = (err_node_t *)malloc(sizeof(err_node_t));
    error_list->next=NULL;
    for (i = 1; argv[i]; i++)
    {
        /* Initializtions */
        name = argv[i];
        error = 0;
        DC = 0;
        IC = 0;
        line_s=(char *)malloc(256);
        file = fopen(name, "r");
        name = strtok(name, ".");
        if (file == NULL)
        {
            printf("Error opening file\n");
            return 1;
        }
        while (fgets(line_s, sizeof(line_s), file))
        {
            ++line_index;
            if (line_s[0] != ';' || line_s[0] != '\n')
            {
                printf("%d\t%s\n",line_index,line_s);
                tok = strtok(line_s, " ");
                line_s=strtok(NULL,"");
                flag = 0;
                label = "";

                if (is_type(tok, LABEL))
                {
                    flag = 1;
                    label = (char *)malloc(sizeof(tok));
                    strncpy(label, tok, strlen(tok) - 1);
                    tok = line_s;
                    line_s=strtok(NULL,"");
                }
                if (is_type(tok, DATA))
                {
                    if (flag)
                        error += add_data_label(line_index, name, label, labels);
                    error += update_data(tok, line_s, data, line_index, name);
                    continue;
                }
                else if (is_type(tok, EXTERN))
                {
                    error += add_extern_label(line_index, name, label, labels);
                    continue;
                }
                else if (is_type(tok, CODE))
                {
                    if (flag)
                        error += add_code_label(line_index, name, label, labels);
                    error += update_code(0, tok, line_s, line_index, name, code, labels);
                }
            }
        }
        fclose(file);

        if (error)
        {
            return 1;
            create_error_file(error_list);
        }
        while (point)
        {
            if (point->type == DATAL)
                point->address += IC;
            point = point->next;
        }
        IC = 0;
        file = fopen(name, "r");
        line_index = 0;
        while (fgets(line_s, sizeof(line_s), file))
        {
            if (line_s[0] != ';' && line_s[0] != '\n')
            {
                char *tok = strtok(line_s, " ");
                if (is_type(tok, DATA) || is_type(tok, EXTERN))
                    continue;
                if (is_type(tok, LABEL))
                    tok = strtok(NULL, " ");
                if (is_type(tok, ENTRY))
                {
                    error += update_entry(line_index, name, line_s, labels);
                    continue;
                }
                if (is_type(tok, CODE))
                    error += update_code(1, tok, line_s, line_index, name, code, labels);
            }
            ++line_index;
        }
        if (error)
        {
            return 1;
            create_error_file(error_list);
        }
        create_files(code, data, labels, argv[i]);
        free(labels);
    }
    return 0;
}
