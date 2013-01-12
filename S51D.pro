#QMake for C project

CONFIG = console
QT -= core gui

QMAKE_CC = gcc
QMAKE_CFLAGS = -std=c99 -O0

win32:INCLUDEPATH = H:/pthreads/Pre-built.2/include
win32:LIBS += H:/pthreads/Pre-built.2/lib/x86/pthreadVCE2.lib

unix:LIBS += -lpthread

HEADERS += \
    MCS51.h \
    Global.h \
    IntelHex.h \
    Debugger.h \
    VT100.h \
    Utils.h \
    Keyboard.h \
    memleaks.h

SOURCES += \
    main.c \
    MCS51.c \
    IntelHex.c \
    Global.c \
    Debugger.c \
    VT100.c \
    CONSOLE.c \
    Utils.c \
    Keyboard.c \
    memleaks.c
