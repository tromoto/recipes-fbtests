
TOPDIR = .

TARGET = $(CROSS_COMPILE)fbtest

SUBDIRS = drawops pnmtohex fonts images visops tests

LIBS += tests/tests.a drawops/drawops.a fonts/fonts.a images/images.a \
	visops/visops.a

include $(TOPDIR)/Rules.make

images:	pnmtohex

