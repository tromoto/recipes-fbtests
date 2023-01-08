DESCRIPTION = "libnetpbm precompiled for aarch64"
SECTION = "base"
PRIORITY = "required"
LICENSE = "CLOSED"
#PACKAGE_ARCH = "${MACHINE_ARCH}"

COMPATIBLE_MACHINE = "^(dreamone|dreamtwo)$"

SRC_URI = "file://libnetpbm.zip"

inherit lib_package 

S = "${WORKDIR}"

do_configure() {
}

do_compile() {
}

do_install() {
	install -d ${D}${libdir}
#	install -m 0755 ${S}/libsystemd.so.0 ${D}${libdir}/
	install -m 0755 ${S}${libdir}/* ${D}${libdir}/
#	cp -r  ${S}${libdir}/* ${D}${libdir}
#	cd  ${D}${libdir}
#	ln -sf  libsystemd.so.0.17.0  libsystemd.so.0 
	install -d ${D}${includedir}/libnetpbm
	cp -r ${S}${includedir}/* ${D}${includedir}/libnetpbm/
}

do_package_qa() {
}

#do_populate_sysroot() {
#}

FILES_${PN} += "{libdir}/libnetpbm.so.10.0  {libdir}/libnetpbm.so.10"
FILES_${PN} += "{libdir}/libnetpbm.a  {libdir}/libnetpbm.so  {includedir}/*"

INSANE_SKIP_${PN} = "already-stripped dev-so"

do_rm_work() {
}

