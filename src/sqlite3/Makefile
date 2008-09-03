CFLAGS = -Os -Wno-deprecated-declarations

OBJECTS = $(patsubst %.c,%.o,$(wildcard *.c))

.PHONY: all
all: $(OBJECTS)

clean:
	rm -f *.o
