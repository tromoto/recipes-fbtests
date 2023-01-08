DESCRIPTION = "fbtest"
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



do_install () {
	install -d ${D}${bindir}
	install -m 0755 ${S}/fbtest  ${D}${bindir}/fbtest1

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

