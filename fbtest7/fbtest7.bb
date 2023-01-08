DESCRIPTION = "fbtest7"
MAINTAINER = "Pure2"
SECTION = "base"
LICENSE = "proprietary"
PACKAGE_ARCH = "${MACHINE_ARCH}"

require conf/license/license-gplv2.inc

PV = "0.1"
PR = "r0"

###  git clone https://github.com/numbqq/aml_libge2d.git

#inherit autotools-brokensep lib_package pkgconfig
#inherit lib_package pkgconfig

#DEPENDS += " libnetpbm "

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
    ${CC} -c fbdev.c ${CFLAGS} 
#    ${CC} -c bm.c ${CFLAGS} -lpthread -lnsl
#    ${CC} -c version.c ${CFLAGS} -lpthread -lnsl
#    ${CC} -o pnscan pnscan.o bm.o version.o ${LDFLAGS} ${CFLAGS} -lpthread -lnsl
    ${CC} -o fbtest7 fbdev.o ${LDFLAGS} ${CFLAGS} 

}



do_install () {
	install -d ${D}${bindir}
	install -m 0755 ${S}/fbtest7  ${D}${bindir}
}


FILES_${PN} += "${bindir}/* "
FILES_${PN}-dev = ""
FILES_${PN}-dbg = ""


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

