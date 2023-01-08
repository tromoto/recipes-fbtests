#CC=gcc
CFLAGS +=  --std=c99
LIBS +=-lm
DEPS = main.h draw.h draw3d.h framebuffer.h vector.h
_OBJ = main.o draw.o draw3d.o framebuffer.o vector.o

ODIR=obj
#OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

fbtest: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)
	
.PHONY: clean

clean:
	rm -f $(ODIR)/*.o
