################################################################################
#
# libnfc
#
################################################################################

LIBNFC_VERSION = 7b7f5061427b5456835dd48923a8cc0563cfd1e9
LIBNFC_SITE = http://libnfc.googlecode.com/git
LIBNFC_SITE_METHOD = git
LIBNFC_LICENSE = LGPLv3+
LIBNFC_LICENSE_FILES = COPYING
LIBNFC_AUTORECONF = YES
LIBNFC_INSTALL_STAGING = YES

LIBNFC_DEPENDENCIES = host-pkgconf libusb libusb-compat

# N.B. The acr122 driver requires pcsc-lite.
LIBNFC_CONF_OPTS = --with-drivers=arygon,pn532_i2c,pn532_spi,pn532_uart,pn53x_usb

ifeq ($(BR2_PACKAGE_LIBNFC_EXAMPLES),y)
LIBNFC_CONF_OPTS += --enable-example
LIBNFC_DEPENDENCIES += readline
else
LIBNFC_CONF_OPTS += --disable-example
endif

$(eval $(autotools-package))
