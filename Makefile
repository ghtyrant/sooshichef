CC := gcc
LD := $(CC)

# Glib/GObject/Gio includes and libraries
GLIB_CFLAGS  := $(shell pkg-config --cflags gtk+-3.0)
GLIB_LDFLAGS := $(shell pkg-config --libs gtk+-3.0)

CFLAGS  := -Wall -pedantic -g $(GLIB_CFLAGS) -I../libsooshi/src/
LDFLAGS := $(GLIB_LDFLAGS) -L../libsooshi/ -lsooshi

TARGET  := sooshichef
SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

run: $(TARGET)
	LD_LIBRARY_PATH=../libsooshi/ G_MESSAGES_DEBUG=all ./$(TARGET)

gdb: $(TARGET)
	LD_LIBRARY_PATH=../libsooshi/ G_MESSAGES_DEBUG=all gdb ./$(TARGET)

valgrind: all
	LD_LIBRARY_PATH=../libsooshi/ valgrind --tool=memcheck --leak-check=full --show-leak-kinds=definite,indirect,possible --show-reachable=no --leak-resolution=high --num-callers=20 --trace-children=no --child-silent-after-fork=yes --track-fds=yes --track-origins=yes ./$(TARGET) 2>&1 | tee valgrind.log

clean:
	rm $(TARGET) $(OBJECTS)
