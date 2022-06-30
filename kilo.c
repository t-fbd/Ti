/*
Author: Tairen Dunham
Date: June 30, 2022
Description: 'Build Your Own Text Editor' by snaptoken, inspired by antirez's 'Kilo'
              editor.
*/

#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*
create struct for original terminal flags
https://www.man7.org/linux/man-pages/man3/termios.3.html <- termios docs
*/

struct termios orig_termios;

void disableRawMode () {
  
  //set orig_termios flags as user_default - called at exit
  //TCSAFLUSH option discard any unread input before applying 
  //changes to the terminal
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
  
}

void enabeRawMode () {

  //get user default termios and return to defaults at exit
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);
  
  //set raw termios to user defualt before adding flag changes
  struct termios raw = orig_termios;
  /*
    Disable canonical mode - input will be read byte-by-byte instead of
    line-by-line now
  
    Diable 'echo' and 'canonical mode' by setting local flag (c_lflag ie. 
    control local flag), then set flag attributes to termios raw
  */
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    
}


int main () {
  
  //enable raw mode
  /*
    Refer to https://pubs.opengroup.org/onlinepubs/7908799/xbd/termios.html
    'General Terminal Interface - The Single UNIX ® Specification, Version 2'
    for info on canonical and raw input modes.
  */
  enabeRawMode();
  
  //initialize char c to use as a buffer
  char c;
  /*
    Read STDIN_FILENO to address of buffer 'c' as long as byte is present
    and not 'q'
  */ 
  while(read(0, &c, 1) == 1 && c != 'q');
  
  return 0;
  
} 