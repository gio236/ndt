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
#define GOTOYX(row, col) printf("\033[%d;%dH", (row + 1), (col + 1))
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
char *path;

bool stay = true; // for quitting set this to false

// capacity of the pointer struct pointer
int capacity = 10, actualrow = 0;
int numofrows, numofcols;

volatile sig_atomic_t terminalResized = 0;

void handle_sigwinch(int sig) {
    (void)sig;
    terminalResized = 1;
}

//funzioni momentanee saranno poi da rivedere quando iniziero ad implementare la parte della scrittura
//perche attualmente andrei senza controlli a scrivere in aree della memoria che nn esistono, skippando spazi di righe

void movLeft(void) { cur.x--; cur.x = CLAMP(cur.x, 0, numofcols); GOTOYX(cur.y, cur.x); }
void movRight(void) { cur.x++; cur.x = CLAMP(cur.x, 0, numofcols); GOTOYX(cur.y, cur.x); }
void movUp(void) { cur.y--; cur.y = CLAMP(cur.y, 0, numofrows); GOTOYX(cur.y, cur.x); }
void movDown(void) { cur.y++; cur.y = CLAMP(cur.y, 0, numofrows); GOTOYX(cur.y, cur.x); }

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

  raw.c_cc[VMIN] = 1;  // Legge almeno 1 carattere prima di sbloccare read()
  raw.c_cc[VTIME] = 0; // Nessun timeout di attesa

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void printFile(void)
{

  for(int i = 0; i < numofrows - 1 && i < actualrow; i++)
  {

    if(rows[i]->len > numofcols)
    {
      printf("%.*s\r", numofcols, rows[i]->buffer); // il cursore shifta in automatico per il \n
    }
    else
    {
      printf("%s\r",rows[i]->buffer);
    }
  }
}

int editorSaveFile()
{
  FILE *file = fopen(path, "w");
  if (file == NULL)
  {
    fprintf(stderr, "error while opening file");
    return 1;
  }
  for(int i = 0; i < actualrow; i++)
  {
    fputs(rows[i]->buffer, file);
  }
  fclose(file);
}

int editorOpenFile()
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

int insertChar(char c)
{
  if(cur.y < actualrow && cur.x < rows[cur.y]->len) /* da aggiungiere shifted row e shifted col **TODO */
  {
    char *temp = realloc(rows[cur.y]->buffer, rows[cur.y]->len + 2); // add a char
    if (!temp)
    {
      fprintf(stderr, "realloc went wrong while inserting new char");
      return 1;
    }

    rows[cur.y]->buffer = temp;

    // **TODO usare shiftrows e shiftcol
    memmove(&(rows[cur.y]->buffer)[cur.x + 1], &(rows[cur.y]->buffer)[cur.x], rows[cur.y]->len - cur.x + 1);

    rows[cur.y]->buffer[cur.x] = c;
    rows[cur.y]->len += 1;

    movRight();

    return 0;
  }

  return -1;
}

void handleInput(char c)
{
  if (iscntrl(c))
  {

    if (c == 24) { stay = false; }
    if (c == 23) { editorSaveFile(); }

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
      }
    }

  }
  else
  {
    //switch (insertChar(c)) { case 1: printf("errore"); break; case 0: printf("inserito"); break; case -1: printf("non inserito"); break; }

    if(!insertChar(c))
    {
      CLEAR;
      printFile();
      GOTOYX(cur.y, cur.x);
    }
  }
}

int main(int argc, char *args[])
{
  if (argc > 1)
  {
    path = args[1];

    signal(SIGWINCH, handle_sigwinch);

    setvbuf(stdout, NULL, _IONBF, 0);  // disattiva il buffering del stdout
    fflush(stdout);

    enableRawMode();
    if(getRowsCols()) return 1;
    editorOpenFile();


    CLEAR;
    printFile();

    GOTOYX(cur.y, cur.x);

    char c;
    while(read(STDIN_FILENO, &c, 1) == 1)
    {
      handleInput(c);
      if (!stay) break;

      if (terminalResized)
      {
        terminalResized = 0;
        if(getRowsCols()) return 1;
        CLEAR;
        printFile();
      }

      //printf("cur.y ==> %d, cur.x ==> %d", cur.y, cur.x);

    }

    for(int i = 0; i < capacity; i++) { free(rows[i]); } free(rows);
  }

  return 0;
}
