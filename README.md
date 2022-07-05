Ti - A Kilo fork
====================================

This is solely being used as a learning tool. If you notice anything 
in the comments that sounds wrong or can be worded better please let me know. 
The code is littered with a ridiculous amount of comments to help me with the
learning process.

See original kilo source code at [github/antirez](https://github.com/antirez/kilo "Kilo Text Editor")

See [Paige Ruten's](https://github.com/paigeruten "paigeruten") 'BYOE' documents [here](https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html "Build Your Own Editor")

Anything to do with modal operations and commands were implemented by me. Several improvements were made to much of the 
keyboard operations in regard to UX. All other credit is entirely to Paige Ruten's "BYOE" project as well as antirez's 
original Kilo project.

Keybinds
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
    - 'Down arrow (↓) / Right arrow (→)' - Next Search
    - 'Up arrow (↑) / Left arrow (←)' - Previous search result
    - 'ESC' - Cancel search
    - 'ENTER' - Go to current selection

- **h, j, k, l** : left, down, up, right movement keys

- **w** : next word
- **W** : previous word

- **d** : delete character under cursor
- **D** : delete current row

- **:** : open editor command line
    - 'w' or 'write' - Save file
    - 'q' or 'quit' - Quit, will prompt if unsaved changes
    - '!q' or '!quit' - Force quit
    - 'wq' or 'done' - Save and quit
    - 'h' or 'help' - Help menu, currently just directs user to README

#### Insert mode

- All general keybindings should work, as well as normal typing to screen

TODO
====

1) Improve everything
2) ...


Contributers / Credit To
========================

- tairenfd
- antirez
- paigeruten
