
TOPDIR = ..

A_TARGET = images.a

SRCS += penguin.c

include $(TOPDIR)/Rules.make

%.c:	%.pbm
	../pnmtohex/pnmtohex $< $* > $@

%.c:	%.pgm
	../pnmtohex/pnmtohex $< $* > $@

%.c:	%.ppm
	../pnmtohex/pnmtohex $< $* > $@

%.c:	%.pnm
	../pnmtohex/pnmtohex $< $* > $@

clean::
	$(RM) $(SRCS)

