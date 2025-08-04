COMPILER = cc
LIB_OPTS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
INCLUDE_PATHS = -I/usr/local/Cellar/raylib/5.5/include/
OUT = -o out
CFILES = src/*.c
PLATFORM := $(shell uname)

ifeq ($(PLATFORM), Darwin)
	COMPILER = clang
	LIB_OPTS = -L/usr/local/Cellar/raylib/5.5/lib/ -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL -lraylib
endif

BUILD_CMD = $(COMPILER) $(INCLUDE_PATHS) $(CFILES) $(OUT) $(LIB_OPTS)

build:
	$(BUILD_CMD)

run:
	$(BUILD_CMD) && ./out && rm -rf ./out

clean:
	rm -rf ./out

