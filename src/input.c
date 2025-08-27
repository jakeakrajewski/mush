#include "input.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **lines = NULL;
size_t lines_count = 0;
size_t lines_capacity = 0;

int is_partial_input(const char *str) {
  size_t len = strlen(str);
  if (len && str[len - 1] == '\n')
    len--;
  return len > 0 && str[len - 1] == '\\';
}

void remove_trailing_backslash(char *str) {
  size_t len = strlen(str);
  if (len && str[len - 1] == '\n')
    str[--len] = '\0';
  if (len && str[len - 1] == '\\')
    str[len - 1] = '\0';
}

void add_line(const char *input) {
  if (lines_count >= lines_capacity) {
    lines_capacity = lines_capacity ? lines_capacity * 2 : 4;
    lines = realloc(lines, lines_capacity * sizeof(char *));
    if (!lines) {
      perror("realloc");
      exit(1);
    }
  }
  lines[lines_count++] = strdup(input);
}

void free_lines(void) {
  for (size_t i = 0; i < lines_count; ++i)
    free(lines[i]);
  free(lines);
  lines = NULL;
  lines_count = 0;
  lines_capacity = 0;
}

char *join_lines() {
  size_t total_len = 1;
  for (size_t i = 0; i < lines_count; i++)
    total_len += strlen(lines[i]);
  char *result = malloc(total_len);
  if (!result)
    return NULL;
  result[0] = '\0';
  for (size_t i = 0; i < lines_count; i++)
    strcat(result, lines[i]);
  return result;
}

char *mu_read_line(void) {
  char *line = NULL;
  size_t bufsize = 0;
  if (getline(&line, &bufsize, stdin) == -1) {
    if (feof(stdin))
      exit(EXIT_SUCCESS);
    perror("readline");
    exit(EXIT_FAILURE);
  }
  return line;
}

char *trim_whitespace(char *str) {
  while (isspace(*str))
    str++;
  if (*str == 0)
    return str;
  char *end = str + strlen(str) - 1;
  while (end > str && isspace(*end))
    end--;
  *(end + 1) = '\0';
  return str;
}
