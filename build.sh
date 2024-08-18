#!/bin/sh
set -xe

if [[ $1 == "windows" ]]
then
	COMPILER="x86_64-w64-mingw32-cc"
	FLAGS="-mwindows -Wall -Wextra -I./ext/raylib-win/include -L./ext/raylib-win/lib"
	LIBS="-l:libraylib.a -lwinmm -lgdi32"
else
	COMPILER="cc"
	FLAGS="-Wall -Wextra -I./ext/raylib/include -L./ext/raylib/lib"
	LIBS="-l:libraylib.a -lm -ldl -ggdb"
fi

${COMPILER} ${FLAGS} -o txt src/main.c ${LIBS}

