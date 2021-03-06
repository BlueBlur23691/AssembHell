#include "error.h"
#include "files.h"
#include "memory.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE 256
extern int DC, IC;
extern err_node_t *error_list;
label_t *labels = NULL;

void first_cycle(FILE *file, int *error, char *name, unsigned int *line_index) {
  int label_flag = 0;
  char line_s[LINE_SIZE];
  char *cmd = NULL, *label = NULL, *sof_label = NULL, *args = NULL;

  while (fgets(line_s, sizeof(line_s), file)) {
    ++*line_index;
    /* Comment or empty line */
    if (line_s[0] != ';' && line_s[0] != '\n') {
      /*
       *  CMD=command
       *  args=Arguments
       */
      cmd = strtok(line_s, " ");
      args = strtok(NULL, "\n");
      label_flag = 0;
      label = "";
      /* Initializtion of a label */
      if (is_type(cmd, LABEL)) {
        label_flag = 1;
        /* Gets rid of ':' */
        cmd[strlen(cmd) - 1] = '\0';
        /*
         *  Label name stored in label
         *  sof_label points to start of label in order to free memory
         *  cmd and args getting the correct values
         */
        label = duplicate_string(cmd);
        sof_label = label;
        cmd = strtok(args, " ");
        args = strtok(NULL, "\n");
      }
      /*Storing data*/
      if (is_type(cmd, DATA)) {
        /* Initializtion of a label */
        if (label_flag)
          *error += add_data_label(*line_index, name, label, &labels);
        /* Adds data words to memory */
        *error += update_data(cmd, args, data, *line_index, name);
        continue;
      }
      /* Add extern label */
      else if (is_type(cmd, EXTERN)) {
        /* Adds extern label to memory */
        *error += add_extern_label(*line_index, name, args, &labels);
        continue;
      }
      /* Line of code */
      else if (is_type(cmd, CODE)) {
        /* Initializtion of a label */
        if (label_flag)
          *error += add_code_label(*line_index, name, label, &labels);
        /* Adds code words to memory */
        *error += update_code(0, cmd, args, *line_index, name, code, &labels);
      } else if (!is_type(cmd, ENTRY)) {
        add_front(&error_list, *line_index, name, "Unkown command");
        ++(*error);
      }

      free(sof_label);
      sof_label = NULL;
    }
  }
}

void second_cycle(FILE *file, int *error, char *name,
                  unsigned int *line_index) {
  char line_s[LINE_SIZE];
  char *cmd = NULL, *args = NULL;
  while (fgets(line_s, sizeof(line_s), file)) {
    ++*line_index;
    /* Comment or empty line */
    if (line_s[0] != ';' && line_s[0] != '\n') {
      /*
       *  CMD=command
       *  args=Arguments
       */
      cmd = strtok(line_s, " ");
      args = strtok(NULL, "\n");
      /* Ignores data types and extern labels */
      if (is_type(cmd, DATA) || is_type(cmd, EXTERN))
        continue;
      /* Ignores label initializtions */
      if (is_type(cmd, LABEL)) {
        cmd = strtok(args, " ");
        args = strtok(NULL, "\n");
      }
      /* Update labels as entry */
      if (is_type(cmd, ENTRY)) {
        *error += update_entry(*line_index, name, args, &labels);
        continue;
      }
      /* Add label operands to memory */
      if (is_type(cmd, CODE)) {
        *error += update_code(1, cmd, args, *line_index, name, code, &labels);
      }
    }
  }
}

int main(int argc, char *argv[]) {
  FILE *file = NULL;
  char *name = NULL;
  unsigned int line_index = 0;
  int i, error;
  label_t **point = NULL;
  error_list = NULL;
  ext = NULL;

  if (argc < 2) {
    printf("No input file.\nUsage: %s <asm files>\n", argv[0]);
  }

  /*Itirates through files*/
  for (i = 1; i < argc; ++i) {
    /* Initializtions */
    point = &labels;
    name = argv[i];
    error = 0;
    DC = 0;
    IC = 0;
    file = fopen(name, "r");

    if (file == NULL) {
      print_message("Error opening file",1);
      continue;
    }

    /* First cycle */
    first_cycle(file, &error, name, &line_index);

    fclose(file);
    if (IC + DC > MEMORY_SIZE) {
      add_front(&error_list, line_index, name,
                "Program exceeds memory");
      error += 1;
    }
    /* Error during first cycle */
    if (error) {
      print_message("First Cycle\nSee errors.txt",1);
      create_error_file(error_list);
      continue;
    }
    /*
     * Adds number of code words to data counter of data labels
     * in order to store the data labels after code
     */
    while (*point) {
      if ((*point)->type == DATAL)
        (*point)->address += IC;
      point = &(*point)->next;
    }

    IC = 0;
    file = fopen(name, "r");
    if (file == NULL) {
      print_message("Error opening file",1);
      continue;
    }
    line_index = 0;

    /* Second cycle */
    second_cycle(file, &error, name, &line_index);

    /* Error during second cycle */
    if (error) {
      print_message("Second Cycle\nSee errors.txt",1);
      create_error_file(error_list);
      continue;
    }

    /* name = name of file */
    name = strtok(name, ".");
    /* Create output files */
    create_files(code, data, labels, argv[i]);
    fclose(file);
    delete_labels_list(&ext);
    delete_labels_list(&labels);
    print_message(argv[i],0);
  }

  /* Free stored memory */
  delete_errors_list(&error_list);

  return 0;
}
