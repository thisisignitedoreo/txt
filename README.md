
# txt
portable and half usable text editor from-scratch in c&raylib

**note:** this is my first big project in c in general, please be generous

## galery
![screenshot](assets/screenshot.png)

## quickstart
to compile run `$ ./build.sh` or `$ ./build.sh windows`<br/>
_compilation on windows isn't supported, sorry_

to compile bundler run `$ ./build_bundler.sh`<br/>
to use bundler run `$ ./bundle <file> [<file> <file>...] 2> file.c`

for linux building install clang. for windows building install mingw-w64.

also, you can download a build under releases

press <kbd>Ctrl</kbd> + <kbd>H</kbd> for in-app help

## pros+cons
pros:
- basic text editing
- opening and saving of files
- c syntax highlighting

cons:
- tabs (\t) dont work (show up as 1 symbol)
- no unicode support (content is just big char dynamic array)
- some more idk

