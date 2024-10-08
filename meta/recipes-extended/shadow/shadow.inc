SUMMARY = "Tools to change and administer password and group data"
HOMEPAGE = "http://github.com/shadow-maint/shadow"
DESCRIPTION = "${SUMMARY}"
BUGTRACKER = "http://github.com/shadow-maint/shadow/issues"
SECTION = "base/utils"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://COPYING;md5=c9a450b7be84eac23e6353efecb60b5b \
                    file://src/passwd.c;beginline=2;endline=30;md5=758c26751513b6795395275969dd3be1 \
                    "

DEPENDS = "virtual/crypt"

GITHUB_BASE_URI = "https://github.com/shadow-maint/shadow/releases"
SRC_URI = "${GITHUB_BASE_URI}/download/${PV}/${BP}.tar.gz \
           ${@bb.utils.contains('PACKAGECONFIG', 'pam', '${PAM_SRC_URI}', '', d)} \
           file://useradd \
	   file://0001-Fix-can-not-print-full-login.patch \
           file://CVE-2023-29383.patch \
           file://0001-Overhaul-valid_field.patch \
           file://CVE-2023-4641.patch \
           "

SRC_URI:append:class-target = " \
           file://login_defs_pam.sed \
           file://shadow-update-pam-conf.patch \
           "

SRC_URI:append:class-native = " \
           file://0001-Disable-use-of-syslog-for-sysroot.patch \
           file://commonio.c-fix-unexpected-open-failure-in-chroot-env.patch \
           "
SRC_URI:append:class-nativesdk = " \
           file://0001-Disable-use-of-syslog-for-sysroot.patch \
           "
SRC_URI[sha256sum] = "813057047499c7fe81108adcf0cffa3ad4ec75e19a80151f9cbaa458ff2e86cd"


# Additional Policy files for PAM
PAM_SRC_URI = "file://pam.d/chfn \
               file://pam.d/chpasswd \
               file://pam.d/chsh \
               file://pam.d/login \
               file://pam.d/newusers \
               file://pam.d/passwd \
               file://pam.d/su"

inherit autotools gettext github-releases

export CONFIG_SHELL="/bin/sh"

EXTRA_OECONF += "--without-libcrack \
                 --with-group-name-max-length=24 \
                 --enable-subordinate-ids=yes \
                 --without-sssd \
                 ${NSCDOPT}"

NSCDOPT = ""
NSCDOPT:class-native = "--without-nscd"
NSCDOPT:class-nativesdk = "--without-nscd"
NSCDOPT:libc-glibc = "--with-nscd"

PAM_PLUGINS = "libpam-runtime \
               pam-plugin-faildelay \
               pam-plugin-securetty \
               pam-plugin-nologin \
               pam-plugin-env \
               pam-plugin-group \
               pam-plugin-limits \
               pam-plugin-lastlog \
               pam-plugin-motd \
               pam-plugin-mail \
               pam-plugin-shells \
               pam-plugin-rootok"

PAM_PLUGINS:remove:libc-musl = "pam-plugin-lastlog"

PACKAGECONFIG ??= "${@bb.utils.filter('DISTRO_FEATURES', 'pam', d)} \
                   ${@bb.utils.contains('DISTRO_FEATURES', 'xattr', 'attr', '', d)}"
PACKAGECONFIG:class-native ??= "${@bb.utils.contains('DISTRO_FEATURES', 'xattr', 'attr', '', d)}"
PACKAGECONFIG:class-nativesdk = ""
PACKAGECONFIG[pam] = "--with-libpam,--without-libpam,libpam,${PAM_PLUGINS}"
PACKAGECONFIG[attr] = "--with-attr,--without-attr,attr"
PACKAGECONFIG[acl] = "--with-acl,--without-acl,acl"
PACKAGECONFIG[audit] = "--with-audit,--without-audit,audit"
PACKAGECONFIG[selinux] = "--with-selinux,--without-selinux,libselinux libsemanage"

RDEPENDS:${PN} = "shadow-securetty \
                  base-passwd \
                  util-linux-sulogin"
RDEPENDS:${PN}:class-native = ""
RDEPENDS:${PN}:class-nativesdk = ""

do_install() {
	oe_runmake DESTDIR="${D}" sbindir="${base_sbindir}" usbindir="${sbindir}" install

	# Info dir listing isn't interesting at this point so remove it if it exists.
	if [ -e "${D}${infodir}/dir" ]; then
		rm -f ${D}${infodir}/dir
	fi

	# Enable CREATE_HOME by default.
	sed -i 's/#CREATE_HOME/CREATE_HOME/g' ${D}${sysconfdir}/login.defs

	# As we are on an embedded system, ensure the users mailbox is in
	# ~/ not /var/spool/mail by default, as who knows where or how big
	# /var is. The system MDA will set this later anyway.
	sed -i 's/MAIL_DIR/#MAIL_DIR/g' ${D}${sysconfdir}/login.defs
	sed -i 's/#MAIL_FILE/MAIL_FILE/g' ${D}${sysconfdir}/login.defs

	# Disable checking emails.
	sed -i 's/MAIL_CHECK_ENAB/#MAIL_CHECK_ENAB/g' ${D}${sysconfdir}/login.defs

	# Comment out SU_NAME to work correctly with busybox
	# See Bug#5359 and Bug#7173
	sed -i 's:^SU_NAME:#SU_NAME:g' ${D}${sysconfdir}/login.defs

	# Use proper encryption for passwords
	sed -i 's/^#ENCRYPT_METHOD.*$/ENCRYPT_METHOD SHA512/' ${D}${sysconfdir}/login.defs

	install -d ${D}${sysconfdir}/default
	install -m 0644 ${WORKDIR}/useradd ${D}${sysconfdir}/default
}

do_install:append() {
	# Ensure that the image has as a /var/spool/mail dir so shadow can
	# put mailboxes there if the user reconfigures shadow to its
	# defaults (see sed below).
	install -m 0775 -d ${D}${localstatedir}/spool/mail
	chown root:mail ${D}${localstatedir}/spool/mail

	if [ -e ${WORKDIR}/pam.d ]; then
		install -d ${D}${sysconfdir}/pam.d/
		install -m 0644 ${WORKDIR}/pam.d/* ${D}${sysconfdir}/pam.d/
		# Remove defaults that are not used when supporting PAM.
		sed -i -f ${WORKDIR}/login_defs_pam.sed ${D}${sysconfdir}/login.defs
	fi

	install -d ${D}${sbindir} ${D}${base_sbindir} ${D}${base_bindir}

	# Move binaries to the locations we want
	rm ${D}${sbindir}/vigr
	ln -sf vipw.${BPN} ${D}${base_sbindir}/vigr
	if [ "${sbindir}" != "${base_sbindir}" ]; then
		mv ${D}${sbindir}/vipw ${D}${base_sbindir}/vipw
	fi
	if [ "${bindir}" != "${base_bindir}" ]; then
		mv ${D}${bindir}/login ${D}${base_bindir}/login
		mv ${D}${bindir}/su ${D}${base_bindir}/su
	fi

	# Handle link properly after rename, otherwise missing files would
	# lead rpm failed dependencies.
	ln -sf newgrp.${BPN} ${D}${bindir}/sg

	# usermod requires the subuid/subgid files to be in place before being
	# able to use the -v/-V flags otherwise it fails:
	# usermod: /etc/subuid does not exist, you cannot use the flags -v or -V
	install -d ${D}${sysconfdir}
	touch ${D}${sysconfdir}/subuid
	touch ${D}${sysconfdir}/subgid
}

PACKAGES =+ "${PN}-base"
FILES:${PN}-base = "\
    ${base_bindir}/login.shadow \
    ${base_bindir}/su.shadow \
    ${bindir}/sg \
    ${bindir}/newgrp.shadow \
    ${bindir}/groups.shadow \
    ${sysconfdir}/pam.d/login \
    ${sysconfdir}/pam.d/su \
    ${sysconfdir}/login.defs \
"
RDEPENDS:${PN} += "${PN}-base"

inherit update-alternatives

ALTERNATIVE_PRIORITY = "200"

ALTERNATIVE:${PN} = "passwd chfn chsh chpasswd vipw vigr nologin"
ALTERNATIVE_LINK_NAME[chfn] = "${bindir}/chfn"
ALTERNATIVE_LINK_NAME[chsh] = "${bindir}/chsh"
ALTERNATIVE_LINK_NAME[chpasswd] = "${sbindir}/chpasswd"
ALTERNATIVE_LINK_NAME[vipw] = "${base_sbindir}/vipw"
ALTERNATIVE_LINK_NAME[vigr] = "${base_sbindir}/vigr"
ALTERNATIVE_LINK_NAME[nologin] = "${base_sbindir}/nologin"

ALTERNATIVE:${PN}-base = "newgrp groups login su"
ALTERNATIVE_LINK_NAME[login] = "${base_bindir}/login"
ALTERNATIVE_LINK_NAME[su] = "${base_bindir}/su"

PACKAGE_WRITE_DEPS += "shadow-native"
pkg_postinst:${PN}:class-target () {
	if [ "x$D" != "x" ]; then
	  rootarg="--root $D"
	else
	  rootarg=""
	fi

	pwconv $rootarg || exit 1
	grpconv $rootarg || exit 1
}
