/*
  Author: Tairen Dunham
  Date: June 30, 2022
  Description: Fork of antirez's 'Kilo' text editor.
*/

#define KILO_VERSION "0.0.1"


/*~~~~~~~~~~~~~~~~~~~~ includes ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//Added for portability with getline() on some compilers
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stddef.h>
#include <asm-generic/ioctls.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>


/*~~~~~~~~~~~~~~~~~~~~ defines ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//set tab stop length
#define KILO_TAB_STOP 8
//set quit confimations
#define KILO_QUIT_TIMES 1

#define CTRL_KEY(key) ((key) & 0x1f)

//define movement key characters
enum editorKey {
  
  BACKSPACE = 127,
  //arbitrary values that are out-of-range of possible chars
  //these are called after parsing in editorReadKey()
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
  //TODO
  WORD_LAST
  
};


/*~~~~~~~~~~~~~~~~~~~~ data ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*
  Create struct for storing text within the editor

  erow = Editor row
  erow value is added to the global state in editorConfig, it stores a
    line of text as a dynamically allocated char arr, as well as the 
    length of the data

  render is for rendering tabs

*/
typedef struct erow {

  int size;
  int rsize;
  char *chars;
  char *render;

} erow;

/*
  Create struct for original terminal flags
  https://www.man7.org/linux/man-pages/man3/termios.3.html <- termios docs
  
  Create global struct for editor state that contains data for terminal 
    width(screencols) and height(screenrows), this is gathered by getWindowSize function using 
    ioctl()

  cx and cy are for cursors current x(horizontal/cols) and y(vertical/rows) position
  rx is the index into the render field of the row, cx is an index into the chars field
    if no tabs are present rx = cx, if tabs are present rx > cx

  numrows correlates to the number of rows in the file

  rowoff (row offset) correlates to the current row of the file the user is scrolled to
  coloff (column offset) correlates to the current column of the file the user is scrolled to
  
  filename correlates to the name of the file currently being edited

  time_t comes from time.h,  E.statusmsg will display a status message - blank by default
    E.statusmsg_time will contain the timestamp when status message is set

  'dirty' is a flag to notify if changes have been made to file

  Initilialize a global config struct 'E'
*/
struct editorConfig {

  int cx, cy;
  int rx;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  int numrows;
  //create array of 'row' erow structs
  erow *row;
  int dirty;
  int modal;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  struct termios orig_termios;

};

struct editorConfig E;


/*~~~~~~~~~~~~~~~~~~~~ function prototypes ~~~~~~~~~~~~~~~~~~~*/

void editorSetStatusMessage(const char *fmt, ...);

/*~~~~~~~~~~~~~~~~~~~~ terminal ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*
  Prints error message thats acquired from const char * argument and exits
*/
void die (const char *s) {

    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
  
}

void disableRawMode () {
  
  /*
    Set orig_termios flags as user_default - called at exit
    TCSAFLUSH option discard any unread input before applying 
    changes to the terminal
  
    if tcsetattr returns -1 which is a an error/failure then
      print error message "tcsetattr" and exit
  */
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
    
    die("tcsetattr");
    
  }
  
}

void enableRawMode () {

  /*
    Get user default termios and return to defaults at exit
    exit with message "tcgetattr" on failure
  */
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);
  
  /*
    Set raw termios to user defualt before adding flag changes
  */
    struct termios raw = E.orig_termios;
  
  /*
  local flags c_lflag -
    Disable canonical mode with ICANON - input will be read byte-by-byte instead of
      line-by-line now
    Disable echo with ECHO 
    Disable SIGINT and SIGTSTP with ISIG - ctrl-c and ctrl-z will be read as a '3'
      byte and '26' byte respectively.
    Disbale XON and XOFF software flow control using ctrl-s and ctrl-q - ctrl-s 
      and ctrl-q will be read as a '19' byte and '17' byte respectiely
    Disable ctrl-v / IEXTEN flag - ctrl-v read as a '22' byte now, and on mac
      ctrl-o will now be a '15' byte
  
  input flags c_iflag - 
    Disable flags by setting flag (c_lflag ie. control local flag, c_iflag - input 
      flags), then set flag attributes to termios raw
    Disable translation of carriage returns ('13') to new lines('10') with ICRNL 
      (input carriage return new line) - ctrl-m and enter now read as a '13' byte 
      and not '10'
  output flags c_oflag - 
    Disable post-processing of output with OPOST - \r\n neccesarry for a newline
      now
  
  miscellaneous flags -
    includes BRKINT, INPCK, ISTRIP and CS8
    refer to miscellaneous flag section of https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html
      for some more info. Most of these flags shouldn't have nay noticable effect
      in modern terminals, but switching them off apparently used to be commonplace 
      when enabling 'raw mode'
  
  control characters c_cc - 
    set VMIN and VTIME - VMIN sets minimum amount of bytes needed before read() can
      return, VTIME sets maximum amount of time to wait before read() returns in 100
      ms intervals
  */
  raw.c_iflag &= ~( BRKINT | ICRNL | INPCK | ISTRIP | IXON );
  raw.c_oflag &= ~( OPOST );
  raw.c_cflag &= ~( CS8 );
  raw.c_lflag &= ~( ECHO | ICANON | IEXTEN |ISIG );
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  
  /*
    set flag attributes
    exit with message "tcsetattr" on failure
  */
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
    
}

/*
  Waits for one keypress and returns it
*/
int editorReadKey () {
  /*
    Initialize char c to use as a buffer
  */  
  int nread;
  char c;
  
  /*
    Read STDIN_FILENO to address of buffer 'c' 1 byte at a time
    exit with message "read" on failure
    
    from 'snaptoken' regarding EAGAIN 
      "In Cygwin, when read() times out it returns -1 with an errno of EAGAIN, 
      instead of just returning 0 like it’s supposed to. To make it work in 
      Cygwin, we won’t treat EAGAIN as an error."
  */ 
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  
  /*
    Check for ESC, if no input after ESC assume just ESC was intended to be 
      pressed, otherwise check if additional inputs match to arrow keys, if
      so move accordingly by returning corresponding key for movement
  */
  if (c == '\x1b') {
    
    char seq[3];
    
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    
    //check for ESC[ sequence
    if (seq[0] == '[') {
      //check for a parameter that is an integer 0 through 9
      if (seq[1] >= '0' && seq[1] <= '9') {
        
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        //check if '~' is in ESC[X sequence
        if (seq[2] == '~') {
          
          switch (seq[1]) {
            case '1': return HOME_KEY;
            case '3': return DEL_KEY;
            case '4': return END_KEY;
            // \x1b[5~ = page up
            case '5': return PAGE_UP;
            // \x1b[6~ = page down
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
            
          }
          
        }
        
      } else {
        switch (seq[1]) {
        
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
                
        }

      } 

    } else if (seq[0] == '0') {
      
      switch (seq[1]) {
        
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
        
      }
      
    }
    
    return '\x1b';
    
  } else {
  
    return c;

  }
  
}

/*
  
  This is essential in fallback method to find terminal screen size if ioctl
    fails. 

  ESC[ + 6(parameter) n - 'n' is the command for Device Status Report which can
    query info about the terminal, the argument/parameter of 6 is used to ask
    for cursor position
  ESC[xx:xxR is the cursor postion report, the R must be parsed out

  refer to https://vt100.net/docs/vt100-ug/chapter3.html#DSR
*/
int getCursorPosition (int *rows, int *cols) {
  
  char buf[32];
  unsigned int i = 0;
  
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  while (i < sizeof(buf) - 1) {
    
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    //find 'R' in cursor position report
    if (buf[i] == 'R') break;
    i++;
    
  }
  //set 'R' to null char
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  /*
    If escape sequence is passed to be parsed and doesnt fail, then
      parse numbers that correspond to height and width of terminal
    String of xx:xx (at address of buf[2]) is passed into '%d;%d'
      using sscanf() which will tells it to parse two integers seper-
      -ated by an integer -> these are then passed into the variables
      'rows' and 'cols'
  */
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

  return -1;
  
}

/*
  ioctl(), TIOCGWINSZ, and struct winsize come from <sys/ioctl.h>
  If ioctl has a failure it will return -1 and a poss-
    -ible erroneous outcome of 0

  If it succeeded, we pass the values back by setting the int 
    references that were passed to the function. (This is a 
    common approach to having functions return multiple values 
    in C. It also allows you to use the return value to indicate 
    success or failure.)
*/
int getWindowSize (int *rows, int *cols) {

  struct winsize ws;
  
  /*
    TIOCGWINSZ = terminal i/o control get window size  
  
    if ioctl() fails, try to fallback to method using B and C
      escape sequences and getCursorPosition() function, if that
      fails as well then return -1 aka failure
  */
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    
    /*
      Fallback for if ioctl() fails to gather screen size data
    
      ESC + 999C moves cursor to farthest column to the right of
        terminal
      ESC + 999B moves cursor to the lowest row of the terminal
    
      refer to https://vt100.net/docs/vt100-ug/chapter3.html#CUD
        for more info on ESC[B and ESC[C sequesnces
    */
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    return getCursorPosition(rows, cols);

  } else {
    
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
    
  }

}


/*~~~~~~~~~~~~~~~~~~~~ row operations ~~~~~~~~~~~~~~~~~~~~~~~~~*/

//Modify erow, not cursor position

/*
  Covert char index to render index, loop through all chars in chars index and
  add space in render index for tabs - return render index (rx) which will be used 
  instead of chars index (cx)
*/
int editorRowCxToRx (erow *row, int cx) {
  
  int rx = 0;
  int j;
  for (j = 0; j < cx; ++j) {
    
    if (row->chars[j] == '\t')
      //    columns left of tab , columns right of last tab
      rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
    //goto next tab stop
    rx++;
    
  }
  
  return rx;
  
}

//update row from character index to rendered index
void editorUpdateRow (erow *row) {

  //all of this essentially will copy each row into render variables so tabs can 
  //be rendered
  int tabs = 0;
  int j;
  //loop through characters in row, count tabs
  for (j = 0; j < row->size; ++j)
    if (row->chars[j] == '\t') tabs++;
  
  free(row->render);
  //allocate size for characters in row, tab spaces, and null byte
  row->render = malloc(row->size + tabs*(KILO_TAB_STOP - 1) + 1);
  
  //idx contains number of characters copied into row->render
  int idx = 0;
  //loop through characters in row
  for (j = 0; j < row->size; ++j) {
    if (row->chars[j] == '\t') {
      //render tabs correctly
      row->render[idx++] = ' ';
      while (idx % KILO_TAB_STOP != 0) row->render[idx++] = ' ';
      
    } else {
      
      row->render[idx++] = row->chars[j];
      
    }
    
  }
  
  row->render[idx] = '\0';
  row->rsize = idx;

}

void  editorInsertRow (int at, char *s, size_t len) {
  
  //validate
  if (at < 0 || at > E.numrows) return;
  
  //make room at specified index for a new row
  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
  
  //allocate more space for each row
  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  
  /*
    Allocate enough memory in E.row.chars for char *s and an extra null byte to 
      signify a string
    
    memcpy() = void * mempcpy (void *dest, const void *src, size_t size)
  
  */
  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';
  
  E.row[at].rsize = 0;
  E.row[at].render = NULL;
  editorUpdateRow(&E.row[at]);
  
  E.numrows++;
  //display file has been modified
  E.dirty++;
  
}

void editorFreeRow (erow *row) {
  
  free(row->render);
  free(row->chars);
  
}

void editorDelRow (int at) {
  
  if (at < 0 || at >= E.numrows) return;
  //free memory in current row
  editorFreeRow(&E.row[at]);
  //overwrite row with next row
  memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
  E.numrows--;
  E.dirty++;
  
}

//user input
void editorRowInsertChar (erow *row, int at, int c) {
  
  //check 'at's value, should be equal to row size
  if (at < 0 || at > row->size) at = row->size;
  //space for character and null byte allocated in char *chars
  row->chars = realloc(row->chars, row->size + 2);
  /*
    void *memmove(void *str1, const void *str2, size_t n)
      str1 − This is a pointer to the destination array where the content is to be 
        copied, type-casted to a pointer of type void*.
      str2 − This is a pointer to the source of data to be copied, type-casted to a 
        pointer of type void*.
      n − This is the number of bytes to be copied.
    memmove is a safer approach to memcpy() with overlapping memory bytes
  */
  //use the first byte of the last two extra allocated bytes to copy 
  //the null byte currently present at the end of the string -> +1 byte in memory
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  //update row size
  row->size++;
  //insert character byte where previous null byte was stored
  row->chars[at] = c;
  //render row
  editorUpdateRow(row);
  //Display file has been modified, could be treated as a bool - may use 'dirtiness'
  //of a file later on though
  E.dirty++;
  
}
//                          prev row     row    len of row
void editorRowAppendString (erow *row, char *s, size_t len) {
  
  //realloc space for new size of row and space for null byte
  row->chars = realloc(row->chars, row->size + len + 1);
  //append row(s) to end of previous row(row)
  memcpy(&row->chars[row->size], s, len);
  //update size
  row->size += len;
  //add null byte at end of string
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  E.dirty++;
  
}

void editorRowDelChar (erow *row, int at) {
  
  //if attempting to overwrite outside of row then return
  if (at < 0 || at >= row->size) return;
  //overwrite current char with character one byte ahead, essenentially shift the row
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  editorUpdateRow(row);
  E.dirty++;
  
  
}

/*~~~~~~~~~~~~~~~~~~~~ editor operations ~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*
  editorInsertChar() doesn’t have to worry about the details of modifying an erow, 
    and editorRowInsertChar() doesn’t have to worry about where the cursor is. That 
    is the difference between functions in the editor operations section and 
    functions in the row operations section
*/
//move cursor to position to place cursor
void editorInsertChar(int c) {
  
  //if EOL, make new row
  if (E.cy == E.numrows) editorInsertRow(E.numrows, "", 0);
  
  //editorRowInsert(erow *row, at, c)
  //row being the row to edit, at being the location in character index to insert
  //character, and c is the actual user input
  editorRowInsertChar(&E.row[E.cy], E.cx, c);
  E.cx++;
  
}

void editorInsertNewline () {
  
  if (E.cx == 0) { //if at start of line -> insert blank row before
    
    editorInsertRow(E.cy, "", 0);
    
  } else { //otherwise split the line -> chars right of cursor will be passed 
           //to newline
    
    //current row
    erow *row = &E.row[E.cy];
    editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    //make sure on current row still because editorInsertRow may invalidate pointer 
    //with realloc
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
    
  }
  
  E.cy++;
  E.cx = 0;
  
}

void editorDelChar () {
  
  //if past EOF return - nothing to remove
  if (E.cy == E.numrows) return;
  //if at start of file aka 1:1
  if (E.cx == 0 && E.cy == 0) return;
  
  //current row
  erow *row = &E.row[E.cy];
  //check if removable byte exists - if E.cx has a value then there are bytes present 
  //at file location
  if (E.cx > 0) { 
  
    //overwrite character to the left of cursor
    editorRowDelChar(row, E.cx - 1);
    E.cx--;
  
  } else { //beginning of line thats not 1:1
    
    //set E.cx to end of previous row
    E.cx = E.row[E.cy - 1].size;
    //append current row to previous row
    editorRowAppendString( &E.row[E.cy - 1], row->chars, row->size);
    //overwrite row with next rows to remove it - will free memory in process
    editorDelRow(E.cy);
    //set E.cy to appended row
    E.cy--;
    
  }
  
}

/*~~~~~~~~~~~~~~~~~~~~ file I/O ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//covert row structs array into single string that can be written to file
char *editorRowsToString (int *buflen) {
  
  int total_len = 0;
  int j;
  //cycle through rows
  for (j = 0; j < E.numrows; ++j) 
    //add size of current row to total_len of file + 1 byte for newline char
    total_len += E.row[j].size + 1;
  *buflen = total_len;
  
  //init 'p' pointer to memory block 'buf' that can hold size of total_len
  char *buf = malloc(total_len);
  char *p = buf;
  //cycle through rows
  for (j = 0; j < E.numrows; ++j) {
    
    //copy memory stored in row.chars, to p
    memcpy(p, E.row[j].chars, E.row[j].size);
    //set address of p one character after typed row
    p += E.row[j].size;
    //assign newline to address of p's current location
    *p = '\n';
    p++;
    
  }
  
  //return string containing all row data seprated by newlines
  return buf;
  
}

//open file
void editorOpen(char *filename) {
  
  //free filename before assigning value, strdup() will return a pointer to a 
  //new string which is a duplicate pointed to by filename
  free(E.filename);
  E.filename = strdup(filename);
  
  //open file in read mode
  FILE *fp = fopen(filename, "r");
  //exit if file fails to open
  if (!fp) die("fopen");
  
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  /*
    getline() - ssize_t getline(char **restrict lineptr, size_t *restrict n,
                       FILE *restrict stream);
      reads an entire line from stream, storing the address
      of the buffer containing the text into *lineptr.
      *lineptr can contain a
      pointer to a malloc(3)-allocated buffer *n bytes in size.

  */
  //read entire file into E.row struct array, will return -1 when file reaches
  //EOF
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' ||
                             line[linelen - 1] == '\r' ))
      linelen--;
      //add each row to 'row' erow struct array
    editorInsertRow(E.numrows, line, linelen);
    
  }
  //free allocated memory for row
  //close file pointer
  free(line);
  fclose(fp);
  //set dirty back to 0, wouldve incremented during AppendRow init
  E.dirty = 0;
  
}

//write return value of editorRowsToString() to file
void editorSave() {
  
  //TODO - currently wont save if filename doesnt already exist
  if (E.filename == NULL) return;
  
  int len;
  char *buf = editorRowsToString(&len);
  
  /*
    open(), O_RDWR, and O_CREAT come from <fcntl.h>. ftruncate() and close() 
      come from <unistd.h>
    
    O_RDRW -> Open for read and write
    O_CREAT -> If file doesnt already exist, then create one with permissions
      '0644' which gives owner rd and wr permission but everyone else rd permission
    
    ftruncate will set file size, if file is larger it will shorten it, if file is
      shorter it will add neccessary bytes
  
    Possibly implement in the future -> 
      "The normal way to overwrite a file is to pass the O_TRUNC flag to open(), 
        which truncates the file completely, making it an empty file, before writing 
        the new data into it. By truncating the file ourselves to the same length as 
        the data we are planning to write into it, we are making the whole 
        overwriting operation a little bit safer in case the ftruncate() call 
        succeeds but the write() call fails. In that case, the file would still 
        contain most of the data it had before. But if the file was truncated 
        completely by the open() call and then the write() failed, you’d end up with 
        all of your data lost.
      More advanced editors will write to a new, temporary file, and then rename 
        that file to the actual file the user wants to overwrite, and they’ll 
        carefully check for errors through the whole process." - snaptoken
  
  */
  int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
  //error handling
  if (fd != -1) {
    
    if (ftruncate(fd, len) != -1) {
      
      if (write(fd, buf, len) == len) {
        
        close(fd);
        free(buf);
        //confirm if save was successful
        editorSetStatusMessage("%d bytes written to disk", len);
        E.dirty = 0;
        return;
        
      }
      
    }
     
  }
  
  free(buf);
  editorSetStatusMessage("Failed write to disk! I/O error: %s", strerror(errno));
  
}

/*~~~~~~~~~~~~~~~~~~~~ append buffer ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*
  Append buffer consists of a pointer to the buffer in memory and a 
    length of the buffer

  ABUF_INIT will represent an empty buffer, setting pointer of buffer 
    to NULL and length to 0
*/
struct abuf {
  
  char *b;
  int len;
  
};

#define ABUF_INIT {NULL, 0}

/*
  realloc() and free() come from <stdlib.h>. memcpy() comes from <string.h>

  use char *new to assign/reallocate a block of memory that is the size of 
    the existing buffer - realloc will handle freeing or extending the block 
    of memory, if buffer NULL then return, otherwise use memcpy() to copy 's' 
    to the end of the existing buffer stream

  AbFree() will free() existing memory used by 'abuf' when called
*/
void abAppend (struct abuf *ab, const char *s, int len) {

  char *new = realloc(ab->b, ab->len + len);
  
  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
  
}

void abFree(struct abuf *ab) {

  free(ab->b);
  
}


/*~~~~~~~~~~~~~~~~~~~~ output ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*
  Check if cursor has moved outside visible window while scrolling, adjust E.rowoff 
    accordingly
*/
void editorScroll () {
  
  //set horizontal cursor position for render index
  E.rx = 0;
  if (E.cy < E.numrows) {
    
    E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
    
  }
  
  //check if cursor is above visible window, if so scroll to cursor location
  if (E.cy < E.rowoff) {

    E.rowoff = E.cy;

  }
  //same as previous if statement but for bottom of screen, but not past bottom of
  //file - refer ro editorMoveCursor() ARROW_DOWN
  if (E.cy >= E.rowoff + E.screenrows) {
    
    E.rowoff = E.cy - E.screenrows + 1;
    
  }
  //check if cursor is past the left visible edge of window, if so scroll to cursor 
  //location
  if (E.rx < E.coloff) {

    E.coloff= E.rx;

  }
  //set cursor to right edge of screen and allow user to scroll past right edge
  if (E.rx >= E.coloff + E.screencols) {
    
    E.coloff = E.rx - E.screencols + 1;
    
  }
  
}

/*
  Draw tildes on all rows visable on screen (ie. cols size of 
    terminal)

  String '~\r\n' is 3 bytes that will write to STDOUT a tilde + carriage
    return + newline, if last line is reached then make an exception

  ESC sequence 'ARROW_UP' - 'Erase In Line', analogous to 'ARROW_DOWN' command, except it
    applies to the current line instead of the entire screen

  refer to https://vt100.net/docs/vt100-ug/chapter3.html#EL

*/
void editorDrawRows (struct abuf *ab) {
  
  int y;
  for (y = 0; y < E.screenrows; ++y) {
    //set current row
    int filerow = y + E.rowoff;
    //check if row is part of text buffer, or a row that comes after the end 
    //of the buffer
    if (filerow >= E.numrows) {
      //If no file/blank file opened, display welcome message
      if (E.numrows == 0 && y == 0) {

        /*
          Initialize char arr 'welcome' and interpolate KILO_VERSION into 
            the 'welcome' array
        */
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
          "KiloT -- version %s", KILO_VERSION);
        //truncate string in case terminal screen size too small
        if (welcomelen > E.screencols) welcomelen = E.screencols;
        //center message
        int padding = (E.screencols -welcomelen) / 2;
        if (padding) {
        
          abAppend(ab, "~", 1);
          padding--;
        
        }
        while (padding--) abAppend(ab, " ", 1);

        abAppend(ab, welcome, welcomelen);

      } else {
        
        //print tilde on blank lines
        abAppend(ab, "~", 1);

      }
    } else {
    
      //truncate if len of text buffer in current row or col is more than screen len
      //if user scrolls horizontally past EOL, set len to 0 so nothing is displayed
      int len = E.row[filerow].rsize - E.coloff;
      if (len < 0) len = 0;
      if (len > E.screencols) len = E.screencols;
      abAppend(ab, &E.row[filerow].render[E.coloff], len);
    
    }
    
    abAppend(ab, "\x1b[K", 3);
      
      abAppend(ab,"\r\n", 2);
      
    }

}

/*
  ESC[7m sequence will invert colors, ESC[m returns normal formatting

  status bar will be inverted from rest of editor
*/
void editorDrawStatusBar (struct abuf *ab) {
  
  abAppend(ab, "\x1b[7m", 4);

  /*
    snprintf Parameters:
  
  *str : is a buffer.

  size : is the maximum number of bytes (characters) that will be written to 
    the buffer.

  format : C string that contains a format string that follows the same 
    specifications as format in printf

  … : the optional ( …) arguments are just the string formats like (“%d”, myint)
     as seen in printf.

  */
  //print filename if it exists to status bar, otherwise print [SCRATCH]
  //print line count of file to status bar
  char status[80], rstatus[80];

  //check if filename exist and if file is 'dirty', get total line count
  int len = snprintf(status, sizeof(status), "%.20s - %d Lines %s",
    E.filename ? E.filename : "[SCRATCH]", E.numrows, E.dirty ? "(+)" : "");
  
  //current line/total line count, E.cy is switched from 0-indexed to 1-indexed 
  //for term
  float perc = ((float)E.cy + 1.0)/((float)E.numrows) * 100;
  int rlen = snprintf(rstatus, sizeof(rstatus), "L %d:%d %.0f%%", 
    E.cy + 1, E.cx + 1, perc);

  if (len > E.screencols) len = E.screencols;

  abAppend(ab, status, len);
  
  while (len < E.screencols) {
    
    if (E.screencols - len == rlen){
      
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

/*
  Clear message bar with ESC[K sequence
*/
void editorDrawMessageBar (struct abuf *ab) {
  
  abAppend(ab, "\x1b[K", 3);
  int msglen = strlen(E.statusmsg);
  //ensure message will fit length of screen
  if (msglen > E.screencols) msglen = E.screencols;
  if (msglen && time(NULL) - E.statusmsg_time < 5)
    abAppend(ab, E.statusmsg, msglen);
  
}

/*
  \x1b = ESC character / ASCII dec = '27'
  
  terminal escape sequences always start with ESC followed by a '[' char
  the 'ARROW_DOWN' command - the 'ARROW_DOWN' command is for 'Erase In Display' / clear screen
    and the parameter of 2 is to clear the entire screen, this is replaced
    with the 'ARROW_UP' to avoid clearing all lines and redrawing after each refresh

  the H command is for cursor position - can take two arguments of row and 
    column number, for example /x1b[12;40H would center the cursor in the 
    center of the screen on a 80x24 size terminal. The default arguments 
    for H are (1;1). Rows and columns start at 1, not 0.
  
  the 'ARROW_LEFT' and 'ARROW_RIGHT' commands are for 'Set Mode' and 'Reset Mode', these are 
    used to turn on and off features/modes of the terminal. argument ?25
    is for hiding/showing the cursor. Not all terminals will support this
    , if thats the case - the sequence will simply be ignored.

  refer to https://vt100.net/docs/vt100-ug/chapter3.html#ED for info on 
    'Erase In Display'
  for info on 'Set Mode' and 'Reset Mode'
    https://vt100.net/docs/vt100-ug/chapter3.html#SM
    and https://vt100.net/docs/vt100-ug/chapter3.html#RM
    and https://vt100.net/docs/vt100-ug/chapter3.html#S3.3.4

  VT100 escape sequences will mostly be used within this editor
*/
void editorRefreshScreen() {
  
  editorScroll();
  
  //initialize an abuf struct 'ab' by assigning ABUF_INIT to it - which is 
  //a null pointer and a length of 0
  struct abuf ab = ABUF_INIT;
  
  //append escape sequences for clear and cursor position to buffer stream
  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);
  
  //pass 'ab' into editorDrawRows to draw tildes on blank lines
  //pass 'ab' into editorDrawStatusBar to draw status bar after 
  //all other rows have been rendered
  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);
  
  /*
    Reposition cursor after drawing tildes, set cursor postion to current value
      of E.cx and E.cy - value of 1 is added to change from 0-indexed to 1-
      indexed values due to terminal using 1-indexed values
  */
  char buf[32];
  //Will draw cursors screen location instead of cursors file location after 
  //scrolling
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,
                                            (E.rx - E.coloff) + 1);

  abAppend(&ab, buf, strlen(buf));
  
  abAppend(&ab, "\x1b[?25h", 6);
  
  //write buffer contents to STDOUT and free allocated memory
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);

}

/*
  va_list, va_start(), and va_end() come from <stdarg.h>. vsnprintf() comes from <stdio.h>
    time() comes from <time.h>

  ... sets a variadic funtion - va_start, va_end to signify start and ed of list
*/
void editorSetStatusMessage (const char *fmt, ...) {
  
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
  va_end(ap);
  E.statusmsg_time = time(NULL);
  
}


/*~~~~~~~~~~~~~~~~~~~~ input ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

void editorMoveCursor (int key) {
  
  //if at end of rows string data, dont allow more scrolling to the right, 
  //exeption is made for appending each byte to line
  erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  
  //change cursor position, if out-of-bounds of terminal screen then
  //dont do anything
  switch (key) {
    
    //left
    case ARROW_LEFT:
      if (E.cx != 0) {

        E.cx--;

      } else if (E.cy > 0) { //move back a row if attempting to move past left edge of screen
        
        E.cy--;
        E.cx = E.row[E.cy].size;
        
      }
      break;
    //right
    case ARROW_RIGHT:
      if (row && E.cx < row->size) {

        E.cx++;

      } else if (row && E.cx == row->size) { //move forward a row when cursor is past rowlen
        
        E.cy++;
        E.cx = 0;
        
      }

      break;

    case WORD_NEXT:
      if (row && E.cx < row->size) {
        while (row && E.cx < row->size && row->render[E.cx] != ' ' && 
          row->render[E.cx] != KILO_TAB_STOP) 
        {

          E.cx++;

        }
        while ((row && E.cx < row->size && row->render[E.cx] == ' ') || 
               (row && E.cx < row->size && row->render[E.cx] == KILO_TAB_STOP)) 
        {

          E.cx++;

        }

      } else if (row && E.cx == row->size) {
        
        E.cy++;
        E.cx = 0;
        
      }
     
      break;
        
    //up
    case ARROW_UP:
      if (E.cy != 0) {

        E.cy--;

      }
      break;
    //below can probably be refactored with ARROW_RIGHT and ARROW_LEFT, essentially
    //the same function
    //down
    case ARROW_DOWN:
      if (E.cy < E.numrows) {

        E.cy++;

      }
      break;
    //EOL
    case END_KEY:
      if (row && E.cx < row->size) {

        E.cx = E.screencols - 1;

      } else if (row && E.cx == row->size) { //move forward a row when cursor is past rowlen
        
        E.cy++;
        E.cx = 0;
        
      }
      break;
    //start of line
    case HOME_KEY:
      if (E.cx != 0) {

        E.cx = 0;

      } else if (E.cy > 0) { //move back a row if attempting to move past left edge of screen
        
        E.cy--;
        E.cx = E.row[E.cy].size;
        
      }
      break;

  }
  
  //snap to EOL if cursor ends up past EOL
  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  int rowlen = row ? row->size : 0;
  if (E.cx > rowlen) {
    
    E.cx = rowlen;
    
  }
  
}

/*
  Waits for a keypress and handles it - deals with mapping keys to editor 
    functions at a higher level
*/
void editorProcessKeypress () {
  
  static int quit_times = KILO_QUIT_TIMES;
  
  int c = editorReadKey();
 
  switch (c) {
    
    //if input is ENTER
    case '\r':
      if (quit_times == 0) {
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
        break;
      }
      editorInsertNewline();
      break;

    //if input is 'ctrl + q' then exit
    //clear and reposition on exit
    case CTRL_KEY('q'):
      if (E.dirty) {
        
        editorSetStatusMessage("!UNSAVED CHANGES! Press <ENTER> to confirm");
        if (quit_times) quit_times--;
        return;
        
      }
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
    
    case CTRL_KEY('s'):
      editorSave();
      break;
    
    //home key sets cursor to start of line
    case HOME_KEY:
      editorMoveCursor(c);
      break;
    
    //end key sets to EOL
    case END_KEY:
      editorMoveCursor(c);
      break;
    
    case BACKSPACE:
    //returns dec 8 / BS
    case CTRL_KEY('h'):
    //mapped to ESC[3~
    case DEL_KEY:
      //if DEL_KEY the delete key under cursor not to left of cursor
      if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
      editorDelChar();
      break;
    
    //set page up and page down logic
    case PAGE_UP:
    case PAGE_DOWN:
      {
        
        //implement page up and page down to just move up or down the length 
        //of the screen
        int times = E.screenrows;
        while (times--)
          editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
        
      }
      break;

    //send to editorMoveCursor()
    case ARROW_LEFT:
    case ARROW_DOWN:
    case ARROW_UP:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;
    
    //ignore 'clear screen' terminal keybinding, and ESC key to prevent user insertions
    case CTRL_KEY('l'):
    //ESC key is ignored due to many EDC sequences that arent being handled like F1-F12
    case '\x1b':
      if (E.modal) {
        
        E.modal = 0;
        editorSetStatusMessage("EDIT MODE"); 
        
      } else {
        
        E.modal++;
        editorSetStatusMessage("MOVEMENT MODE");
      
      }
      
      break;
    
    //if edit mode or command, then print character to screen
    default:
      if (E.modal) {
        switch (c){
          
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
          case 'd':
            editorMoveCursor(ARROW_RIGHT);
            editorDelChar();
            break;
          
        }
      } else {
        
        editorInsertChar(c);
        break;

      }

  }
  
  if (quit_times == 0) editorSetStatusMessage("");
  quit_times = KILO_QUIT_TIMES;
  
}


/*~~~~~~~~~~~~~~~~~~~~ init ~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//initialize editor default values
void initEditor () {
  
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
  
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
  //keep 2 lines free at bottom of screen for status bar info
  E.screenrows -= 2;
  
}

int main (int argc, char *argv[]) {
  /*
    Enable raw mode
    then initialize fields in editorConfig struct 'E'
  
    Refer to https://pubs.opengroup.org/onlinepubs/7908799/xbd/termios.html
      'General Terminal Interface - The Single UNIX ® Specification, Version 2'
      for info on canonical and raw input modes.
  */
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    
    editorOpen(argv[1]);
  
  }
  
  editorSetStatusMessage("<C-q> = Quit  |  <C-s> = Save");
  
  while (1) {
    
      editorRefreshScreen();
      editorProcessKeypress();
    
  }
  
  return 0;
  
} 
