COMPILER = cc
LIB_OPTS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
OUT = -o out
CFILES = src/*.c
PLATFORM := $(shell uname)
WSL := $(shell test -d /run/WSL && echo 1 || echo 0)
RUN_CMD = ./out
CLEAN_CMD = rm -rf ./out

ifeq ($(PLATFORM), Darwin)
	COMPILER = clang
	LIB_OPTS = -L/usr/local/Cellar/raylib/5.5/lib/ -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL -lraylib
	INCLUDE_PATHS = -I/usr/local/Cellar/raylib/5.5/include/
endif

ifeq ($(PLATFORM), Linux)
	COMPILER = gcc
	INCLUDE_PATHS = -I/usr/include
endif

ifeq ($(WSL), 1)
	COMPILER = x86_64-w64-mingw32-gcc icon.res -o out.exe
	LIB_OPTS = -lraylib -lopengl32 -lgdi32 -lwinmm -lcomdlg32 -lole32 -static -lpthread -L/mnt/c/Users/asus/Desktop/programing/c/include/raylib-5.5_win64_mingw-w64/lib/ -Wl,--subsystem,windows
	RUN_CMD = ./out.exe
	CLEAN_CMD = rm -rf ./out.exe
	INCLUDE_PATHS = -I/usr/x86_64-w64-mingw32/include/ -I/mnt/c/Users/asus/Desktop/programing/c/include/raylib-5.5_win64_mingw-w64/include/
endif

BUILD_CMD = $(COMPILER) $(INCLUDE_PATHS) $(CFILES) $(OUT) $(LIB_OPTS)

build:
	$(BUILD_CMD)

run:
	$(BUILD_CMD) && $(RUN_CMD) && $(CLEAN_CMD)

clean:
	$(CLEAN_CMD)

