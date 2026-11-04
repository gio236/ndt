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

int main(int argc, char* args[])
{
  if (argc > 1)
  {
    FILE *file = fopen(args[1], "r");
    if (file != NULL) goto FILE;

    fprintf(stderr, "the path doesn't exist");
    return 1;

    FILE:
      char *buffer = NULL;
      size_t len = 0;

      int seenrow = 0, capacity = 10;
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

        printf("%s", rows[seenrow]->buffer);

      
        rows[seenrow]->buffer = strdup(buffer);
        if (!rows[seenrow])
        {
          fprintf(stderr, "strdup went wrong");
        }

        seenrow++;
      }
  }

  return 0;
}
