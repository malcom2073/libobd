######################################################################
# Automatically generated by qmake (2.01a) Thu Feb 11 07:06:09 2010
######################################################################

TEMPLATE = lib
TARGET = obd
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += debug


HEADERS += ObdInfo.h obdlib.h ObdThread.h
SOURCES += ObdInfo.cpp obdlib.cpp ObdThread.cpp

VERSION = 1.0.0
PROJECT_NAME = libobd

win32: {
	CONFIG += staticlib qt
	DEFINES += WINHACK=1
	target.path = c:/libs/libobd/lib
	headers.path = c:/libs/libobd/include
	headers.files = $$HEADERS
}
unix:CONFIG += debug qt
# create_prl
# Input


isEmpty( INSTALL_ROOT ){
	INSTALL_ROOT = $$DESTDIR
}
unix {
	target.path = $$INSTALL_ROOT/usr/lib
	headers.files = $$HEADERS
	headers.path = $$INSTALL_ROOT/usr/include
	dist.commands += rm -fR $${PROJECT_NAME}-$${VERSION} &&
	dist.commands += git clone . $${PROJECT_NAME}-$${VERSION} &&
	dist.commands += rm -fR $${PROJECT_NAME}-$${VERSION}/.git &&
	dist.commands += tar cjf $${PROJECT_NAME}-$${VERSION}.tar.bz2 $${PROJECT_NAME}-$${VERSION} &&
	dist.commands += rm -fR $${PROJECT_NAME}-$${VERSION} &&
	dist.commands += echo "make dist complete."
	QMAKE_EXTRA_TARGETS += dist
}

INSTALLS += target headers

OTHER_FILES += \
    ObdThread.cs

DISTFILES += \
    libobd.pri
