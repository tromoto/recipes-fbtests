DESCRIPTION = "fbtest"
MAINTAINER = "Pure2"
SECTION = "base"
LICENSE = "proprietary"
PACKAGE_ARCH = "${MACHINE_ARCH}"

require conf/license/license-gplv2.inc

PV = "0.1"
PR = "r7"

###  git clone https://github.com/numbqq/aml_libge2d.git

#inherit autotools-brokensep lib_package pkgconfig
#inherit lib_package pkgconfig

DEPENDS += " libnetpbm "

SRC_URI = "file://fbtest/ \
            \
"

S = "${WORKDIR}/fbtest/"


#CFLAGS += " -I${STAGING_INCDIR} "  # not useful as Rules.make resets CFLAGS

#EXTRA_OECONF = " \
#	BUILD_SYS=${BUILD_SYS} \
#	HOST_SYS=${HOST_SYS} \
#	STAGING_INCDIR=${STAGING_INCDIR} \
#	STAGING_LIBDIR=${STAGING_LIBDIR} \
#	"

#STAGING_INCDIR = ${STAGING_INCDIR}
#STAGING_LIBDIR = ${STAGING_LIBDIR}

#export STAGING_INCDIR
#export STAGING_LIBDIR
#export BUILD_SYS
#export HOST_SYS

do_compile () {
	#oe_runmake lnx
	#...um, the Makefile is too messy. do it directly:
	cd ${S}drawops
    ${CC} -c *.c  -I../include ${CFLAGS} 
    ${AR} -rcs drawops.a  *.o

	cd ${S}fonts
    ${CC} -c *.c  -I../include ${CFLAGS} 
    ${AR} -rcs fonts.a  *.o

	cd ${S}tests
    ${CC} -c *.c  -I../include ${CFLAGS} 
    ${AR} -rcs tests.a  *.o

    cd ${S}visops
    ${CC} -c *.c  -I../include ${CFLAGS} 
    ${AR} -rcs visops.a  *.o
    
    cd ${S}images
    ${CC} -c penguin.c  -I../include ${CFLAGS} 
    ${AR} -rcs images.a  *.o

    cd ${S}
    ${CC} -c *.c  -Iinclude ${CFLAGS} 

    ${CC} -o fbtest *.o  tests/tests.a drawops/drawops.a fonts/fonts.a images/images.a  visops/visops.a   ${LDFLAGS} ${CFLAGS}




#    ${CC} -c bm.c ${CFLAGS} -lpthread -lnsl
#    ${CC} -c version.c ${CFLAGS} -lpthread -lnsl
#    ${CC} -o pnscan pnscan.o bm.o version.o ${LDFLAGS} ${CFLAGS} -lpthread -lnsl
#    ${CC} -o fbtest3 fbtest3.o ${LDFLAGS} tests/tests.a drawops/drawops.a fonts/fonts.a images/images.a  visops/visops.a  ${CFLAGS} 

}




do_install () {
	install -d ${D}${bindir}
	install -m 0755 ${S}/fbtest  ${D}${bindir}/
#	install -m 0755 ${S}/ge2d_chip_check  ${D}${bindir}/
}


FILES_${PN} += "${bindir}/* "


#INSANE_SKIP_${PN} += "file-rdeps  dev-so"

#sysroot_stage_all() {
#    :
#}

#do_package_qa() {
#}


#do_qa_staging() {
#}

do_rm_work() {
}

