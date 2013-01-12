CC            = gcc
CFLAGS        = -std=c99 -O3 -march=native -fomit-frame-pointer -fno-exceptions -mfpmath=sse -mpush-args -mno-accumulate-outgoing-args -mno-stack-arg-probe
INCPATH       = -I'.' -I'D:/pthreads/Pre-built.2/include'
LIBS        =        D:/pthreads/Pre-built.2/lib/pthreadVCE2.lib 
LINK          = ld
LFLAGS        = -subsystem,console 
LIBS          = -lpthread

OBJECTS_DIR   = . 

SOURCES       = ../S51D/main.c \
		MCS51.c \
		DeAsmTables.c \
		DeAsm.c \
		IntelHex.c \
		Global.c \
		Debugger.c \
		VT100.c \
		CONSOLE.c \
		Utils.c \
		Keyboard.c 
OBJECTS       = main.o \
		MCS51.o \
		DeAsmTables.o \
		DeAsm.o \
		IntelHex.o \
		Global.o \
		Debugger.o \
		VT100.o \
		CONSOLE.o \
		Utils.o \
		Keyboard.o
DIST          = 
QMAKE_TARGET  = S51D
DESTDIR_TARGET = S51D.exe

all: $(OBJECTS) 
	$(LINK) $(LFLAGS) -o $(DESTDIR_TARGET)  $(LIBS)

main.o: main.c Global.h \
		MCS51.h \
		Debugger.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o main.o main.c

MCS51.o: MCS51.c MCS51.h \
		Utils.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o MCS51.o MCS51.c

DeAsmTables.o: DeAsmTables.c 
	$(CC) -c $(CFLAGS) $(INCPATH) -o DeAsmTables.o DeAsmTables.c

DeAsm.o: DeAsm.c DeAsm.h \
		Global.h \
		MCS51.h \
		DeAsmTables.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o DeAsm.o DeAsm.c

IntelHex.o: IntelHex.c IntelHex.h \
		Global.h \
		MCS51.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o IntelHex.o IntelHex.c

Global.o: Global.c Global.h \
		MCS51.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o Global.o Global.c

Debugger.o: Debugger.c Global.h \
		MCS51.h \
		Debugger.h \
		DeAsm.h \
		DeAsmTables.h \
		IntelHex.h \
		VT100.h \
		Utils.h \
		Keyboard.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o Debugger.o Debugger.c

VT100.o: VT100.c vt100.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o VT100.o VT100.c

CONSOLE.o: CONSOLE.c vt100.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o CONSOLE.o CONSOLE.c

Utils.o: Utils.c Global.h \
		MCS51.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o Utils.o Utils.c

Keyboard.o: Keyboard.c Global.h \
		MCS51.h \
		Keyboard.h
	$(CC) -c $(CFLAGS) $(INCPATH) -o Keyboard.o Keyboard.c
