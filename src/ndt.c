#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <signal.h>

/*
• \033[N A: Muove il cursore Su di N righe.
• \033[N B: Muove il cursore Giù di N righe.
• \033[N C: Muove il cursore a Destra di N colonne.
• \033[N D: Muove il cursore a Sinistra di N colonne.
*/

#define CLEAR printf("\033[H\033[2J")
#define GOTOYX(row, col) printf("\033[%d;%dH", (row), (col))
#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

typedef struct
{
  char *buffer;
  size_t len;
} row;

typedef struct
{
  int x;
  int y;
} cursor;

row **rows;
cursor cur = { .x = 0, .y = 0 };

int capacity = 10, actualrow = 0;
int numofrows, numofcols;

volatile sig_atomic_t terminalResized = 0;

void handle_sigwinch(int sig) {
    (void)sig;
    terminalResized = 1;
}

//funzioni momentanee saranno poi da rivedere quando iniziero ad implementare la parte della scrittura
//perche attualmente andrei senza controlli a scrivere in aree della memoria che nn esistono, skippando spazi di righe

void movLeft(void) { cur.x--; cur.x = CLAMP(cur.x, 1, numofcols); GOTOYX(cur.y, cur.x); }
void movRight(void) { cur.x++; cur.x = CLAMP(cur.x, 1, numofcols); GOTOYX(cur.y, cur.x); }
void movUp(void) { cur.y--; cur.y = CLAMP(cur.y, 1, numofrows); GOTOYX(cur.y, cur.x); }
void movDown(void) { cur.y++; cur.y = CLAMP(cur.y, 1, numofrows); GOTOYX(cur.y, cur.x); }

int getRowsCols(void)
{
  struct winsize w;
  if (!ioctl(STDOUT_FILENO, TIOCGWINSZ, &w))
  {
    numofcols = w.ws_col;
    numofrows = w.ws_row;
  }else
  {
    fprintf(stderr, "error while retriving terminal's cols and rows");
    return 1;
  }

  return 0;
}

struct termios origTermios; // original termios struct, to restore before exiting ndt

void disableRawMode()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios);
}

void enableRawMode() 
{
  tcgetattr(STDIN_FILENO, &origTermios);
  struct termios raw = origTermios;

  atexit(disableRawMode);

  raw.c_iflag &= ~(IXON | ICRNL);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_oflag &= ~(OPOST);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void printFile(void)
{
  for(int i = 0; i < numofrows && i < actualrow; i++)
  {
    if(rows[i]->len > numofcols)
    {
      printf("%.*s\r", numofcols, rows[i]->buffer);
    }
    else
    {
      printf("%s\r",rows[i]->buffer);
    }
  }
}

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

    actualrow++;

    fprintf(file, rows[0]->buffer);
  }
  else
  {
    char *buffer = NULL;
    size_t len = 0;

    rows = malloc(capacity * sizeof(row *));

    while (getline(&buffer, &len, file) != -1)
    {
      if (actualrow >= capacity)
      {
        capacity *= 2;
        row **temp = realloc(rows, capacity * sizeof(row *));
        if (!temp)
        {
          fprintf(stderr, "realloc went wrong");
        }

        rows = temp;
      }

      rows[actualrow] = malloc(sizeof(row));
      rows[actualrow]->buffer = strdup(buffer);
      rows[actualrow]->len = strlen(buffer);

      //printf("%s", rows[actualrow]->buffer);

      if (!rows[actualrow]) fprintf(stderr, "strdup went wrong");

      actualrow++;
    }

    return 0;
  }

  fclose(file);
}

void handleInput(char c)
{
  if (iscntrl(c))
  {
    if (c == 24) { /* exit ctrl-x */ }
    if (c == 23) { /* save ctrl-w */ }

    if (c == 27) // esc
    {
      char seq[2];
      // Leggiamo i successivi due caratteri della sequenza se presenti
      if (read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1)
      {
        if (seq[0] == '[')
        {
          switch (seq[1])
          {
            case 'A': movUp(); break;
            case 'B': movDown(); break;
            case 'C': movRight(); break;
            case 'D': movLeft(); break;
          }
        }
      }
      else 
      {
        //printf("Tasto: Solo ESC\r\n");
      }
    }

  }
  else
  {
    //printf("qua stampabile");
  }
}

int main(int argc, char* args[])
{
  if (argc > 1)
  {
    signal(SIGWINCH, handle_sigwinch);

    setvbuf(stdout, NULL, _IONBF, 0);  // disattiva il buffering del stdout
    fflush(stdout);

    enableRawMode();
    if(getRowsCols()) return 1;
    editorOpenFile(args[1]);

    CLEAR;
    printFile();

    GOTOYX(cur.y, cur.x);

    char c;
    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
    {
      handleInput(c);

      if (terminalResized)
      {
        terminalResized = 0;
        if(getRowsCols()) return 1;
        CLEAR;
        printFile();
      }
    }

    for(int i = 0; i < capacity; i++) { free(rows[i]); } free(rows);
  }

  return 0;
}
