TEMPLATE = app
CONFIG += console
CONFIG -= qt

include(other.pro)
SOURCES += ./gfx.c \
./linkedlist.c \
./main.c

HEADERS += ./common.h \
./linkedlist.h \
./script.h \
./gfx.h \
./dejavusans_ttf.h

