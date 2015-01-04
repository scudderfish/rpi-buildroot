################################################################################
#
# python-smbus
#
################################################################################

PYTHON_SMBUS_VERSION = 3.1.0
PYTHON_SMBUS_SOURCE = i2c-tools-$(PYTHON_SMBUS_VERSION).tar.bz2
PYTHON_SMBUS_SITE = http://dl.lm-sensors.org/i2c-tools/releases
PYTHON_SMBUS_SETUP_TYPE = distutils
PYTHON_SMBUS_SUBDIR = py-smbus
PYTHON_SMBUS_ENV = CPPFLAGS="$(TARGET_CPPFLAGS) -I../include"

PYTHON_SMBUS_LICENSE = MIT

$(eval $(python-package))

