
# txt
portable and half usable text editor from-scratch in c&raylib

**note:** this is my first big project in c in general, please be generous

## galery
![screenshot1](assets/screenshot1.png)
_Empty buffer with "Hello world" in txt_

![screenshot2](assets/screenshot2.png)
_txt's src opened in itself_

## quickstart
to compile run `$ ./build.sh` or `$ ./build.sh windows`<br/>
_compilation on windows isn't supported, sorry_
Or the usual `make` command should do the same

to compile bundler run `$ ./build_bundler.sh`<br/>
to use bundler run `$ ./bundle <font.ttf> [<some-more-fonts.ttf> <more.ttf>...] 2> src/font.c`

for linux building install clang or gcc. for windows building install the mingw-w64 compiling set.
Compiling for mac (Darwin) using XCode should work since it is a BSD/Unix system

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

## thanks to
- creators of font Victor Mono, as it is used as default font here

