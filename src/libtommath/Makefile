CFLAGS = -I. -O3

OBJECTS = $(patsubst %.c,%.o,$(wildcard *.c))

libtommath.a: $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS) > /dev/null

clean:
	rm -f *.o
