# Makefile for cross-compiling the fbtest program 
#
# Export an OETMP environment variable that points to the
# same location as TMPDIR in build/conf/local.conf
#

### cross-build defs ###

#ifeq ($(strip $(OETMP)),)
## make things fail if OETMP not defined by pointing OETMP
## at an invalid location
#        OETMP=/tmp
#endif

#TOOLDIR = $(OETMP)/sysroots/`uname -m`-linux/usr/bin
#STAGEDIR = ${OETMP}/sysroots/overo/usr

# soft-floating point
#CC = ${TOOLDIR}/armv7a-vfp-neon-poky-linux-gnueabi/arm-poky-linux-gnueabi-gcc

# hard-floating point
#CC = ${TOOLDIR}/cortexa8hf-vfp-neon-poky-linux-gnueabi/arm-poky-linux-gnueabi-gcc

#CFLAGS = -O2 -Wall

#LIBDIR = $(STAGEDIR)/lib
#INCDIR = $(STAGEDIR)/include

### end cross-build defs ###


OBJS = main.o

TARGET = fbtest

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -L $(LIBDIR) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -I $(INCDIR) -c -o $@ $<


# purely for my convenience ;-)
#install: $(TARGET)
#	scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $(TARGET) root@192.168.10.111:/#home/root


.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)

