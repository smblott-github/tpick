tpick
=====

`tpick` is a simple, curses-based interactive utility for picking one of a
number of things at the terminal.  Here are a couple of examples...

Interactively pick a file (with `zsh`-style globing and read):

    tpick **/*.gpg | read file

Interactively pick from standard input (with `zsh`-style read):

    seq 1000 | tpick -i | read number

`tpick` works like `dmenu` or `slmenu`: just start typing characters, and only entries containing those characters are displayed.  Matching is smartcase, and uses `fnmatch`.

Use `ENTER` to exit, writing the selected element (the one at the top of the list) to standard output.  You can also exit (and fail) using either `ESCAPE`, `Control-C` or `qq`.  You never search for two consecutive `q` characters, right?

There are more details in the manual page.
