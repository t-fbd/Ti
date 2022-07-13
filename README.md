Ti - A Kilo fork
====================================

This is solely being used as a learning tool and isn't intended to be used in lieu of a well established
text editor.

Keeping true with the original Kilo project, the SLOC for this project aims to stay under 1024 lines.
Currently sitting at ~1100 lines according to sloc, but theres plenty of refactoring that can be done.

See original kilo source code at [github/antirez](https://github.com/antirez/kilo "Kilo Text Editor")

See [Paige Ruten's](https://github.com/paigeruten "paigeruten") 'BYOE' documents [here](https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html "Build Your Own Editor")

Any advice is greatly appreciated!

ASCIICAST
=========

[![asciicast](https://asciinema.org/a/mQMB04FYcA8uVQxpkHmbkgl4L.svg)](https://asciinema.org/a/mQMB04FYcA8uVQxpkHmbkgl4L)

FEATURES
========

- Simple Syntax Highlighting for:
    - C
    - Python
    - Go
    - Rust
    - Javascript
    - Bash (Needs a lot of improvement)
    - TODO...
- Search functionality
- Modal editor, 2 modes - NORMAL/INSERT
- Simple editor-command line
- Set theme color

KEYBINDS
========

#### General bindings

- **Ctrl + q** : Quit, if unsaved changes, a prompt will appear and require you to press <ENTER> 

- **Ctrl + s** : Save file

- **ESC** : Enter *Normal mode*
- **i** : Enter *Insert mode*

- **HOME** : Go to start of row, if already at start, go to previous row
- **END** : Go to end of row, if already at end, go to next row

- **Page Up** : Go up one page
- **Page Down** : Go down one page

#### Normal mode

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
    - *'h'* or *'help'* - Help menu, currently just directs user to README

#### Insert mode

- All general keybindings should work, as well as normal typing to screen

SETTINGS
========
- TI_TAB_STOP = Tab render size
- in editor-command: set theme <color> = Editor's "theme"
    - Black
    - Red
    - Green
    - Yellow
    - Blue
    - Magenta
    - Cyan
    - White / Default

TODO
====

- Syntax highlighting:
    - Zsh
    - Make
    - Git
    - HTML
    - etc...
- Scroll buffer
- Refactoring of deletion functionalities
- Refactor binds
- Refactor suntax(?)
- Fix search function (same row functionality)
- Undo / Redo functionality
- Rewrite C -> Rust?

KNOWN ISSUES
============
- Search function only finds the first match in a row
- Del current word will go to end of next word when
deleting a string of spaces/tabs

CLOC RESULTS
============

diff against paigerutens [kilo-tut source](https://github.com/snaptoken/kilo-src/blob/master/kilo.c)
github.com/AlDanial/cloc v 1.94  T=0.03 s (34.6 files/s, 46928.2 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
SUM:
 same                            0              0            110            539
 modified                        1              0             40            202
 added                           0             50             39            359
 removed                         0              0              0             16
-------------------------------------------------------------------------------

diff against original [kilo](https://github.com/antirez/kilo/blob/master/kilo.c)

github.com/AlDanial/cloc v 1.94  T=0.03 s (30.4 files/s, 56173.0 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
SUM:
 same                            0              0              1             87
 modified                        1              0            103            509
 added                           0             82             85            504
 removed                         0              0             89            390
-------------------------------------------------------------------------------


CONTRIBUTERS / CREDIT TO
========================

- tairenfd
- antirez
- paigeruten
- Python and Go syntax highlighting keywords/types from 
[dvwallin's](https://github.com/dvwallin) 
[openemacs project](https://github.com/dvwallin/openemacs), another fork of Kilo!
