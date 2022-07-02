KiloT - 'Build your own editor' fork of Kilo by Antirez
====================================

This is solely being used as a learning tool. If you notice anything 
in the comments that sounds wrong or can be worded better please let me know. 
The code is littered with a ridiculous amount of comments to help me with the
learning process.

See original kilo source code at [github/antirez](https://github.com/antirez/kilo "Kilo Text Editor")

See 'BYOE' documents [here](https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html "Build Your Own Editor")


# Keybinds

### General bindings

- **Ctrl + q** : Quit, if unsaved changes, a prompt will appear and require you to press <ENTER> 

- **Ctrl + s** : Save file

- **ESC** : Change mode, current modes are 'Movement' and 'Type'

- **HOME** : Go to start of row, if already at start, go to previous row
- **END** : Go to end of row, if already at end, go to next row

- **Page Up** : Go up one page
- **Page Down** : Go down one page

### Movement mode

- **h, j, k, l** : left, down, up, right movement keys

- **w** : next word
- **W** : previous word

- **d** : delete character under cursor
- **D** : delete current row

### Type mode

- All general keybindings should work, as well as normal typing to screen

Contributers
=============

- github.com/tairenfd
- github.com/antirez
