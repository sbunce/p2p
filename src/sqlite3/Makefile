CFLAGS = -O3

OBJECTS = $(patsubst %.c,%.o,$(wildcard *.c))

.PHONY: all
all: $(OBJECTS)

clean:
	rm -f *.o
