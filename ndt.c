#include <stdio.h>
#include <unistd.h>

int main(int argc, char* args[])
{
  if (argc > 1)
  {
    FILE *file = fopen(args[1], "r");
    if (file != NULL) goto FILE;

    fprintf(stderr, "the path doesn't exist");
    return 1;

    FILE:
      char buffer[256];
      while (fgets(buffer, sizeof(buffer), file) != NULL)
      {
        printf("%s", buffer);
      }

  }

  return 0;
}
