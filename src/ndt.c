#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct
{
  char *buffer;
  size_t len;
} row;

row **rows;
int capacity = 1;

int editorOpenFile(char* path)
{
  FILE *file = fopen(path, "r");

  if (file == NULL)
  {
    file = fopen(path, "w");
    if (file == NULL) fprintf(stderr, "error while creating file");

    rows = malloc(capacity * sizeof(row *));

    rows[0] = malloc(sizeof(row));
    rows[0]->buffer = strdup("thank you to use ntd");
    rows[0]->len = strlen(rows[0]->buffer);

    fprintf(file, rows[0]->buffer);

    return 1;

  }
  else
  {

    char *buffer = NULL;
    size_t len = 0;

    int seenrow = 0;
    rows = malloc(capacity * sizeof(row *));

    while (getline(&buffer, &len, file) != -1)
    {
      if (seenrow >= capacity)
      {
        capacity *= 2;
        row **temp = realloc(rows, capacity * sizeof(row *));
        if (!temp)
        {
          fprintf(stderr, "realloc went wrong");
        }

        rows = temp;
      }

      rows[seenrow] = malloc(sizeof(row));
      rows[seenrow]->buffer = strdup(buffer);
      rows[seenrow]->len = strlen(buffer);

      //printf("%s", rows[seenrow]->buffer);


      rows[seenrow]->buffer = strdup(buffer);
      if (!rows[seenrow]) fprintf(stderr, "strdup went wrong");

      seenrow++;
    }

    return 0;
  }

  fclose(file);
}

int main(int argc, char* args[])
{
  if (argc > 1)
  {
    editorOpenFile(args[1]);
    for(int i = 0; i < capacity; i++) { free(rows[i]); } free(rows);
  }

  return 0;
}
