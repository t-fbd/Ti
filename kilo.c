/*
  Author: Tairen Dunham
  Date: June 30, 2022
  Description: 'Build Your Own Text Editor' by snaptoken, inspired by antirez's 'Kilo'
                editor.
*/

/*** includes ***/

#include <asm-generic/ioctls.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

#define CTRL_KEY(key) ((key) & 0x1f)

/*** data ***/

/*
  Create struct for original terminal flags
  https://www.man7.org/linux/man-pages/man3/termios.3.html <- termios docs
  
  Create global struct for editor state that contains data for terminal 
    width and height, this is gathered by getWindowSize function using 
    ioctl()

  Initilialize a config struct 'E'
*/
struct editorConfig {

  int screenrows;
  int screencols;
  struct termios orig_termios;

};

struct editorConfig E;

/*** terminal ***/

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
char editorReadKey () {
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
  
  return c;

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

/*** append buffer ***/

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

/*** output ***/

/*
  Draw tildes on all rows visable on screen (ie. cols size of 
    terminal)

  String '~\r\n' is 3 bytes that will write to STDOUT a tilde + carriage
    return + newline, if last line is reached then make an exception

  ESC sequence 'K' - 'Erase In Line', analogous to 'J' command, except it
    applies to the current line instead of the entire screen

  refer to https://vt100.net/docs/vt100-ug/chapter3.html#EL

*/
void editorDrawRows (struct abuf *ab) {
  
  int y;
  for (y = 0; y < E.screenrows; ++y) {

    abAppend(ab, "~", 1);
    
    abAppend(ab, "\x1b[K", 3);
    if (y < E.screenrows -1) {
      
      abAppend(ab,"\r\n", 2);
      
    }
        
  }
  
}

/*
  \x1b = ESC character / ASCII dec = '27'
  
  terminal escape sequences always start with ESC followed by a '[' char
  the 'J' command - the 'J' command is for 'Erase In Display' / clear screen
    and the parameter of 2 is to clear the entire screen, this is replaced
    with the 'K' to avoid clearing all lines and redrawing after each refresh

  the H command is for cursor position - can take two arguments of row and 
    column number, for example /x1b[12;40H would center the cursor in the 
    center of the screen on a 80x24 size terminal. The default arguments 
    for H are (1;1). Rows and columns start at 1, not 0.
  
  the 'h' and 'l' commands are for 'Set Mode' and 'Reset Mode', these are 
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
  
  //initialize an abuf struct 'ab' by assigning ABUF_INIT to it - which is 
  //a null pointer and a length of 0
  struct abuf ab = ABUF_INIT;
  
  //append escape sequences for clear and cursor position to buffer stream
  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);
  
  //pass 'ab' into editorDrawRows to draw tildes on blank lines
  editorDrawRows(&ab);
  
  //reposition cursor after drawing tildes
  abAppend(&ab, "\x1b[H", 3);
  abAppend(&ab, "\x1b[?25h", 6);
  
  //write buffer contents to STDOUT and free allocated memory
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

/*** input ***/

/*
  Waits for a keypress and handles it - deals with mapping keys to editor 
    functions at a higher level
*/
void editorProcessKeypress () {
  
  char c = editorReadKey();
  
  switch (c) {

    //if input is 'ctrl + q' then exit
    //clear and reposition on exit
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
    
  }
  
}

/*** init ***/

void initEditor () {
  
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
  
}

int main () {
  /*
    Enable raw mode
    then initialize fields in editorConfig struct 'E'
  
    Refer to https://pubs.opengroup.org/onlinepubs/7908799/xbd/termios.html
      'General Terminal Interface - The Single UNIX ® Specification, Version 2'
      for info on canonical and raw input modes.
  */
  enableRawMode();
  initEditor();
  
  while (1) {
    
      editorRefreshScreen();
      editorProcessKeypress();
    
  }
  
  return 0;
  
} 
