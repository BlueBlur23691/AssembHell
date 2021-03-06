#include "memory.h"
#include "parse.h"
#include <string.h>

/**
 * @brief Adds code words to memory
 * first cycle - adds code words except labels
 * second cycle - adds label code words
 * 
 * @param run 0 = first cycle , 1 = second cycle
 * @param tok The opcode
 * @param line_s The string of rest of the line
 * @param line_index Number of current line
 * @param fname Name of current file
 * @param code Memory of code words
 * @param labels Labels table
 * @return int returns 1 if error
 */
int update_code(int run, char *tok, char *line_s, unsigned int line_index,
                char *fname, unsigned int *code, label_t **labels)
{
    /*
    *  parse - Holds value code word
    *  opnum - Number representing the opcode
    *  first_register - Used to determine if both operands are register and if so add only one code word
    *  args - arguments
    *  curr - pointer to head of labels used in second cycle
    */
    unsigned int parse = 0, opnum = 0, first_register = 0;
    char *args = line_s;
    label_t **curr = labels;

    /* First cycle */
    if (!run)
    {
        /* Adds operand code word to memory */
        if (parse_code(tok, line_s, &parse, line_index, fname)) /* error */
            return 1;
        code[IC] = parse;
        ++IC;
        opnum = find_opcode(tok);
        if (opnum == OPCODE_NUM)
        {
            add_front(&error_list, line_index, fname, "Unknown opcode");
            return 1;
        }
        if (comma_check(opnum, args, 0))
        {
            add_front(&error_list, line_index, fname, "Inappropriate use of commas");
            return 1;
        }
        parse = 0;
        args = strtok(args, " , ");
        /* Opcode with 2 operands */
        if (opnum <= SUB || opnum == LEA)
        {
            if (first_cycle_parse_operand(args,2,&parse,&first_register,line_index,fname)) return 1;
            args = strtok(NULL, " , ");
            /* 
            *  If the first argument wasn't a register add to memory
            *  Otherwise checks if second argument is also register
            */
            if (!first_register)
            {
                code[IC] = parse;
                parse = 0;
                ++IC;
            }
            if (first_cycle_parse_operand(args,1,&parse,&first_register,line_index,fname)) return 1;
            code[IC] = parse;
            ++IC;
        }
        /* Opcode with 1 operand */
        else if (opnum == NOT || opnum == CLR || (LEA < opnum && opnum < RTS))
        {
            if (first_cycle_parse_operand(args,1,&parse,&first_register,line_index,fname)) return 1;
            code[IC] = parse;
            ++IC;
        }
        args = strtok(NULL, " , ");
        if (args)
        {
            add_front(&error_list, line_index, fname,
                      "Extraneous text after end of command");
            return 1;
        }
    }
    /* Second cycle */
    else
    {
        opnum = find_opcode(tok);
        args = strtok(args, " , ");
        ++IC;
        /* Opcode with 2 operands */
        if (opnum <= SUB || opnum == LEA)
        {
            if(second_cycle_parse_operand(args,2,&parse,&first_register,curr,line_index,fname)) return 1;
            parse = 0;
            args = strtok(NULL, " , ");
            curr = labels;
            if(second_cycle_parse_operand(args,1,&parse,&first_register,curr,line_index,fname)) return 1;
            
        }
        /* Opcode with 1 operand */
        else if (opnum == NOT || opnum == CLR || (LEA < opnum && opnum < RTS))
            if(second_cycle_parse_operand(args,1,&parse,&first_register,curr,line_index,fname)) return 1;
    }
    return 0;
}

/**
 * @brief Adds data words to memory
 * 
 * @param tok Data type
 * @param line Arguments
 * @param data Memory of data words
 * @param line_index Number of current line
 * @param fname Name of current file
 * @return int returns 1 if error
 */
int update_data(char *tok, char *line, unsigned int *data,
                unsigned int line_index, char *fname)
{
    /*
    *  parse - Holds value data word
    *  i - iterator
    *  start_of_args - Used to free args
    *  args - arguments
    *  argument - Number of arguments in number type used in comma check
    */
    char *args = duplicate_string(line);
    char *start_of_args = args;
    unsigned int parse = 0, i, argument = 0;
    if (!line)
    {
         add_front(&error_list, line_index, fname, "Arguments missing");
            return 1;
    }
    /* Number type */
    if (!strcmp(tok, ".data"))
    {
        args = strtok(args, " , ");
        /* Adds each num to memory */
        while (args)
        {
            if (parse_data(args, NUM_DATA, &parse, line_index, fname))
                return 1;
            data[DC] = parse;
            DC++;
            ++argument;
            args = strtok(NULL, " , ");
        }
        if (comma_check(OPCODE_NUM, line, argument))
        {
            add_front(&error_list, line_index, fname, "Inappropriate use of commas");
            return 1;
        }
    }
    /* String type */
    else
    {
        /* Check if stars with " */
        if (line[0] != '"')
        {
            add_front(&error_list, line_index, fname, "Missing a closing \"");
            return 1;
        }
        /* Adds each char to memory */
        for (i = 1; i < strlen(args) - 1 && args[i] != '"'; i++)
        {
            parse_data(args + i, CHAR_DATA, &parse, line_index, fname);
            data[DC] = parse;
            DC++;
        }
        /* Check if end with " */
        if (i == strlen(args) - 1 && args[i] != '"')
        {
            add_front(&error_list, line_index, fname, "Missing a closing \"");
            return 1;
        }
        args = strtok(args + i + 1, " ");
        if (args)
        {
            add_front(&error_list, line_index, fname,
                      "Extraneous text after end of command");
            return 1;
        }
        /* Adds '\0' */
        data[DC] = 0;
        DC++;
    }

    free(start_of_args);
    return 0;
}

/**
 * @brief Adds node to linked list
 * 
 * @param type Label type
 * @param name Label name
 * @param address Label address
 * @param labels Head of linked list
 */
void add_label(int type, char *name, int address, label_t **labels)
{
    label_t *new_node = (label_t *)malloc(sizeof(label_t));

    new_node->type = type;
    new_node->name = duplicate_string(name);
    new_node->address = address;
    new_node->next = NULL;

    if (*labels == NULL)
    {
        *labels = new_node;
    }
    else if ((*labels)->next == NULL)
    {
        (*labels)->next = new_node;
    }
    else
    {
        label_t *current = *labels;
        while (true)
        {
            if (current->next == NULL)
            {
                current->next = new_node;
                return;
            }
            current = current->next;
        };
    }
}

/**
 * @brief Adds label of type data
 * 
 * @param line_index Index of current line
 * @param fname Name of current file
 * @param name Name of label
 * @param labels Head of linked list
 * @return int Return 1 if error
 */
int add_data_label(unsigned int line_index, char *fname, char *name,
                   label_t **labels)
{
    label_t **current_node = labels;

    while (*current_node)
    {
        if (!strcmp((*current_node)->name, name))
        {
            add_front(&error_list, line_index, fname, "Illegal label name");
            return 1;
        }
        current_node = &(*current_node)->next;
    }
    if (!is_type(name, LABELN))
    {
        add_front(&error_list, line_index, fname, "Illegal label name");
        return 1;
    }
    add_label(DATAL, name, DC + START, labels);
    return 0;
}

/**
 * @brief Adds label of type extern
 * 
 * @param line_index Index of current line
 * @param fname Name of current file
 * @param name Name of label
 * @param labels Head of linked list
 * @return int Return 1 if error
 */
int add_extern_label(unsigned int line_index, char *fname, char *name,
                     label_t **labels)
{
    label_t **current_node = labels;
    char *label = duplicate_string(name);

    while (*current_node)
    {
        if (!strcmp((*current_node)->name, name))
        {
            add_front(&error_list, line_index, fname, "Illegal label name");
            return 1;
        }
        current_node = &(*current_node)->next;
    }
    if (!is_type(label, LABELN))
    {
        add_front(&error_list, line_index, fname, "Illegal label name");
        return 1;
    }
    add_label(EXTERNL, label, 0, labels);
    return 0;
}

/**
 * @brief Adds label of type code
 * 
 * @param line_index Index of current line
 * @param fname Name of current file
 * @param name Name of label
 * @param labels Head of linked list
 * @return int Return 1 if error
 */
int add_code_label(unsigned int line_index, char *fname, char *name,
                   label_t **labels)
{
    label_t **current_node = labels;

    if (!is_type(name, LABELN))
    {
        add_front(&error_list, line_index, fname, "Illegal label name");
        return 1;
    }
    while (*current_node)
    {
        if (!strcmp((*current_node)->name, name))
        {
            add_front(&error_list, line_index, fname, "Illegal label name");
            return 1;
        }
        current_node = &(*current_node)->next;
    }
    add_label(CODEL, name, IC + START, labels);
    return 0;
}

/**
 * @brief Update each entry label in label table
 * 
 * @param line_index Index of current line
 * @param fname Name of current file
 * @param name Name of label
 * @param labels Head of linked list
 * @return int Return 1 if error
 */
int update_entry(unsigned int line_index, char *fname, char *name,
                 label_t **labels)
{
    label_t **current_node = labels;
    name = strtok(name, " ");
    while (*current_node)
    {
        if (!strcmp((*current_node)->name, name))
        {
            (*current_node)->type = ENTRYL;
            break;
        }
        current_node = &(*current_node)->next;
    }
    if (!(*current_node))
    {
        add_front(&error_list, line_index, fname, "Unkown label");
        return 1;
    }
    name = strtok(NULL, " ");
    if (name)
    {
        add_front(&error_list, line_index, fname,
                  "Extraneous text after end of command");
        return 1;
    }
    return 0;
}


/**
 * @brief Deletes linked lists from memory
 * 
 * @param root Root of linked list
 */
void delete_labels_list(label_t **root)
{
    label_t *curr = *root, *tmp = NULL;

    while (curr != NULL)
    {
        tmp = curr;
        curr = curr->next;
        free(tmp->name);
        free(tmp);
    }

    *root = NULL;
}
