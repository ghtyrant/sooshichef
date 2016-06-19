CC := gcc
LD := $(CC)

# Glib/GObject/Gio includes and libraries
GLIB_CFLAGS  := $(shell pkg-config --cflags gtk+-3.0)
GLIB_LDFLAGS := $(shell pkg-config --libs gtk+-3.0)

CFLAGS  := -Wall -pedantic -g $(GLIB_CFLAGS) -I../sooshi/src/
LDFLAGS := $(GLIB_LDFLAGS) -L../sooshi/ -lsooshi

TARGET  := sooshichef
SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

run: $(TARGET)
	LD_LIBRARY_PATH=/home/fabian/projects/privat/sooshi/ G_MESSAGES_DEBUG=all ./$(TARGET)

gdb: $(TARGET)
	LD_LIBRARY_PATH=/home/fabian/projects/privat/sooshi/ G_MESSAGES_DEBUG=all gdb ./$(TARGET)

clean:
	rm $(TARGET) $(OBJECTS)
