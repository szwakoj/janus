# Detect OS
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
else
    DETECTED_OS := $(shell uname -s)
endif

CC = gcc
CFLAGS = -Wall -I./include -g

# Platform-specific settings
ifeq ($(DETECTED_OS),Windows)
    TARGET = bin/janus.exe
    LIBS = -lraylib -lopengl32 -lgdi32 -lwinmm
    RM = del /Q
    MKDIR = if not exist bin mkdir bin
    PATHSEP = \\
else ifeq ($(DETECTED_OS),Darwin)
    TARGET = bin/janus
    LIBS = -lraylib -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
    RM = rm -f
    MKDIR = mkdir -p bin
    PATHSEP = /
else
    # Linux
    TARGET = bin/janus
    LIBS = -lraylib -lm -lpthread -ldl
    RM = rm -f
    MKDIR = mkdir -p bin
    PATHSEP = /
endif

SRCS = main.c
OBJS = obj/$(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(MKDIR)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

obj/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run