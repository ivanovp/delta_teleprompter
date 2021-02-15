TEMPLATE = app
CONFIG += console
CONFIG -= qt

include(other.pro)
SOURCES += ./gfx.c \
./main.c

HEADERS += ./common.h \
./gfx.h

