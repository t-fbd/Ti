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
- Scroll buffer with mouse -> may need ncurses? I debated about using ncurses to start with, but wanted
to grasp a more solid understanding of c, the terminal, and their inter-workings. Truthfully I would like to remake it using better 
suited libraries such as ncurses. At that point though I think I would rather move towards a rewrite in Rust or Go.
- Refactor deletion functionalities: As kilo doesnt have any native funtionality for deleting the current word, next word, etc.
I figured it'd be good to add some functionality for deletion keys while in *Normal* mode. I think it's a much better UX than
just using *BACKSPACE*, *INS*, or *CTRL-H* (due to emulation of vt100 terminals ANSII escape codes. ANSII ^H == 8 == 0x08 == BS/BACKSPACE)
- Refactor binds
- Refactor syntax/general improvement
- Fix search function (same row functionality)
- Undo / Redo functionality
- Open new empty file (this shouldnt be too difficult)
- Rewrite C -> Rust

KNOWN ISSUES
============
- ~~When saving an existing file as a new file, it results in a small memory error. Valgrind
states 'definitely lost block' equal to the amount of data input since new save. All 
data is successfully saved to new file, however nothing is written to original file 
after new file opened. Error is from previous file not being written to, resulting in 
'lost memory'.~~ managed to work all memory errors down to a single 128 byte empty buffer.
Should be memory safe soon.

- Search function only finds the first match in a row

DESIGN/STRUCTURE
=================

Most of the functionality comes from a combination of the termios library,
ANSII escape sequences, and a good chunk of standard c libraries. Termios
is used to 1) contain a *termios* structure in which we can store the users
default terminal structure before opening the editor 2) contain a new *termios*
structure in which we will be using as the UI. The termios library also allows us to
enable raw mode - where input is available from STDIN character  by character,
echoing is disable, special proccessing of terminal input/output characters
is disabled. The flag constants set within the program are primarily responsible for
allowing us to exit canonical mode as well as enable raw mode. Canonical mode takes input line by line/
when a line delimiter is typed, this doesnt work  for us as we want to get each
character input as its typed - regardless of a delimiter char.

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


CLOC RESULTS
============

diff against paigerutens [kilo-tut source](https://github.com/snaptoken/kilo-src/blob/master/kilo.c)

|Language                   |  files       |    blank      |   comment      |      code |
|:-------------------------:|:------------:|:-------------:|:--------------:|:---------:|
|SUM:                       |              |               |                |           |
| same                      |      0       |        0      |         0      |       522 |
| modified                  |      1       |        0      |        26      |       212 |
| added                     |      0       |       52      |         0      |       680 |
| removed                   |      0       |        0      |       124      |        23 |


diff against original [kilo](https://github.com/antirez/kilo/blob/master/kilo.c)

|Language                   |  files       |   blank       | comment        |  code  |
|:-------------------------:|:------------:|:-------------:|:--------------:|:------:|
|SUM:                       |              |               |                |        |
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
