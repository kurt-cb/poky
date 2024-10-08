SUMMARY = "DMI (Desktop Management Interface) table related utilities"
HOMEPAGE = "http://www.nongnu.org/dmidecode/"
DESCRIPTION = "Dmidecode reports information about your system's hardware as described in your system BIOS according to the SMBIOS/DMI standard (see a sample output)."
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://LICENSE;md5=b234ee4d69f5fce4486a80fdaf4a4263"

SRC_URI = "${SAVANNAH_NONGNU_MIRROR}/dmidecode/${BP}.tar.xz \
           file://0001-Committing-changes-from-do_unpack_extra.patch \
           file://CVE-2023-30630_1a.patch \
           file://CVE-2023-30630_1b.patch \
           file://CVE-2023-30630_2.patch \
           file://CVE-2023-30630_3.patch \
           file://CVE-2023-30630_4.patch \
           "

COMPATIBLE_HOST = "(i.86|x86_64|aarch64|arm|powerpc|powerpc64).*-linux"

EXTRA_OEMAKE = "-e MAKEFLAGS="

# The upstream buildsystem uses 'docdir' as the path where it puts AUTHORS,
# README, etc, but we don't want those in the root of our docdir.
docdir .= "/${BPN}"

do_install() {
	oe_runmake DESTDIR="${D}" install
}

SRC_URI[sha256sum] = "43cba851d8467c9979ccdbeab192eb6638c7d3a697eba5ddb779da8837542212"

