Ti - A Kilo fork
====================================

This is solely being used as a learning tool and isn't intended to be used in lieu of a well established
text editor.

The initial plan was to keep true to the original Kilo project, and keep the SLOC for this project under 1024 lines.
Currently sitting at ~1408 lines according to sloc, but theres plenty of refactoring that can be done, however, 
I doubt it will get back below 1024 without having to strip features.

See original kilo source code at [github/antirez](https://github.com/antirez/kilo "Kilo Text Editor")

See [Paige Ruten's](https://github.com/paigeruten "paigeruten") 'BYOE' documents [here](https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html "Build Your Own Editor")

Any advice is greatly appreciated!

ASCIICAST
=========

  [![asciicast](https://asciinema.org/a/508410.svg)](https://asciinema.org/a/508410)


HOW TO INSTALL
==============

### Linux (will test other environments soon)

- Dependencies
    - a C compiler, gcc is recommended
    - git, to clone the repo
- Clone git repo
        
        git clone git@github.com:tairenfd/Ti.git
                           or                   
        git clone git@tairenfd.xyz:/var/www/git/

- cd into Ti/ directory
- while in the directory run the command

        sudo make install

- If you dont want to install using sudo you can also run 'make' and
either run it directly within the directory using './ti' or by
installing to another directory within your path manually. This
will just uninstalling slightly more tedious.
- Run with 

        ti [options/filename]   


- Makefile flags: make help, make install, make uninstall, make dist, make options, 
make clean, make clean install

### Uninstall

- To uninstall Ti, if installed using the default Makefile, simply type use the below line

        sudo make uninstall

- This will uninstall the binary and remove the man pages
- Now just delete the Ti/ folder and it should be completely removed!

FEATURES
========

- Simple Syntax Highlighting for:
    - C
    - C++
    - Python
    - Go
    - Rust
    - Javascript
    - HTML, Bash (<- These need a good amount of improvement)
    - more soon...
- Search functionality
- Modal editor, 3 modes - NORMAL/INSERT/DELETE
- Simple editor-command line
- Set theme color
- Set language syntax on the fly

KEYBINDS
========

### General bindings

- **Ctrl + q** : Quit, if unsaved changes, a prompt will appear and require you to press <ENTER> 

- **Ctrl + s** : Save file

- **ESC** : Enter *Normal mode*
- **i** : Enter *Insert mode*

- **HOME** : Go to start of row, if already at start, go to previous row
- **END** : Go to end of row, if already at end, go to next row

- **Page Up** : Go up one page
- **Page Down** : Go down one page

### Normal mode

- **/** : search
    - *'Down arrow (↓) / Right arrow (→)'* - Next Search
    - *'Up arrow (↑) / Left arrow (←)'* - Previous search result
    - *'ESC'* - Cancel search
    - *'ENTER'* - Go to current selection

- **h, j, k, l** : left, down, up, right movement keys

- **w** : next word
- **W** : previous word

- **x** : delete character under cursor
- **dd** : delete current row
- **dw** : delete from current char to end of word
- **dx** : delete current word
- **ENTER** : insert row

- **:** : open editor command line
    - *'w'* or *'write'* - Save file
    - *'q'* or *'quit'* - Quit, will prompt if unsaved changes
    - *'!q'* or *'!quit'* - Force quit
    - *'wq'* or *'done'* - Save and quit
    - *'themes'* - show available themes
    - *'set theme <color>'* - set theme
    - *'set lang <language>'* - set language highlighting
    - *'h'* or *'help'* - Help menu, currently just directs user to README

### Insert mode

- All general keybindings should work, as well as normal typing to screen

SETTINGS AND MORE
=================
- TI_TAB_STOP = Tab render size
- in editor-commands:
    - set theme <color> = Editor's "theme"
        - Black
        - Red
        - Green
        - Yellow
        - Blue
        - Magenta
        - Cyan
        - White / Default
    - set lang <language> = syntax highlighting
        - c
        - c++
        - go
        - html
        - rust
        - js
        - python
        
- More info can be found in

      man ti
        or
      ti -h

- 'ti -h' will show a help menu
- 'ti -v' will show current version of Ti

TODO/POSSIBLE FUTURE DEVELOPMENTS
=================================

- Syntax highlighting:
    - Zsh
    - Make
    - Git
    - Markdown
    - etc...
- Scroll buffer with mouse -> may need ncurses? I debated about using ncurses
to start with, but wanted to grasp a more solid understanding of c, the
terminal, and their inter-workings. Truthfully I would like to remake it using
better  suited libraries such as ncurses. At that point though I think I would
rather move towards a rewrite in Rust or Go.
- Refactor deletion functionalities: As kilo doesnt have any native
funtionality for deleting the current word, next word, etc. I figured it'd
be good to add some functionality for deletion keys while in *Normal* mode.
I think it's a much better UX than just using *BACKSPACE*, *INS*, or *CTRL-H*
(due to emulation of vt100 terminals ANSII escape codes. ANSII ^H == 8 == 0x08
== BS/BACKSPACE)
- Refactor binds
- Refactor syntax/general improvement
- Fix search function (same row functionality)
- Undo / Redo functionality
- Open new empty file (this shouldnt be too difficult)
- Rewrite C -> Rust

KNOWN ISSUES
============
- 1 known mem errors
  - 128 byte empty buffer mem loss when writing an existing file to a new
name.
- Search function only finds the first match in a row
- Set language command will not work unless a filename is present
- If file was opened from a path other than current directory, it will not save
changes to the file in the original directory. It will instead save a new copy
of the original file, with any changes, to the current directory.

DESIGN/STRUCTURE
=================

Most of the functionality comes from a combination of the termios library,
ANSII vt100/xterm escape sequences, and a good chunk of standard c libraries. Termios
is used to 1) contain a *termios* structure in which we can store the users
default terminal structure before opening the editor 2) contain a new *termios*
structure in which we will be using as the UI. The termios library also allows
us to enable raw mode - where input is available from STDIN character  by
character, echoing is disable, special proccessing of terminal input/output
characters is disabled. The flag constants (input flags, output flags, control
flags, and local flags)set within the program are primarily responsible for
allowing us to exit canonical mode as well as enable raw mode. Canonical mode
takes input line by line/ when a line delimiter is typed, this doesnt work
for us as we want to get each character input as its typed - regardless of a
delimiter char.

termios structure

    tcflag_t c_iflag;      // input modes
    tcflag_t c_oflag;      // output modes
    tcflag_t c_cflag;      // control modes
    tcflag_t c_lflag;      // local modes
    cc_t     c_cc[NCCS];   // special characters

editor state structure

    int cx, cy;                   // x,y position of characters
    int rx;                       // position of render index
    int rowoff;                   // offset of current row
    int coloff;                   // offset of current column
    int screenrows;               // total rows that can be displayed
    int screencols;               // total columns that can be displaye
    int numrows;                  // total number of rows in file/scratchpad
    erow *row;                    // refer below to row structure, state of each row
    int dirty;                    // 0 = all data saved, 1 = modified
    int modal;                    // 0 =  insert mode, 1 = normal mod
    int new;                      // 0 = save normally, 1 = write new file
    int delete;                   // toggle 'delete mode', could be part of modal state, but I thought it made more sense seperate
    char *filename;               // current filename
    char setlang[10];             // placeholder for user defined syntax highlighting, will try to incorporate into syntax structure eventually
    int theme;                    // editors 'theme'
    char statusmsg[80];           // placeholder for status message string
    time_t statusmsg_time;        // timestamp for status message so it can be cleared
    struct editorSyntax *syntax;  // refer below to syntax structure, hold state of syntax hl
    struct termios orig_termios;  // refer to termios structure, this contains state of original user terminal

for row
   
      int idx;             // row index
      int size;            // total size of row without \0
      int rsize;           // total size of rendered row
      char *chars;         // characters in row index
      char *render;        // content rendered for screen
      unsigned char *hl;   // syntax color for char in row index
      int hl_open_comment; // row ends with open comment

for syntax

      char *filetype;                  // name of filetype displayed in status bar
      char **filematch;                // array of filetypes to match against
      char **keywords;                 // array of keywords to match against
      char *single_line_comment_start; // string which denotes start of a comment
      char *multi_line_comment_start;  // string which denotes start of a ml comment
      char *multi_line_comment_end;    // string which denotes end of a ml comment
      int flags;                       // flags if syntax hl is for number or string

Once out of canonical mode and in raw mode, we can set up the users interface.
All of this is done by appending escape sequences to the input  processing of
the terminal (hence why you may see a lot of '\x1b' - ESC - or '\x1b[', the
control sequence introducer - character strings  around the code). The cursor
movement, status messages, theming, rendering, etc, all is at least partly
affected by the control sequences. A typical layout for a single escape/control
code typically looks like:

           CSI(n)m              
      '\x1b['  +  39  +  m      
      the CSI   arg(s)  function

the above control sequence for instance would set the foreground color to the
terminals default color. This is because CSI(m)n or '\x1b[(n)m'  is the control
sequence for the 'Select Graphic Rendition' or 'SGR' which sets the display
attributes of the terminal. 39 is simply the code  which is labeled as -> "39
| Default foreground color | Implementation defined (according to standard)".
Codes 40-47 and 49 are what I use for the  terminal theme colors as those set
the background colors. Most of the syntax highlighting is done with codes 30-37
- the foreground modifiers - or 90 - 97 which are *bright* foreground colors.
Most of the *bright* color options will eventually be added into Ti. A few are
currently used for syntax highlighting though. These escape/control sequences
combined with the toggling of raw mode and canonical mode lay much of the
foundation for  the actual user interface in Ti.

lets look at this chunk of code from Ti:

    if (c == ESC) {
      char seq[3];                               // when an ESC key is pressed, initialize a 3 byte sequence to store any potential bytes after ESC
                
    if (read(STDIN_FILENO, &seq[0], 1) != 1)     // return ESC on timeout of read into seq
      return ESC;
    if (read(STDIN_FILENO, &seq[1], 1) != 1)     // return ESC on timeout ''
      return ESC;
                  
    if (seq[0] == '[') {                         // detected control sequence indicator 
      if (seq[1] >= '0' && seq[1] <= '9') {      // check if *n* is between 0 and 9 after CSI, if not return ESC
        if (read(STDIN_FILENO, &seq[2], 1) != 1) 
          return ESC;
        if (seq[2] == '~') {                     // if *n* is in appropriate range, check termianting char
          switch (seq[1]) {                      // <esc> '[' (<keycode>) (';'<modifier>) '~'      -> keycode sequence, <keycode> and <modifier> are decimal numbers and default to 1 (vt)]
          case '1':                              // <esc> '[' (<modifier>) <char>                  -> keycode sequence, <modifier> is a decimal number and defaults to 1 (xterm)
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

and now the corresponding ansii sequences so you can get a better idea of why we're parsing things like this
    
    vt sequences:
    <esc>[1~    - Home        <esc>[16~   -             <esc>[31~   - F17
    <esc>[2~    - Insert      <esc>[17~   - F6          <esc>[32~   - F18
    <esc>[3~    - Delete      <esc>[18~   - F7          <esc>[33~   - F19
    <esc>[4~    - End         <esc>[19~   - F8          <esc>[34~   - F20
    <esc>[5~    - PgUp        <esc>[20~   - F9          <esc>[35~   - 
    <esc>[6~    - PgDn        <esc>[21~   - F10         
    <esc>[7~    - Home        <esc>[22~   -             
    <esc>[8~    - End         <esc>[23~   - F11         
    <esc>[9~    -             <esc>[24~   - F12         
    <esc>[10~   - F0          <esc>[25~   - F13         
    <esc>[11~   - F1          <esc>[26~   - F14         
    <esc>[12~   - F2          <esc>[27~   -             
    <esc>[13~   - F3          <esc>[28~   - F15         
    <esc>[14~   - F4          <esc>[29~   - F16         
    <esc>[15~   - F5          <esc>[30~   -
                    
    xterm sequences:
    <esc>[A     - Up          <esc>[K     -             <esc>[U     -
    <esc>[B     - Down        <esc>[L     -             <esc>[V     -
    <esc>[C     - Right       <esc>[M     -             <esc>[W     -
    <esc>[D     - Left        <esc>[N     -             <esc>[X     -
    <esc>[E     -             <esc>[O     -             <esc>[Y     -
    <esc>[F     - End         <esc>[1P    - F1          <esc>[Z     -
    <esc>[G     - Keypad 5    <esc>[1Q    - F2       
    <esc>[H     - Home        <esc>[1R    - F3       
    <esc>[I     -             <esc>[1S    - F4       
    <esc>[J     -             <esc>[T     -

The next hurdle after this is properly identifying and registering keypresses.
This is done by essentially parsing out each key press(several keypresses
result  in multiple chars) and either having variables within the global state
such as cx/cy postions change due to the keypress, appending control sequences
to the terminal input processing or inserting a normal printable key  onto the
screen. One issue that arises during this is the rendering of tabs. The reason
this is explicitely set within Ti is because tab length is usually up to the
host terminal, however we cant always know the host terminal, so instead we
render tabs manually. This results in a 'render' array and 'character' array to
work with.

One of the bigger challenges I faced was with memory leaks. Both the original
kilo and kilotut repositories have several issues and pull request regarding
memory leaks, as well as potential fixes for a few of them. After a LOT of testing using
valgrind, I was able to trace down every memory leak except one. This will
be  something I fix, but currently it's in a much better position than it
previously was, especially since the I know exactly which block of memory is
being lost and why its happening(some more info in the known problems section).
There were also many bugs related to creating and saving new files, movement
within the  rows, etc, some of which were jsut hiding other errors behind them.

Below is an image of the valgrind results when running Ti with an existing file -> writing to and 
saving that file -> create a duplicate file with new name -> write, save and quit new file.
Any other scenarios I've ran into that have produced memory errors have been squashed other than this one.
![Ti leaks](https://pasteboard.co/wfTBhCG2BE1r.png)

for reference, these are the results from a much larger editor when performing the same operation
![Other editor leaks](https://pasteboard.co/k70aKkxJ2j9W.png)

I decided to improve nearly every feature already present in Kilo - ie safely
saving after changes are made, syntax highlighting and the search function (the
search function is easily what I've worked on the least - I need to take some
time to fix the same row search, a bug that was found ~6 years ago in Kilo). I
also added a large amount of new features not present, such as real time syntax
changing, modal operation, themes, increased syntax support  for several more
languages (python, js, c, c++, go, rust, etc), improved line count, a man page,
more verbose and informative flags(no flags are actually present in Kilo or the
Kilotut afaik), and a generally more robust and better UX.

This is definitely not going to replace your go-to text editor, but maybe you
can learn from this or use it to improve your own editor. Whatever you do,
thank you for checking out the project!

###### Sections in program -> find by searching for '/*~~~+ section'

1) Version
  - version info
2) Includes
  - library header files
3) Defines
  - define macros and enum declarations
4) Data
  - build structs for containing data of current 
  term, original user term, syntax data, etc
5) Filetypes
  - Build syntax structures for each filetype
6) Function Prototypes
7) Terminal
  - deals with low level terminal input, keystrokes,
  error handling, etc
9) Syntax highlighting
  - deals with parsing/analyzing filetype and rows for 
  syntax matches and coloring
10) Row operations
 - functions for operations within a given row
ie append, delete, create, etc
11) Editor Operations
  - Operations that will typically call to row operations
  in order to edit the row
12) File I/O
  - deals with file read and write operations
13) Find/search
  - functions for search functionality
14) Append buffer
 - Creates dynamic string to use as a buffer
  for writing to STDOUT
15) Output
  - Draw to screen, uses append buffer to avoiding
  constant write()'s
16) Input
  - instructions for keys input at a higher level than in Terminal section
17) CLI flag options
  - set program flag options for when the program is run with a flag(s)
18) Init
  - Initialize default editor state, contians main
19) Footnotes


CLOC RESULTS
============

diff against paigerutens [kilo-tut source](https://github.com/snaptoken/kilo-src/blob/master/kilo.c)

|Language                   |  files       |    blank      |   comment      |      code |
|:-------------------------:|:------------:|:-------------:|:--------------:|:---------:|
| same                      |      0       |        0      |         0      |       522 |
| modified                  |      1       |        0      |        26      |       212 |
| added                     |      0       |       52      |         0      |       680 |
| removed                   |      0       |        0      |       124      |        23 |


diff against original [kilo](https://github.com/antirez/kilo/blob/master/kilo.c)

|Language                   |  files       |   blank       | comment        |  code  |
|:-------------------------:|:------------:|:-------------:|:--------------:|:------:|
| same                      |      0       |       0       |       0        |   102  |
| modified                  |      1       |       0       |      26        |   583  |
| added                     |      0       |      84       |       0        |   729  |
| removed                   |      0       |       0       |     167        |   301  |



CONTRIBUTERS / CREDIT TO
========================

- tairenfd
- antirez for creating the original Kilo src
- paigeruten for creating Kilo-tut
- Python and Go syntax keywords/types from the
[openemacs project](https://github.com/dvwallin/openemacs)
