
# txt
portable and half usable text editor from-scratch in c&raylib

**note:** this is my first big project in c in general, please be generous

## galery
![screenshot1](assets/screenshot1.png)
_Empty buffer with "Hello world" in txt_

![screenshot2](assets/screenshot2.png)
_txt's src opened in itself_

## quickstart
to compile run `$ make linux` or `$ make windows`<br/>
_compilation on windows isn't supported, sorry_

to compile bundler run `$ make bundle`<br/>
to use bundler run `$ ./bundle <file> [<another-file>] [...] 2> src/file.c`

for linux building install clang or gcc. for windows building install the mingw-w64 compiling set.<br/>
compiling for mac (darwin) using XCode should work since it is a posix system

also, you can download a build under releases

press <kbd>Ctrl</kbd> + <kbd>H</kbd> for in-app help

## pros+cons
pros:
- basic text editing
- opening and saving of files
- c syntax highlighting

cons:
- tabs (\t) dont work (show up as 1 symbol)
- some more idk

## thanks to
- creators of font Victor Mono, as it is used as default font here

