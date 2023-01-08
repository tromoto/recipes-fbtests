CROSS_COMPILE ?=
KDIR ?=
KINC := -I$(KDIR)/usr/include

CC	:= $(CROSS_COMPILE)gcc
CFLAGS	?= -O2 -W -Wall $(KINC)
LDFLAGS	?= -lrt

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: fbdev-test

fbdev-test: fbdev.o
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	-rm -f *.o
	-rm -f fbdev-test

