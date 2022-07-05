/*~~~~~~~~~~~~~~~~~~~~ version ~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#define TI_VERSION "0.0.1"

/*~~~~~~~~~~~~~~~~~~~~ includes ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <asm-generic/ioctls.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/*~~~~~~~~~~~~~~~~~~~~ defines ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#define TI_TAB_STOP 8
#define TI_QUIT_TIMES 1

#define CTRL_KEY(key) ((key)&0x1f)

enum editorKey {

  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN,
  WORD_NEXT,
  WORD_LAST

};

enum editorHighlight {

  HL_NORMAL = 0,
  HL_COMMENT,
  HL_KEYWORD1,
  HL_KEYWORD2,
  HL_STRING,
  HL_NUMBER,
  HL_MATCH

};

#define HL_HIGHLIGHT_NUMBERS (1<<10)
#define HL_HIGHLIGHT_STRINGS (1<<1)

/*~~~~~~~~~~~~~~~~~~~~ data ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct editorSyntax {

  char *filetype;
  char **filematch;
  char **keywords;
  char *single_line_comment_start;
  int flags;

};

typedef struct erow {

  int size;
  int rsize;
  char *chars;
  char *render;
  unsigned char *hl;

} erow;

struct editorConfig {

  int cx, cy;
  int rx;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  int numrows;
  erow *row;
  int dirty;
  int modal;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  struct editorSyntax *syntax;
  struct termios orig_termios;
};

struct editorConfig E;

/*~~~~~~~~~~~~~~~~~~~~ filetypes ~~~~~~~~~~~~~~~~~~~*/

char *C_HL_extensions[] = { ".c", ".h", ".cpp", NULL};
char *C_HL_keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",

  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};

struct editorSyntax HLDB[] = {
  {
    "c",
    C_HL_extensions,
    C_HL_keywords,
    "//",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/*~~~~~~~~~~~~~~~~~~~~ function prototypes ~~~~~~~~~~~~~~~~~~~*/

void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char *editorPrompt(char *prompt, void (*callback)(char *, int));

/*~~~~~~~~~~~~~~~~~~~~ terminal ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
    die("tcgetattr");
  atexit(disableRawMode);

  struct termios raw = E.orig_termios;

  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= ~(CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

int editorReadKey() {

  int nread;
  char c;

  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }

  if (c == '\x1b') {
    char seq[3];

    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
      return '\x1b';

    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1)
          return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
          case '1':
            return HOME_KEY;
          case '3':
            return DEL_KEY;
          case '4':
            return END_KEY;
          case '5':
            return PAGE_UP;
          case '6':
            return PAGE_DOWN;
          case '7':
            return HOME_KEY;
          case '8':
            return END_KEY;
          }
        }
      } else {
        switch (seq[1]) {
        case 'A':
          return ARROW_UP;
        case 'B':
          return ARROW_DOWN;
        case 'C':
          return ARROW_RIGHT;
        case 'D':
          return ARROW_LEFT;
        case 'H':
          return HOME_KEY;
        case 'F':
          return END_KEY;
        }
      }
    } else if (seq[0] == '0') {
      switch (seq[1]) {
      case 'H':
        return HOME_KEY;
      case 'F':
        return END_KEY;
      }
    }

    return '\x1b';
  } else {
    return c;
  }
}

int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return -1;
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1)
      break;
    if (buf[i] == 'R')
      break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[')
    return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
    return -1;
  return -1;
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      return -1;
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*~~~~~~~~~~~~~~~~~~~~ syntax highlighting ~~~~~~~~~~~~~~~~~~~~*/

int is_seperator(int c) {
  return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void editorUpdateSyntax(erow *row) {
  row->hl = realloc(row->hl, row->rsize);
  memset(row->hl, HL_NORMAL, row->size);
  
  if (E.syntax == NULL) return;
  
  char **keywords = E.syntax->keywords;
  
  char *scs = E.syntax->single_line_comment_start;
  int scs_len = scs ? strlen(scs) : 0;

  int prev_sep = 1;
  int in_string = 0;
  
  int i = 0;
  while (i < row->rsize) {
    char c = row->render[i];
    unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;
    
    if (scs_len && !in_string) {
      if (!strncmp(&row->render[i], scs, scs_len)) {
        memset(&row->hl[i], HL_COMMENT, row->size - i);
        break;
      }
    }

    if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
      if (in_string) {
        row->hl[i] = HL_STRING;
        if (c == '\\' && i + 1 < row->rsize) {
          row->hl[i + 1] = HL_STRING;
          i += 2;
          continue;
        }
        if (c == in_string) in_string = 0;
        i++;
        prev_sep = 1;
        continue;
      } else {
        if (c == '"' || c == '\'') {
          in_string = c;
          row->hl[i] = HL_STRING;
          i++;
          continue;
        }
      }
    }
    
    if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
      if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) || 
          (c == '.' && prev_hl == HL_NUMBER)) {
        row->hl[i] = HL_NUMBER;
        i++;
        prev_sep = 0;
        continue;
      }
    }
    
    if (prev_sep) {
      int j;
      for (j = 0; keywords[j]; ++j) {
        int klen = strlen(keywords[j]);
        int kw2 = keywords[j][klen - 1] == '|';
        if (kw2) klen--;
        
        if (!strncmp(&row->render[i], keywords[j], klen) &&
            is_seperator(row->render[i + klen])) {
          memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
          i += klen;
          break;
        }
      }
      if (keywords[j] != NULL) {
        prev_sep = 0;
        continue;
      }
    }

    prev_sep = is_seperator(c);
    i++;
  }
}

int editorSyntaxToColor(int hl) {

  switch (hl) {
  case HL_COMMENT:
    return 36;
    
  case HL_KEYWORD1:
    return 33;

  case HL_KEYWORD2:
    return 32;

  case HL_STRING:
    return 35;

  case HL_NUMBER:
    return 31;

  case HL_MATCH:
    return 34;

  default:
    return 37;
  }
}

void editorSelectSyntaxHighlighting() {
  E.syntax = NULL;
  if (E.filename == NULL) return;
  
  char *ext = strchr(E.filename, '.');
  
  for (unsigned int j = 0; j < HLDB_ENTRIES; ++j) {
    struct editorSyntax *s = &HLDB[j];
    unsigned int i = 0;
    while (s->filematch[i]) {
      int is_ext = (s->filematch[i][0] == '.');
      if((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
          (!is_ext && strstr(E.filename, s->filematch[i]))) {
        E.syntax = s;
        
        int filerow;
        for (filerow = 0; filerow < E.numrows; ++filerow) {
          editorUpdateSyntax(&E.row[filerow]);
        }
        
        return;
      }
      
      i++;
    }
  }
}

/*~~~~~~~~~~~~~~~~~~~~ row operations ~~~~~~~~~~~~~~~~~~~~~~~~~*/

int editorRowCxToRx(erow *row, int cx) {
  int rx = 0;
  int j;
  for (j = 0; j < cx; ++j) {
    if (row->chars[j] == '\t')
      rx += (TI_TAB_STOP - 1) - (rx % TI_TAB_STOP);
    rx++;
  }

  return rx;
}

int editorRowRxToCx(erow *row, int rx) {
  int cur_rx = 0;
  int cx;
  for (cx = 0; cx < row->size; ++cx) {
    if (row->chars[cx] == '\t')
      cur_rx += (TI_TAB_STOP - 1) - (cur_rx % TI_TAB_STOP);
    cur_rx++;
    if (cur_rx > rx)
      return cx;
  }

  return cx;
}

void editorUpdateRow(erow *row) {
  int tabs = 0;
  int j;

  for (j = 0; j < row->size; ++j)
    if (row->chars[j] == '\t')
      tabs++;
  free(row->render);

  row->render = malloc(row->size + tabs * (TI_TAB_STOP - 1) + 1);

  int idx = 0;
  for (j = 0; j < row->size; ++j) {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while (idx % TI_TAB_STOP != 0)
        row->render[idx++] = ' ';
    } else {
      row->render[idx++] = row->chars[j];
    }
  }

  row->render[idx] = '\0';
  row->rsize = idx;
  editorUpdateSyntax(row);
}

void editorInsertRow(int at, char *s, size_t len) {
  if (at < 0 || at > E.numrows)
    return;
  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';
  E.row[at].rsize = 0;
  E.row[at].render = NULL;
  E.row[at].hl = NULL;
  editorUpdateRow(&E.row[at]);
  E.numrows++;
  E.dirty++;
}

void editorFreeRow(erow *row) {
  free(row->render);
  free(row->chars);
  free(row->hl);
}

void editorDelRow(int at) {
  if (at < 0 || at >= E.numrows)
    return;
  editorFreeRow(&E.row[at]);
  memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
  E.numrows--;
  E.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c) {
  if (at < 0 || at > row->size)
    at = row->size;

  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowDelChar(erow *row, int at) {
  if (at < 0 || at >= row->size)
    return;

  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  editorUpdateRow(row);
  E.dirty++;
}

/*~~~~~~~~~~~~~~~~~~~~ editor operations ~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void editorInsertChar(int c) {
  if (E.cy == E.numrows)
    editorInsertRow(E.numrows, "", 0);

  editorRowInsertChar(&E.row[E.cy], E.cx, c);
  E.cx++;
}

void editorInsertNewline() {
  if (E.cx == 0) {
    editorInsertRow(E.cy, "", 0);
  } else {
    erow *row = &E.row[E.cy];
    editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
  }

  E.cy++;
  E.cx = 0;
}

void editorDelChar() {
  if (E.cy == E.numrows)
    return;
  if (E.cx == 0 && E.cy == 0)
    return;

  erow *row = &E.row[E.cy];
  if (E.cx > 0) {
    editorRowDelChar(row, E.cx - 1);
    E.cx--;
  } else {
    E.cx = E.row[E.cy - 1].size;
    editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
    editorDelRow(E.cy);
    E.cy--;
  }
}

/*~~~~~~~~~~~~~~~~~~~~ file I/O ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

char *editorRowsToString(int *buflen) {
  int total_len = 0;
  int j;
  for (j = 0; j < E.numrows; ++j)
    total_len += E.row[j].size + 1;

  *buflen = total_len;
  char *buf = malloc(total_len);
  char *p = buf;
  for (j = 0; j < E.numrows; ++j) {
    memcpy(p, E.row[j].chars, E.row[j].size);
    p += E.row[j].size;
    *p = '\n';
    p++;
  }

  return buf;
}

void editorOpen(char *filename) {
  free(E.filename);
  E.filename = strdup(filename);
  
  editorSelectSyntaxHighlighting();

  FILE *fp = fopen(filename, "r");
  if (!fp)
    die("fopen");

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    line[(linelen = strcspn(line, "\r\n"))] = 0;

    editorInsertRow(E.numrows, line, linelen);
  }

  free(line);
  fclose(fp);
  E.dirty = 0;
}

void editorSave() {

  if (E.filename == NULL) {
    E.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
    if (E.filename == NULL) {
      editorSetStatusMessage("Save aborted");
      return;
    }

    editorSelectSyntaxHighlighting();
  }

  int len;
  char *buf = editorRowsToString(&len);
  int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
        editorSetStatusMessage("%d bytes written to disk", len);
        E.dirty = 0;
        return;
      }
    }

    close(fd);
  }

  free(buf);
  editorSetStatusMessage("Failed write to disk! I/O error: %s",
                         strerror(errno));
}

/*~~~~~~~~~~~~~~~~~~~~ find / search ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void editorSearchCallback(char *query, int key) {
  static int last_match = -1;
  static int direction = 1;
  static int saved_hl_line;
  static char *saved_hl = NULL;
  if (saved_hl) {
    memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
    free(saved_hl);
    saved_hl = NULL;
  }

  if (key == '\r' || key == '\x1b') {
    last_match = -1;
    direction = 1;
    return;
  } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
    direction = 1;
  } else if (key == ARROW_LEFT || key == ARROW_UP) {
    direction = -1;
  } else {
    last_match = -1;
    direction = 1;
  }

  if (last_match == -1)
    direction = 1;

  int current = last_match;
  int i;
  for (i = 0; i < E.numrows; ++i) {
    current += direction;

    if (current == -1)
      current = E.numrows - 1;
    else if (current == E.numrows)
      current = 0;

    erow *row = &E.row[current];
    char *match = strstr(row->render, query);
    if (match) {
      last_match = current;
      E.cy = current;
      E.cx = editorRowRxToCx(row, match - row->render);
      E.rowoff = E.numrows;
      saved_hl_line = current;
      saved_hl = malloc(row->rsize);
      memcpy(saved_hl, row->hl, row->rsize);
      memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
      break;
    }
  }
}

void editorSearch() {
  int saved_cx = E.cx;
  int saved_cy = E.cy;
  int saved_coloff = E.coloff;
  int saved_rowoff = E.rowoff;
  char *query =
      editorPrompt("Search: %s (ESC/Arrows/Enter)", editorSearchCallback);

  if (query) {
    free(query);
  } else {
    E.cx = saved_cx;
    E.cy = saved_cy;
    E.coloff = saved_coloff;
    E.rowoff = saved_rowoff;
  }
}

/*~~~~~~~~~~~~~~~~~~~~ append buffer ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT { NULL, 0 }

void abAppend(struct abuf *ab, const char *s, int len) {
  char *app = realloc(ab->b, ab->len + len);
  if (app == NULL)
    return;

  memcpy(&app[ab->len], s, len);
  ab->b = app;
  ab->len += len;
}

void abFree(struct abuf *ab) { free(ab->b); }

/*~~~~~~~~~~~~~~~~~~~~ output ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void editorScroll() {
  E.rx = 0;
  if (E.cy < E.numrows) {

    E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
  }

  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }

  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }

  if (E.rx < E.coloff) {
    E.coloff = E.rx;
  }

  if (E.rx >= E.coloff + E.screencols) {
    E.coloff = E.rx - E.screencols + 1;
  }
}

void editorDrawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < E.screenrows; ++y) {
    int filerow = y + E.rowoff;
    if (filerow >= E.numrows) {
      if (E.numrows == 0 && y == 0) {
        char welcome[80];
        int welcomelen =
            snprintf(welcome, sizeof(welcome), "Ti -- version %s", TI_VERSION);

        if (welcomelen > E.screencols)
          welcomelen = E.screencols;

        int padding = (E.screencols - welcomelen) / 2;
        if (padding) {
          abAppend(ab, "~", 1);
          padding--;
        }
        while (padding--)
          abAppend(ab, " ", 1);

        abAppend(ab, welcome, welcomelen);
      } else {
        abAppend(ab, "~", 1);
      }
    } else {
      int len = E.row[filerow].rsize - E.coloff;
      if (len < 0)
        len = 0;

      if (len > E.screencols)
        len = E.screencols;

      char *c = &E.row[filerow].render[E.coloff];
      unsigned char *hl = &E.row[filerow].hl[E.coloff];
      int current_color = -1;
      int j;
      for (j = 0; j < len; ++j) {
        if (iscntrl(c[j])) {
          char sym = (c[j] <= 26) ? '@' + c[j] : '?';
          abAppend(ab, "\x1b[7m", 4);
          abAppend(ab, &sym, 1);
          abAppend(ab, "\x1b[m", 3);
          if (current_color != -1) {
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
            abAppend(ab, buf, clen);
          }
        } else if (hl[j] == HL_NORMAL) {
          if (current_color != -1) {
            abAppend(ab, "\x1b[39m", 5);
            current_color = -1;
          }
          abAppend(ab, &c[j], 1);
        } else {
          int color = editorSyntaxToColor(hl[j]);
          if (color != current_color) {
            current_color = color;
            char buf[16];
            int colorlen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
            abAppend(ab, buf, colorlen);
          }

          abAppend(ab, &c[j], 1);
        }
      }

      abAppend(ab, "\x1b[39m", 5);
    }

    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
  }
}

void editorDrawStatusBar(struct abuf *ab) {
  abAppend(ab, "\x1b[7m", 4);
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "%.20s - %d Lines %s",
                     E.filename ? E.filename : "[SCRATCH]", E.numrows,
                     E.dirty ? "(+)" : "");
  int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
    E.syntax ? E.syntax->filetype : "no filetype detected", E.cy + 1, E.numrows);
  if (E.cy + 1 > E.numrows) {
    rlen = snprintf(rstatus, sizeof(rstatus), "L %s", "EOF");
  } else {
    float perc = ((float)E.cy + 1) / ((float)E.numrows) * 100;
    rlen = snprintf(rstatus, sizeof(rstatus), "L %d:%d %.0f%%",
                    E.cy + 1 >= E.numrows ? E.numrows : E.cy + 1, E.cx + 1,
                    perc > 0 || perc <= 100 ? perc : 0);
  }

  if (len > E.screencols)
    len = E.screencols;

  abAppend(ab, status, len);
  while (len < E.screencols) {
    if (E.screencols - len == rlen) {
      abAppend(ab, rstatus, rlen);
      break;
    } else {
      abAppend(ab, " ", 1);
      len++;
    }
  }

  abAppend(ab, "\x1b[m", 3);
  abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab) {
  abAppend(ab, "\x1b[K", 3);
  int msglen = strlen(E.statusmsg);
  if (msglen > E.screencols)
    msglen = E.screencols;

  if (msglen && time(NULL) - E.statusmsg_time < 5)
    abAppend(ab, E.statusmsg, msglen);
}

void editorRefreshScreen() {
  editorScroll();
  struct abuf ab = ABUF_INIT;
  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);
  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,
           (E.rx - E.coloff) + 1);
  abAppend(&ab, buf, strlen(buf));
  abAppend(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
  va_end(ap);
  E.statusmsg_time = time(NULL);
}

/*~~~~~~~~~~~~~~~~~~~~ input ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

char *editorPrompt(char *prompt, void (*callback)(char *, int)) {
  size_t bufsize = 128;
  char *buf = malloc(bufsize);
  size_t buflen = 0;
  buf[0] = '\0';
  while (1) {
    editorSetStatusMessage(prompt, buf);
    editorRefreshScreen();
    int c = editorReadKey();
    if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
      if (buflen != 0)
        buf[--buflen] = '\0';
    } else if (c == '\x1b') {
      editorSetStatusMessage("");
      if (callback)
        callback(buf, c);
      free(buf);
      return NULL;
    } else if (c == '\r') {
      if (buflen != 0) {
        editorSetStatusMessage("");
        if (callback)
          callback(buf, c);
        return buf;
      }

      return NULL;
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }

      buf[buflen++] = c;
      buf[buflen] = '\0';
    }

    if (callback)
      callback(buf, c);
  }
}

void editorMoveCursor(int key) {
  erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  switch (key) {
  case ARROW_LEFT:
    if (E.cx != 0) {
      E.cx--;
    } else if (E.cy > 0) {
      E.cy--;
      E.cx = E.row[E.cy].size;
    }
    break;
  case ARROW_RIGHT:
    if (row && E.cx < row->size) {
      E.cx++;
    } else if (row && E.cx == row->size) {
      E.cy++;
      E.cx = 0;
    }
    break;
  case WORD_NEXT:
    if (row && E.cx < row->size) {
      while (row && E.cx < row->size && row->render[E.cx] != ' ' &&
             row->render[E.cx] != TI_TAB_STOP) {
        E.cx++;
      }
      while ((row && E.cx < row->size && row->render[E.cx] == ' ') ||
             (row && E.cx < row->size && row->render[E.cx] == TI_TAB_STOP)) {
        E.cx++;
      }
    } else if (row && E.cx == row->size) {
      E.cy++;
      E.cx = 0;
    }
    break;
  case WORD_LAST:
    if (E.cx != 0) {
      while (E.cx != 0 && row->render[E.cx] != ' ' &&
             row->render[E.cx] != TI_TAB_STOP) {
        E.cx--;
      }
      while ((E.cx != 0 && row->render[E.cx] == ' ') ||
             (E.cx != 0 && row->render[E.cx] == TI_TAB_STOP)) {
        E.cx--;
      }
    } else if (E.cy > 0) {
      E.cy--;
      E.cx = E.row[E.cy].size;
    }
    break;
  case ARROW_UP:
    if (E.cy != 0) {
      E.cy--;
    }
    break;
  case ARROW_DOWN:
    if (E.cy < E.numrows) {
      E.cy++;
    }
    break;
  case END_KEY:
    if (row && E.cx < row->size)
      E.cx = E.screencols - 1;
    break;
  case HOME_KEY:
    E.cx = 0;
    E.coloff = 0;
    break;
  }

  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  int rowlen = row ? row->size : 0;
  if (E.cx > rowlen) {
    E.cx = rowlen;
  }
}

void editorExit() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  exit(0);
  return;
}

void editorProcessKeypress() {
  static int quit_times = TI_QUIT_TIMES;
  int c = editorReadKey();
  switch (c) {
  case '\r':
    if (quit_times == 0) {
      editorExit();
      break;
    }
    editorInsertNewline();
    break;
  case CTRL_KEY('q'):
    if (E.dirty) {
      editorSetStatusMessage("!UNSAVED CHANGES! Press <ENTER> to confirm");
      if (quit_times)
        quit_times--;
      return;
    }
    editorExit();
    break;
  case CTRL_KEY('s'):
    editorSave();
    break;
  case HOME_KEY:
    editorMoveCursor(c);
    break;
  case END_KEY:
    editorMoveCursor(c);
    break;
  case BACKSPACE:
  case CTRL_KEY('h'):
  case DEL_KEY:
    if (c == DEL_KEY)
      editorMoveCursor(ARROW_RIGHT);
    editorDelChar();
    break;
  case PAGE_UP:
  case PAGE_DOWN: {
    int times = E.screenrows;
    while (times--)
      editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
  } break;
  case ARROW_LEFT:
  case ARROW_DOWN:
  case ARROW_UP:
  case ARROW_RIGHT:
    editorMoveCursor(c);
    break;
  case CTRL_KEY('l'):
  case '\x1b':
    if (!E.modal) {
      E.modal = 1;
      editorSetStatusMessage("NORMAL MODE");
    }
    break;
  default:
    if (E.modal) {
      char *command;
      switch (c) {
      case 'i':
        E.modal = 0;
        editorSetStatusMessage("INSERT MODE");
        break;
      case '/':
        editorSearch();
        break;
      case 'h':
        editorMoveCursor(ARROW_LEFT);
        break;
      case 'l':
        editorMoveCursor(ARROW_RIGHT);
        break;
      case 'j':
        editorMoveCursor(ARROW_DOWN);
        break;
      case 'k':
        editorMoveCursor(ARROW_UP);
        break;
      case 'w':
        editorMoveCursor(WORD_NEXT);
        break;
      case 'W':
        editorMoveCursor(WORD_LAST);
        break;
      case 'd':
        editorMoveCursor(ARROW_RIGHT);
        editorDelChar();
        break;
      case 'D':
        editorDelRow(E.cy);
        break;
      case ':':
        command = editorPrompt("command: %s", NULL);
        if (command == NULL)
          break;
        if (!strcmp(command, "q") || !strcmp(command, "quit")) {
          if (E.dirty) {
            free(command);
            editorSetStatusMessage("!UNSAVED CHANGES! Press <ENTER> to "
                                   "confirm, ANY other key to cancel");
            if (quit_times)
              quit_times--;
            return;
          }

          free(command);
          editorExit();
          break;
        } else if (!strcmp(command, "!q") || !strcmp(command, "!quit")) {
          free(command);
          editorExit();
          break;
        } else if (!strcmp(command, "w") || !strcmp(command, "write")) {
          editorSave();
          break;
        } else if (!strcmp(command, "help") || !strcmp(command, "h")) {
          editorSetStatusMessage(
              "'w'/'write', '!q'/'!quit', 'wq'/'done', '/' - search");
          break;
        } else if (!strcmp(command, "wq") || !strcmp(command, "done")) {
          editorSave();
          if (E.filename != NULL) {
            free(command);
            editorExit();
          }
          break;
        }
        free(command);
        break;
      }

      break;
    } else {
      editorInsertChar(c);
      break;
    }
  }

  if (quit_times == 0)
    editorSetStatusMessage("");
  quit_times = TI_QUIT_TIMES;
}

/*~~~~~~~~~~~~~~~~~~~~ init ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void initEditor() {
  E.cx = 0;
  E.cy = 0;
  E.rx = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.row = NULL;
  E.dirty = 0;
  E.modal = 1;
  E.filename = NULL;
  E.statusmsg[0] = '\0';
  E.statusmsg_time = 0;
  E.syntax = NULL;
  
  if (getWindowSize(&E.screenrows, &E.screencols) == -1)
    die("getWindowSize");
  E.screenrows -= 2;
}

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    editorOpen(argv[1]);
  }

  editorSetStatusMessage(
      "<C-q> = Quit  |  <C-s> = Save | ESC = Movement mode | i = Insert mode");
  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}

/*~~~~~~~~ FOOTNOTES - CONTRIBUTERS - ETC ~~~~~~~~~*/
/*

  Author: Tairen Dunham <tairenfd at mailbox dot org>
  Description: Ti is a significantly modified fork of antirez's 'Kilo' text
               editor / Paige Ruten's Kilo tutorial. It's largely inspired
               by features found in the text editor Vim, but my name is not
               Tim, so I opted for Ti

*/
