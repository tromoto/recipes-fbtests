PROGRAM = fbtest

CSOURCES = fbtest.c
COBJECTS = $(CSOURCES:.c=.o)
CFLAGS   = $(CDEFINES) $(CINCLUDE)
CDEFINES = 
CINCLUDE = 
CC       = gcc

LDFLAGS  = $(LDDIRS) $(LDLIBS)
LDDIRS   = 
LDLIBS   = 

all: $(PROGRAM)

$(PROGRAM): $(COBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean: execlean objclean

execlean:
	$(RM) $(PROGRAM)

objclean:
	$(RM) $(COBJECTS)

