/*
 * S51D - Simple MCS51 Debugger
 *
 * Copyright (C) 2011, Michał Dobaczewski / mdobak@(Google mail)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include "Global.h"

#include "Debugger.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>

#include "DeAsm.h"
#include "IntelHex.h"
#include "Utils.h"
#include "Keyboard.h"

const size_t MAX_LINE = 128;

const char* g_unavailable  = "Unavailable when thread is running.\n";
const char* g_tooFewArguments = "Too few arguments. Type help for more details.\n";
const char* g_stoped = "Stoped at %.4Xh after execute %i instructions.\n";

pthread_t g_MCUThread;
pthread_t g_keyEventLoop;
bool g_MCUThreadRunning;
bool g_keyEventLoopRunning;

#define REQUIRED_ARGS(n, command) \
  if (argc < ((n) + 1)) { fprintf(AppSettings()->errorOut, g_tooFewArguments); command; }

#define STOP_IF_THREAD_RUN(command) \
  if (g_MCUThreadRunning) { fprintf(AppSettings()->errorOut, g_unavailable); command; }

struct _params {
  const char* name;
  void (*function)(int argc, char** argv);
  const char* paramsDesc;
  const char* shortDesc;
};

const char* operatorToString(BreakpointType type)
{
  const char* str = "";

  if (type == EQUAL)
    str = "e";
  else if (type == NOT_EQUAL)
    str = "ne";
  else if (type == LESS)
    str = "l";
  else if (type == LESS_EQUAL)
    str = "le";
  else if (type == GREATER)
    str = "g";
  else if (type == GREATER_EQUAL)
    str = "ge";
  else if (type == AND)
    str = "and";
  else if (type == OR)
    str = "or";
  else if (type == XOR)
    str = "xor";

  return str;
}

BreakpointType stringToOperator(const char* str)
{
  BreakpointType type = 0;
  if (0 == stricmp(str, "e"))
    type = EQUAL;
  else if (0 == stricmp(str, "ne"))
    type = NOT_EQUAL;
  else if (0 == stricmp(str, "l"))
    type = LESS;
  else if (0 == stricmp(str, "le"))
    type = LESS_EQUAL;
  else if (0 == stricmp(str, "g"))
    type = GREATER;
  else if (0 == stricmp(str, "ge"))
    type = GREATER_EQUAL;
  else if (0 == stricmp(str, "and"))
    type = AND;
  else if (0 == stricmp(str, "or"))
    type = OR;
  else if (0 == stricmp(str, "xor"))
    type = XOR;

  return type;
}

const char* memoryToString(MemoryType type)
{
  const char* str = "";

  if (type == IDATA)
    str = "idata";
  else if (type == XDATA)
    str = "xdata";
  else if (type == SFR)
    str = "sfr";
  else if (type == IROM)
    str = "irom";
  else if (type == XROM)
    str = "xrom";

  return str;
}

MemoryType stringToMemory(const char* str)
{
  MemoryType type = 0;

  if (0 == stricmp(str, "idata") || 0 == stricmp(str, "i")  ||
      0 == stricmp(str, "iram") || 0 == stricmp(str, "intram"))
    type = IDATA;
  else if (0 == stricmp(str, "xdata") || 0 == stricmp(str, "x")  ||
           0 == stricmp(str, "xram") || 0 == stricmp(str, "extram"))
    type = XDATA;
  else if (0 == stricmp(str, "sfr") || 0 == stricmp(str, "s")  ||
           0 == stricmp(str, "registers") || 0 == stricmp(str, "r"))
    type = SFR;
  else if (0 == stricmp(str, "irom") || 0 == stricmp(str, "icode")  ||
           0 == stricmp(str, "introm"))
    type = IROM;
  else if (0 == stricmp(str, "xrom") || 0 == stricmp(str, "xcode")  ||
           0 == stricmp(str, "extrom"))
    type = XROM;

  return type;
}

void* defputchar(char chr)
{
  if (AppSettings()->out == AppSettings()->defaultOut) {
#ifdef _WIN32
    vtProcessedTextOut(&chr, 1);
#else
    print("%c", chr);
    fflush(AppSettings()->out);
#endif
  } else {
    print("%c", chr); // Czemu nie ma fputchar ???
  }
  return NULL;
}

void* hexputchar(char chr)
{
  print("%.2X  ", chr);
  if (AppSettings()->out == AppSettings()->defaultOut)
    fflush(AppSettings()->out);
  return NULL;
}

void* keyEventLoop(void* _arg)
{
  g_keyEventLoopRunning = true;
#ifdef __linux__
  initKeyboard();
#endif
  while (1) {
    if (AppSettings()->stopThread) {
      AppSettings()->stopThread = false;
      break;
    }

    if (keyboardHit()) {
      AppSettings()->lastKey = getKey();
      AppSettings()->keyAvailable = true;
    } else {
      AppSettings()->keyAvailable = false;
    }

    /* Daje odrobine czasu na "trafienie" na moment kiedy wczytany program
     * oczekuje wejścia. */
    for (int i = 0; i < 4 && AppSettings()->keyAvailable && !keyboardHit(); ++i)
      msSleep(50);
  }
#ifdef __linux__
  closeKeyboard();
#endif
  g_keyEventLoopRunning = false;
  return NULL;
}

void* runMCU(void* output(char))
{
  g_MCUThreadRunning = true;
  unsigned long long simStart = clock();
  unsigned long long lastTitleRefresh = clock();
  unsigned long long cyclesFromLastRefresh = AppSettings()->mcu->cycles;
  const unsigned long long titleRefreshInterval = CLOCKS_PER_SEC;

  while (true) {
    bool useOut = false;
    bool needIn = false;
    BYTE* in = AppSettings()->keyAvailable ? (BYTE*)&AppSettings()->lastKey : NULL;
    BYTE out = '?';

    /* Przerwij pętle gdy z innego wątku ustanowiono falgę. */
    if (AppSettings()->stopThread) {
      AppSettings()->stopThread = false;
      break;
    }

    /* Czasy działania symulatora i mikrokontrolera. */
    AppSettings()->mcuSec = getMCUTime(AppSettings()->mcu);
    AppSettings()->simSec = (double)(clock() - simStart) / CLOCKS_PER_SEC +
                            AppSettings()->simSyncTimeBeforeStop;

    /* Spowalnia program, aby działał z realną prędkością określoną predkością
       kwarcu. */
    if (AppSettings()->realtime && AppSettings()->mcuSec - AppSettings()->simSec > 0.01) {
      msSleep(10);
      continue;
    }

    /* Przerwij gdy wciśnięto magiczny klawisz (exitkey). */
    if (AppSettings()->keyAvailable && AppSettings()->lastKey == AppSettings()->exitkey) {
      AppSettings()->keyAvailable = false;
      break;
    }

    if (!processMCUEx(AppSettings()->mcu, &out, in, &useOut, &needIn))
      break;

    if (useOut && output != NULL)
      output(out);
    if (needIn)
      AppSettings()->keyAvailable = false;

    /* Przerwij gdy program chce wyłączyć procesor... całkiem logiczne, co? */
    if (checkRegister(AppSettings()->mcu, PCON_PD))
      break;

    /* Przerwij przy nieskończonej pętli. */
    if (!checkRegister(AppSettings()->mcu, IE_EA) &&
        *ROM(AppSettings()->mcu, AppSettings()->mcu->PC) == 0x80 &&
        *ROM(AppSettings()->mcu, AppSettings()->mcu->PC + 1) == 0xFE)
      break;

    /* Tytuł okna konsoli. */
    if (clock() - lastTitleRefresh > titleRefreshInterval) {
      char title[128];
      unsigned long long cyclesPerSec = (AppSettings()->mcu->cycles -
                                         cyclesFromLastRefresh);

      snprintf(title, 128, "%s - cycles: %lu XTAL: %.4fMHz "
               "MCU: %.1fs SIM: %.1fs PC: %.4X",
               APP_NAME,
               (unsigned long)AppSettings()->mcu->cycles, // Nie działa %llu. :(.
               (double)(cyclesPerSec * 12) / 1000000,
               AppSettings()->mcuSec,
               AppSettings()->simSec,
               AppSettings()->mcu->PC);
      setConsoleTitle(title);

      lastTitleRefresh = clock();
      cyclesFromLastRefresh = AppSettings()->mcu->cycles;
    }
  }

  if (AppSettings()->mcu->errid != E_NOERRORS && AppSettings()->pauseOnError)
    fprintf(AppSettings()->errorOut, getError(AppSettings()->mcu));

  /* Zapaiętaj czas działania, aby go uzyć go przy ponownym uruchomieniu */
  AppSettings()->simSyncTimeBeforeStop = AppSettings()->simSec;
  AppSettings()->simTimeBeforeStop = AppSettings()->simSec;

  g_MCUThreadRunning = false;
  return NULL;
}

bool step(bool memory)
{
  bool ret;
  bool outSth;
  BYTE outByte;

  char* asmCode = disassembler(AppSettings()->mcu,
                               AppSettings()->mcu->PC, AppSettings()->format, NULL);
  ret = processMCUEx(AppSettings()->mcu, &outByte, NULL, &outSth, NULL);

  if (memory) {
    print("%s"
          C_FWHITE
          C_NBOLD "IDATA[" C_BOLD "%.2Xh" C_NBOLD "]=" C_BOLD "%.2X "
          C_NBOLD "XDATA[" C_BOLD "%.4Xh" C_NBOLD "]=" C_BOLD "%.2X "
          C_NBOLD "SFR["   C_BOLD "%-4s"  C_NBOLD "]=" C_BOLD "%.2X "
          C_NBOLD "ACC="   C_BOLD "%.2X     "
          C_NBOLD "R["     C_BOLD "%i"    C_NBOLD "]=" C_BOLD "%.2X       "
          C_NBOLD "SP="    C_BOLD "%.2X\n"
          C_NBOLD "CY="    C_BOLD "%i "
          C_NBOLD "AC="    C_BOLD "%i "
          C_NBOLD "F0="    C_BOLD "%i "
          C_NBOLD "OV="    C_BOLD "%i "
          C_NBOLD "F1="    C_BOLD "%i "
          C_NBOLD "P="     C_BOLD "%i  "
          C_NBOLD "DPTR="  C_BOLD "%.4X    "
          C_NBOLD "@DPTR=" C_BOLD "%.4X "
          C_NBOLD "SBUF["  C_BOLD "out"   C_NBOLD "]=" C_BOLD "%.2X  "
          C_NBOLD "SBUF["  C_BOLD "in"    C_NBOLD "]=" C_BOLD "%.2X\n"
          C_RESET,
          asmCode,
          AppSettings()->mcu->changedIntRAM,
          AppSettings()->mcu->idata[AppSettings()->mcu->changedIntRAM],
          AppSettings()->mcu->changedExtRAM,
          AppSettings()->mcu->xdata[AppSettings()->mcu->changedExtRAM],
          AppSettings()->mcu->SFRNames[AppSettings()->mcu->changedSFR],
          AppSettings()->mcu->sfr[AppSettings()->mcu->changedSFR - 0x80],
          *AppSettings()->mcu->ACC,
          (checkRegister(AppSettings()->mcu, PSW_RS0) << 1 |
           checkRegister(AppSettings()->mcu, PSW_RS1)) << 3,
          *AppSettings()->mcu->R,
          *AppSettings()->mcu->SP,
          (*AppSettings()->mcu->PSW & 0x80) > 0 ? 1 : 0,
          (*AppSettings()->mcu->PSW & 0x40) > 0 ? 1 : 0,
          (*AppSettings()->mcu->PSW & 0x20) > 0 ? 1 : 0,
          (*AppSettings()->mcu->PSW & 0x04) > 0 ? 1 : 0,
          (*AppSettings()->mcu->PSW & 0x02) > 0 ? 1 : 0,
          (*AppSettings()->mcu->PSW & 0x01) > 0 ? 1 : 0,
          *AppSettings()->mcu->DPTR,
          AppSettings()->mcu->xdata[*AppSettings()->mcu->DPTR],
          *AppSettings()->mcu->SBUF,
          AppSettings()->mcu->OUTPUT == -1 ? 0 : AppSettings()->mcu->OUTPUT);

    if (outSth) {
      if (outByte >= 32 && outByte <= 126)
        print("Send byte: %.2X (%c)\n", outByte, outByte);
      else
        print("Send byte: %.2X\n", outByte);
    }
  } else {
    print("%s", asmCode, AppSettings()->out);
  }

  free(asmCode);
  return ret;
}

void hex(void* memory, WORD start, WORD stop, int offset)
{
  int i = start & 0xFFF0;

  if (i >= stop)
    return;

  for (; i <= stop; i += 0x10) {
    int j;

    print(C_BOLD C_FWHITE "0x%.4X " C_RESET, i);

    for (j = 0; j <= 0xF; ++j) {
      BYTE byte = *(BYTE*)(memory + (i -  offset) + j);
      byte > 0 ? print(C_FWHITE) : print(C_BOLD C_FRED);

      print("%.2X ", byte);
      print(C_RESET);
    }

    for (j = 0; j <= 0xF; ++j) {
      BYTE byte = *(BYTE*)(memory + (i -  offset) + j);
      byte > 0 ? print(C_FWHITE) : print(C_BOLD C_FRED);

      if (byte >= 32 && byte <= 126)
        print("%c", byte);
      else
        print(".");

      print(C_RESET);
    }
    print("\n");
  }
}

void cmd_stop(int argc, char** argv)
{
  if (g_MCUThreadRunning) {
    AppSettings()->stopThread = true;
    pthread_join(g_MCUThread, NULL);
  }
}

void cmd_run(int argc, char** argv)
{
  STOP_IF_THREAD_RUN(return);

  if (argc >= 2) {
    int i = 0;
    int n = atoi(argv[1]);
    for (i = 0; i < n; ++i)
      if (!processMCUEx(AppSettings()->mcu, NULL, NULL, NULL, NULL))
        break;

    printf(g_stoped, AppSettings()->mcu->PC, i);
  } else {
    pthread_create(&g_MCUThread, NULL, (void*)(void*)runMCU, NULL);
  }
}

void cmd_terminal(int argc, char** argv)
{
  STOP_IF_THREAD_RUN(return);

  pthread_create(&g_keyEventLoop, NULL, &keyEventLoop, NULL);
  runMCU(&defputchar);
  AppSettings()->stopThread = true;
  pthread_join(g_keyEventLoop, NULL);

  if (AppSettings()->out == AppSettings()->defaultOut)
    putchar('\n');
}

void cmd_hexoutput(int argc, char** argv)
{
  pthread_create(&g_keyEventLoop, NULL, &keyEventLoop, NULL);
  runMCU(&hexputchar);
  AppSettings()->stopThread = true;
  pthread_join(g_keyEventLoop, NULL);
  AppSettings()->stopThread = false;

  if (AppSettings()->out == AppSettings()->defaultOut)
    putchar('\n');
}

void cmd_trace(int argc, char** argv)
{
  STOP_IF_THREAD_RUN(return);
  REQUIRED_ARGS(1, return);

  bool moreInfo = false;
  if (argc >= 3)
    moreInfo = boolQuestion(argv[2], "y", "n", NULL);

  int i = 0;
  int n = atoi(argv[1]);
  for (i = 0; i < n; ++i)
    if (!step(moreInfo))
      break;

  print(g_stoped, AppSettings()->mcu->PC, i);
}

void cmd_cycles(int argc, char** argv)
{
  print("cycles = %llu\n", AppSettings()->mcu->cycles);
}

void cmd_oscillator(int argc, char** argv)
{
  if (argc >= 2)
    sscanf(argv[1], "%u[ ][Hz]", &AppSettings()->mcu->oscillator);
  else
    print("XTAL = %u Hz = %.4f MHz\n", AppSettings()->mcu->oscillator,
          (double)AppSettings()->mcu->oscillator / 1000000);

  AppSettings()->simSyncTimeBeforeStop = getMCUTime(AppSettings()->mcu);
}

void cmd_step(int argc, char** argv)
{
  STOP_IF_THREAD_RUN(return);
  step(true);
}

void cmd_reset(int argc, char** argv)
{
  STOP_IF_THREAD_RUN(return);
  resetMCU(AppSettings()->mcu);
  AppSettings()->simTimeBeforeStop = 0;
  AppSettings()->simSyncTimeBeforeStop = 0;
}

void cmd_state(int argc, char** argv)
{
  print("Cycles: " C_BOLD C_FWHITE "%llu" C_RESET "\n",
        AppSettings()->mcu->cycles);

  print("Oscillator clocks: " C_BOLD C_FWHITE "%llu" C_RESET "\n",
        AppSettings()->mcu->cycles * 12);

  print("Oscillator frequency: " C_BOLD C_FWHITE "%u Hz" C_RESET "\n",
        AppSettings()->mcu->oscillator);

  print("Executed instructions: " C_BOLD C_FWHITE "%llu" C_RESET "\n",
        AppSettings()->mcu->instructions);

  print("Run time: " C_BOLD C_FWHITE "%.8f seconds" C_RESET "\n",
        getMCUTime(AppSettings()->mcu));

  print("Sim time: " C_BOLD C_FWHITE "%.8f seconds" C_RESET "\n",
        AppSettings()->simSec);

  print("Max value of stack pointer: " C_BOLD C_FWHITE "0x%X" C_RESET "\n",
        AppSettings()->mcu->maxValueOfStackPointer);

  print("Highest used internal ROM address: " C_BOLD C_FWHITE "0x%.4X" C_RESET "\n",
        AppSettings()->mcu->maxIntRomAddress);

  print("Highest used external ROM address: " C_BOLD C_FWHITE "0x%.4X" C_RESET "\n",
        AppSettings()->mcu->maxExtRomAddress);

  print("Highest used IDATA address: ");
  if (AppSettings()->mcu->maxIntRamAddress == -1)
    print(C_BOLD C_FWHITE "Not used." C_RESET "\n");
  else
    print(C_BOLD C_FWHITE "0x%.2X" C_RESET "\n",
          AppSettings()->mcu->maxIntRamAddress);

  print("Lowest used IDATA address: ");
  if (AppSettings()->mcu->minIntRamAddress == -1)
    print(C_BOLD C_FWHITE "Not used." C_RESET "\n");
  else
    print(C_BOLD C_FWHITE "0x%.2X" C_RESET "\n",
          AppSettings()->mcu->minIntRamAddress);

  print("Highest used XDATA address: ");
  if (AppSettings()->mcu->maxExtRamAddress == -1)
    print(C_BOLD C_FWHITE "Not used." C_RESET "\n");
  else
    print(C_BOLD C_FWHITE "0x%.4X" C_RESET "\n",
          AppSettings()->mcu->maxExtRamAddress);

  print("Lowest used XDATA address: ");
  if (AppSettings()->mcu->minExtRamAddress == -1)
    print(C_BOLD C_FWHITE "Not used." C_RESET "\n");
  else
    print(C_BOLD C_FWHITE "0x%.4X" C_RESET "\n",
          AppSettings()->mcu->minExtRamAddress);

  print("Available internal ROM memory: " C_BOLD C_FWHITE "%.2f kB" C_RESET "\n",
        (double) AppSettings()->mcu->iromMemorySize / 1024);

  print("Available external ROM memory: " C_BOLD C_FWHITE "%.2f kB" C_RESET "\n",
        (double) AppSettings()->mcu->xromMemorySize / 1024);

  print("Available IDATA memory: " C_BOLD C_FWHITE "%i bytes" C_RESET "\n",
        AppSettings()->mcu->idataMemorySize);

  print("Available XDATA memory: " C_BOLD C_FWHITE "%.2f kB" C_RESET "\n",
        (double) AppSettings()->mcu->xdataMemorySize / 1024);

  if (checkRegister(AppSettings()->mcu, PCON_IDL))
    print("Microcontroller state: " C_BOLD C_FWHITE "IDLE" C_RESET "");
  else if (checkRegister(AppSettings()->mcu, PCON_PD))
    print("Microcontroller state: " C_BOLD C_FWHITE "POWER DOWN" C_RESET "");
  else
    print("Microcontroller state: " C_BOLD C_FWHITE "OK" C_RESET "");

  print("\n");
}

void cmd_load(int argc, char** argv)
{
  STOP_IF_THREAD_RUN(return);
  REQUIRED_ARGS(2, return);

  bool valid = false;
  WORD highestAddress = 0;
  MemoryType memType = stringToMemory(argv[1]);

  if (memType == IROM) {
    resetMCU(AppSettings()->mcu);

    loadIntelHexFile(argv[2], AppSettings()->mcu->irom,
                     0, AppSettings()->mcu->iromMemorySize - 1, &valid, &highestAddress);

    /* load to int ram if no memory */
    if (highestAddress >= AppSettings()->mcu->iromMemorySize) {
      loadIntelHexFile(argv[2], AppSettings()->mcu->xrom,
                       AppSettings()->mcu->iromMemorySize, AppSettings()->mcu->xromMemorySize - 1, &valid, NULL);
    }

    AppSettings()->simTimeBeforeStop = 0;
  } else if (memType == XROM) {
    resetMCU(AppSettings()->mcu);
    loadIntelHexFile(argv[2], AppSettings()->mcu->xrom, 0, AppSettings()->mcu->xromMemorySize - 1, &valid, NULL);
    AppSettings()->simTimeBeforeStop = 0;
  } else {
    fprintf(AppSettings()->errorOut, "Specify memory type. Choose `%s` or `%s`!\n",
            memoryToString(IROM), memoryToString(XROM));
  }

  if (!valid)
    fprintf(AppSettings()->errorOut, "File not loaded corectly!\n");

}

void cmd_pc(int argc, char** argv)
{
  if (argc >= 2) {
    bool valid;
    int address = hextoi(argv[1], 0x0, 0xFFFF, 0x0, &valid);

    if (!valid)
      fprintf(AppSettings()->errorOut, "Address %s is invalid.\n", argv[1]);
    else
      AppSettings()->mcu->PC = address;
  } else
    print("PC = %.4X\n", (unsigned)AppSettings()->mcu->PC);
}

void cmd_deasm(int argc, char** argv)
{
  REQUIRED_ARGS(1, return);

  bool valid;
  int from = argc >= 2 ? hextoi(argv[1], 0x0, 0xFFFF, 0x0, &valid) : 0;
  int lines = argc >= 3 ? atoi(argv[2]) : 0x10000;

  if (!valid) {
    fprintf(AppSettings()->errorOut, "Invalid argument.\n");
    return;
  }

  int ip = 0;
  for (ip = 0; ip < from; ip += AppSettings()->mcu->byteCount[*ROM(AppSettings()->mcu, ip)]);

  if (ip != from)
    printf("%.4Xh probably is not valid position.\n", from);

  for (int i = 0; i < lines; ++i) {
    char* asmCode = disassembler(AppSettings()->mcu, from, AppSettings()->format, &from);
    print(asmCode);
    free(asmCode);
  }

  if (AppSettings()->out == AppSettings()->defaultOut)
    putchar('\n');
}

void cmd_byte(int argc, char** argv)
{
  REQUIRED_ARGS(2, return);

  MemoryType memType = stringToMemory(argv[1]);

  bool valid;
  char* memory;
  BYTE* byte = NULL;
  WORD address = 0;

  if (memType == IDATA) {
    address = hextoi(argv[2], 0x0, 0xFF, 0x0, &valid);
    byte = &AppSettings()->mcu->idata[address];
    memory = "IDATA";

    if (!valid) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }
  } else if (memType == XDATA) {
    address = hextoi(argv[2], 0x0, 0xFFFF, 0x0, &valid);
    byte = &AppSettings()->mcu->xdata[address];
    memory = "XDATA";

    if (!valid) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }
  } else if (memType == IROM) {
    address = hextoi(argv[2], 0x0, 0xFFFF, 0x0, &valid);
    byte = &AppSettings()->mcu->irom[address];
    memory = "IROM";

    if (!valid) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }
  } else if (memType == XROM) {
    address = hextoi(argv[2], 0x0, 0xFFFF, 0x0, &valid);
    byte = &AppSettings()->mcu->xrom[address];
    memory = "XROM";

    if (!valid) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }
  } else if (memType == SFR) {
    bool found = false;
    for (int i = 0x80; i < 0x100; ++i) {
      if (0 == stricmp(argv[2], AppSettings()->mcu->SFRNames[i])) {
        address = i;
        found = true;
        break;
      }
    }

    if (!found)
      address = hextoi(argv[2], 0x80, 0xFF, 0x80, &valid);

    if (!found && !valid) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }

    byte = &AppSettings()->mcu->sfr[address - 0x80];
    memory = "SFR";
  }

  if (byte != NULL) {
    if (argc >= 4) {
      bool valid;
      int value = hextoi(argv[3], 0x0, 0xFF, 0x0, &valid);

      if (!valid) {
        fprintf(AppSettings()->errorOut, "Invalid argument.");
        return;
      }

      *byte = value;
    } else {
      print("%s[%.2X] = %.2X\n", memory, address, *byte);
    }
  }
}

void cmd_bit(int argc, char** argv)
{
  REQUIRED_ARGS(3, return);

  MemoryType memType = stringToMemory(argv[1]);
  BYTE bit = 1 << atoi(argv[3]);

  bool valid;
  char* memory;
  BYTE* byte = NULL;
  WORD address = 0;

  if (memType == IDATA) {
    address = hextoi(argv[2], 0x0, 0xFF, 0x0, &valid);
    byte = &AppSettings()->mcu->idata[address];
    memory = "IDATA";

    if (!valid) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }
  } else if (memType == XDATA) {
    address = hextoi(argv[2], 0x0, 0xFFFF, 0x0, &valid);
    byte = &AppSettings()->mcu->xdata[address];
    memory = "XDATA";

    if (!valid) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }
  } else if (memType == IROM) {
    address = hextoi(argv[2], 0x0, 0xFFFF, 0x0, &valid);
    byte = &AppSettings()->mcu->irom[address];
    memory = "ROM";

    if (!valid) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }
  } else if (memType == XROM) {
    address = hextoi(argv[2], 0x0, 0xFFFF, 0x0, &valid);
    byte = &AppSettings()->mcu->xrom[address];
    memory = "ROM";

    if (!valid) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }
  } else if (memType == SFR) {
    bool found = false;
    for (int i = 0x80; i < 0x100; ++i) {
      if (0 == stricmp(argv[2], AppSettings()->mcu->SFRNames[i])) {
        address = i;
        found = true;
        break;
      }
    }

    if (!found)
      address = hextoi(argv[2], 0x80, 0xFF, 0x80, &valid);

    if (!found && !valid) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }

    byte = &AppSettings()->mcu->sfr[address - 0x80];
    memory = "SFR";
  }

  if (byte != NULL) {
    if (argc >= 5) {
      bool valid;
      bool answer = boolQuestion(argv[4], "1", "0", &valid);

      if (!valid)
        fprintf(AppSettings()->errorOut, "Invalid argument.\n");
      else {
        if (answer)
          *byte |= bit;
        else
          *byte &= ~bit;
      }
    } else {
      print("%s[%.2X.%i] = ", memory, address, atoi(argv[3]));
      if(*byte & bit)
        print("true\n");
      else
        print("false\n");
    }
  }
}

void cmd_hex(int argc, char** argv)
{
  REQUIRED_ARGS(1, return);

  MemoryType memType = stringToMemory(argv[1]);
  int start = 0;
  int stop = 0;

  if (memType == IDATA) {
    start = argc >= 3 ? hextoi(argv[2], 0x0, 0xFF, 0x0, NULL) : 0x0;
    stop = argc >= 4 ? hextoi(argv[3], start, 0xFF, start, NULL) : 0xFF;

    hex(AppSettings()->mcu->idata, start, stop, 0);
  } else if (memType == XDATA) {
    start = argc >= 3 ? hextoi(argv[2], 0x0, 0xFFFF, 0x0, NULL) : 0x0;
    stop = argc >= 4 ? hextoi(argv[3], start, 0xFFFF, start, NULL) : 0xFFFF;

    hex(AppSettings()->mcu->xdata, start, stop, 0);
  } else if (memType == IROM) {
    start = argc >= 3 ? hextoi(argv[2], 0x0, 0xFFFF, 0x0, NULL) : 0x0;
    stop = argc >= 4 ? hextoi(argv[3], start, 0xFFFF, start, NULL) : 0xFFFF;

    hex(AppSettings()->mcu->irom, start, stop, 0);
  } else if (memType == XROM) {
    start = argc >= 3 ? hextoi(argv[2], 0x0, 0xFFFF, 0x0, NULL) : 0x0;
    stop = argc >= 4 ? hextoi(argv[3], start, 0xFFFF, start, NULL) : 0xFFFF;

    hex(AppSettings()->mcu->xrom, start, stop, 0);
  } else if (memType == SFR) {
    start = argc >= 3 ? hextoi(argv[2], 0x80, 0xFF, 0x0, NULL) : 0x80;
    stop = argc >= 4 ? hextoi(argv[3], start, 0xFF, start, NULL) : 0xFF;

    hex(AppSettings()->mcu->sfr, start, stop, 0x80);
  } else {
    fputs("Undefined memory type.\n", AppSettings()->out);
  }
}

void cmd_fill(int argc, char** argv)
{
  REQUIRED_ARGS(2, return);

  MemoryType memType = stringToMemory(argv[1]);
  int start = 0;
  int stop = 0;

  bool valid1;
  bool valid2;
  bool valid3;

  if (memType == IDATA) {
    start = argc >= 4 ? hextoi(argv[3], 0x0, 0xFF, 0x0, &valid1) : 0x0;
    stop = argc >= 5 ? hextoi(argv[4], start, 0xFF, start, &valid2) : 0xFF;
    int address = hextoi(argv[2], 0x0, 0xFF, 0x0, &valid3);

    if (!valid1 || !valid2 || !valid3) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }

    memset(AppSettings()->mcu->idata + start, address, stop - start);
  } else if (memType == XDATA) {
    start = argc >= 4 ? hextoi(argv[3], 0x0, 0xFFFF, 0x0, &valid1) : 0x0;
    stop = argc >= 5 ? hextoi(argv[4], start, 0xFFFF, start, &valid2) : 0xFFFF;
    int address = hextoi(argv[2], 0x0, 0xFF, 0x0, &valid3);

    if (!valid1 || !valid2 || !valid3) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }

    memset(AppSettings()->mcu->xdata + start, address, stop - start);
  } else if (memType == IROM) {
    start = argc >= 4 ? hextoi(argv[3], 0x0, 0xFFFF, 0x0, &valid1) : 0x0;
    stop = argc >= 5 ? hextoi(argv[4], start, 0xFFFF, start, &valid2) : 0xFFFF;
    int address = hextoi(argv[2], 0x0, 0xFF, 0x0, &valid3);

    if (!valid1 || !valid2 || !valid3) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }

    memset(AppSettings()->mcu->irom + start, address, stop - start);;
  } else if (memType == XROM) {
    start = argc >= 4 ? hextoi(argv[3], 0x0, 0xFFFF, 0x0, &valid1) : 0x0;
    stop = argc >= 5 ? hextoi(argv[4], start, 0xFFFF, start, &valid2) : 0xFFFF;
    int address = hextoi(argv[2], 0x0, 0xFF, 0x0, &valid3);

    if (!valid1 || !valid2 || !valid3) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }

    memset(AppSettings()->mcu->xrom + start, address, stop - start);;
  } else if (memType == SFR) {
    start = argc >= 4 ? hextoi(argv[3], 0x80, 0xFF, 0x0, &valid1) : 0x80;
    stop = argc >= 5 ? hextoi(argv[4], start, 0xFF, start, &valid2) : 0xFF;
    int address = hextoi(argv[2], 0x80, 0xFF, 0x80, &valid3);

    if (!valid1 || !valid2 || !valid3) {
      fprintf(AppSettings()->errorOut, "Invalid argument.");
      return;
    }

    start -= 0x80;
    stop -= 0x80;

    memset(AppSettings()->mcu->sfr + start, address, stop - start);
  }
}

void cmd_break(int argc, char** argv)
{
  REQUIRED_ARGS(1, return);

  for (int i = 1; i < argc; ++i) {
    bool valid;
    int address = hextoi(argv[i], 0x0, 0xFFFF, 0x0, &valid);

    if (!valid)
      fprintf(AppSettings()->errorOut, "Address %s is invalid.\n", argv[i]);
    else
      setPCBreakpoint(AppSettings()->mcu, address);
  }
}

void cmd_access(int argc, char** argv)
{
  REQUIRED_ARGS(2, return);
  MemoryType memType = stringToMemory(argv[1]);

  for (int i = 2; i < argc; ++i) {
    bool valid;
    int address = hextoi(argv[i], 0x0, 0xFFFF, 0x0, &valid);

    if (!valid)
      fprintf(AppSettings()->errorOut, "Address %s is invalid.\n", argv[i]);
    else
      setAccessPause(AppSettings()->mcu, memType, address);
  }
}

void cmd_cond(int argc, char** argv)
{
  REQUIRED_ARGS(4, return);
  MemoryType memType = stringToMemory(argv[1]);
  BreakpointType operatorType = stringToOperator(argv[3]);

  bool valid;
  int value = hextoi(argv[2], 0x0, 0xFF, 0x0, &valid);

  if (!valid)
    fprintf(AppSettings()->errorOut, "Value %s is invalid.\n", argv[2]);
  else {
    for (int i = 4; i < argc; ++i) {
      int address = hextoi(argv[i], 0x0, 0xFFFF, 0x0, &valid);

      if (!valid)
        fprintf(AppSettings()->errorOut, "Address %s is invalid.\n", argv[i]);
      else
        setCondPause(AppSettings()->mcu, memType, address, operatorType, value);
    }
  }
}

void cmd_clear(int argc, char** argv)
{
  REQUIRED_ARGS(1, return);

  for (int i = 1; i < argc; ++i) {
    bool valid;
    int address = hextoi(argv[i], 0x0, 0xFFFF, 0x0, &valid);

    if (!valid)
      fprintf(AppSettings()->errorOut, "Address %s is invalid.\n", argv[i]);
    else
      clearPCBreakpoint(AppSettings()->mcu, address);
  }
}

void cmd_accessclear(int argc, char** argv)
{
  REQUIRED_ARGS(2, return);
  MemoryType memType = stringToMemory(argv[1]);

  for (int i = 2; i < argc; ++i) {
    bool valid;
    int address = hextoi(argv[i], 0x0, 0xFFFF, 0x0, &valid);

    if (!valid)
      fprintf(AppSettings()->errorOut, "Address %s is invalid.\n", argv[i]);
    else
      clearAccessPause(AppSettings()->mcu, memType, address);
  }
}

void cmd_clearcond(int argc, char** argv)
{
  REQUIRED_ARGS(2, return);
  MemoryType memType = stringToMemory(argv[1]);

  for (int i = 2; i < argc; ++i) {
    bool valid;
    int address = hextoi(argv[i], 0x0, 0xFFFF, 0x0, &valid);

    if (!valid)
      fprintf(AppSettings()->errorOut, "Address %s is invalid.\n", argv[i]);
    else
      clearCondPause(AppSettings()->mcu, memType, address);
  }
}

void cmd_clearall(int argc, char** argv)
{
  clearAllBreakpointsAndPauses(AppSettings()->mcu);
}

void cmd_breakpoints(int argc, char** argv)
{
  int i = 0;

  print(C_BOLD C_FWHITE "PC breakpoints:" C_RESET "\n");
  for (i = 0; i < AppSettings()->mcu->numOfPCBreakpoints; ++i)
    print("%.4Xh\t",
          AppSettings()->mcu->PCBreakpoints[i]);
  print("\n");

  print(C_BOLD C_FWHITE "Memory access pauses:" C_RESET "\n");
  print(C_BOLD C_FWHITE "* Internal RAM:" C_RESET "\n");
  for (i = 0; i < AppSettings()->mcu->numOfaccessIntRAMPauses; ++i)
    print("%.2Xh\t",
          AppSettings()->mcu->accessIntRAMPauses[i]);
  print("\n");

  print(C_BOLD C_FWHITE "* External RAM:" C_RESET "\n");
  for (i = 0; i < AppSettings()->mcu->numOfAccessExtRAMPauses; ++i)
    print("%.4Xh\t",
          AppSettings()->mcu->accessExtRAMPauses[i]);
  print("\n");

  print(C_BOLD C_FWHITE "* SFR:" C_RESET "\n");
  for (i = 0; i < AppSettings()->mcu->numOfAccessSFRPauses; ++i)
    print("%.2Xh\t",
          AppSettings()->mcu->accessSFRPauses[i]);
  print("\n");

  print(C_BOLD C_FWHITE "* Internal ROM:" C_RESET "\n");
  for (i = 0; i < AppSettings()->mcu->numOfAccessIntROMPauses; ++i)
    print("%.4Xh\t",
          AppSettings()->mcu->accessIntROMPauses[i]);
  print("\n");

  print(C_BOLD C_FWHITE "* External ROM:" C_RESET "\n");
  for (i = 0; i < AppSettings()->mcu->numOfAccessExtROMPauses; ++i)
    print("%.4Xh\t",
          AppSettings()->mcu->accessExtROMPauses[i]);
  print("\n");

  print(C_BOLD C_FWHITE "Conditional pauses:" C_RESET "\n");
  print(C_BOLD C_FWHITE "* Internal RAM:" C_RESET "\n");
  for (i = 0; i < AppSettings()->mcu->numOfIntRAMPauses; ++i)
    print("%.2Xh\t",
          AppSettings()->mcu->intRAMPauses[i].address);
  print("\n");

  print(C_BOLD C_FWHITE "* External RAM:" C_RESET "\n");
  for (i = 0; i < AppSettings()->mcu->numOfExtRAMPauses; ++i)
    print("%.4Xh\t",
          AppSettings()->mcu->extRAMPauses[i].address);
  print("\n");

  print(C_BOLD C_FWHITE "* SFR:" C_RESET "\n");
  for (i = 0; i < AppSettings()->mcu->numOfSFRPauses; ++i)
    print("%.2Xh\t",
          AppSettings()->mcu->SFRPauses[i].address);
  print("\n");
}

void cmd_realtime(int argc, char** argv)
{
  REQUIRED_ARGS(1, return);

  bool valid;
  bool answer = boolQuestion(argv[1], "y", "n", &valid);

  if (valid)
    AppSettings()->realtime = answer;
  else
    fprintf(AppSettings()->errorOut, "Invalid argument.\n");
}

void cmd_auto_ti(int argc, char** argv)
{
  REQUIRED_ARGS(1, return);

  bool valid;
  bool answer = boolQuestion(argv[1], "y", "n", &valid);

  if (valid)
    AppSettings()->mcu->autoRead = answer;
  else
    fprintf(AppSettings()->errorOut, "Invalid argument.\n");
}

void cmd_auto_ri(int argc, char** argv)
{
  REQUIRED_ARGS(1, return);

  bool valid;
  bool answer = boolQuestion(argv[1], "y", "n", &valid);

  if (valid)
    AppSettings()->mcu->autoWrite = answer;
  else
    fprintf(AppSettings()->errorOut, "Invalid argument.\n");
}

void cmd_format(int argc, char** argv)
{
  REQUIRED_ARGS(1, return);
  AppSettings()->format = argv[1];
}

void cmd_int0(int argc, char** argv)
{
  REQUIRED_ARGS(1, return);

  bool valid, state;
  state = boolQuestion(argv[1], "1", "0", &valid);

  if (valid)
    setRegister(AppSettings()->mcu, P3_INT0, state);
  else
    fprintf(AppSettings()->errorOut, "Invalid argument.\n");
}

void cmd_int1(int argc, char** argv)
{
  REQUIRED_ARGS(1, return);

  bool valid, state;
  state = boolQuestion(argv[1], "1", "0", &valid);

  if (valid)
    setRegister(AppSettings()->mcu, P3_INT1, state);
  else
    fprintf(AppSettings()->errorOut, "Invalid argument.\n");
}

void cmd_pauseonerror(int argc, char** argv)
{
  REQUIRED_ARGS(1, return);

  bool valid, state;
  state = boolQuestion(argv[1], "y", "n", &valid);

  if (valid)
    AppSettings()->pauseOnError = state;
  else
    fprintf(AppSettings()->errorOut, "Invalid argument.\n");
}

void cmd_ea(int argc, char** argv)
{
  REQUIRED_ARGS(1, return);

  bool valid, state;
  state = boolQuestion(argv[1], "1", "0", &valid);

  if (valid)
    AppSettings()->mcu->EA = state;
  else
    fprintf(AppSettings()->errorOut, "Invalid argument.\n");
}

void cmd_connect_ea(int argc, char** argv)
{
  REQUIRED_ARGS(2, return);

  int port = atoi(argv[1]);
  int pin = atoi(argv[2]);
  BYTE byte = 0;

  if (port < 0 || port > 3) {
    fprintf(AppSettings()->errorOut, "Port should be between 0 and 3.\n");
    return;
  }

  if (pin < 0 || pin > 7) {
    fprintf(AppSettings()->errorOut, "Byte should be between 0 and 7.\n");
    return;
  }

  if (port == 0)
    byte = 0x80 | pin;
  else if (port == 1)
    byte = 0x90 | pin;
  else if (port == 2)
    byte = 0xA0 | pin;
  else if (port == 3)
    byte = 0xB0 | pin;

  AppSettings()->mcu->EAconnect = byte;
}

#ifndef NDEBUG
void cmd_exitkey(int argc, char** argv)
{
  printf("Press key: ");
  fflush(AppSettings()->defaultOut);
#ifdef __linux__
  initKeyboard();
#endif
  AppSettings()->exitkey = getKey();
  printf("%#.2x", AppSettings()->exitkey);
  putchar('\n');
#ifdef __linux__
  closeKeyboard();
#endif
}

void cmd_unsetexitkey(int argc, char** argv)
{
  AppSettings()->exitkey = -1;
}

void cmd_system(int argc, char** argv)
{
  REQUIRED_ARGS(1, return);

  char* arg;
  int args = 0;
  int i = 0;
  int n = 0;

  for (i = 1; i < argc; ++i)
    args += strlen(argv[i]) + 1;
  arg = malloc(args + 1);

  for (i = 1; i <argc; ++i) {
    strcpy(&arg[n], argv[i]);
    n += strlen(argv[i]) + 1;
    arg[n - 1] = ' ';
  }
  arg[n - 1] = '\0';

  system(arg);
  free(arg);
}

#endif // NDEBUG

void freeArg(int argc, char** argv)
{
  int i = 0;
  for (i = 0; i < argc; ++i)
    free(argv[i]);
  free(argv);
}

void runDebugger(void)
{
  g_MCUThreadRunning = false;
  g_keyEventLoopRunning = false;

  /*
   * Tablica komend.
   */
  const struct _params params[] = {
    {
      "run", &cmd_run, "[n]",
      "Run n instructions. If n not given run microcontroller indefinitely "
      "in background."
    },
    {
      "output", &cmd_terminal, "",
      "Show program SBUF output. You can define key which can be used to exit "
      "simulator. To define key use 'exitkey key'."
    },
    {
      "terminal", &cmd_terminal, "",
      "Equivalent to 'output'."
    },
    {
      "hexOutput", &cmd_hexoutput, "",
      "Works the same as 'output' but display output in hex."
    },
    {
      "hexTerminal", &cmd_hexoutput, "",
      "Equivalent to 'hexoutput'."
    },
    {
      "trace", &cmd_trace, "n [y|n]",
      "Display executed instructions. Second argument indicated if show more "
      "information."
    },
    {
      "stop", &cmd_stop, "",
      "Stop."
    },
    {
      "step", &cmd_step, "",
      "Step."
    },
    {
      "cycles", &cmd_cycles, "",
      "Display cycles."
    },
    {
      "oscillator", &cmd_oscillator, "[Hz]",
      "Get/set oscillator sped in Hz."
    },
    {
      "xtal", &cmd_oscillator, "[Hz]",
      "Equivalent to 'oscillator'"
    },
    {
      "reset", &cmd_reset, "",
      "Reset microcontroller."
    },
    {
      "state", &cmd_state, "",
      "State of microcontroller."
    },
    {
      "load", &cmd_load, "mem file",
      "Load file into ROM."
    },
    {
      "pc", &cmd_pc, "[addr]",
      "Set/get PC."
    },
    {
      "deAsm", &cmd_deasm, "[start [lines]]",
      "Disassemble code."
    },
    {
      "disassemble", &cmd_deasm, "[start [lines]]",
      "Equivalent to 'deasm'"
    },
    {
      "byte", &cmd_byte, "mem addr [data]",
      "Set/get byte in/from memory."
    },
    {
      "bit", &cmd_bit, "mem addr bit [0|1]",
      "Set/get bit in/from memory."
    },
    {
      "fill", &cmd_fill, "mem value [start [stop]]",
      "Fill bytes in memory"
    },
    {
      "hex", &cmd_hex, "mem addr [from [size]]",
      "Show memory."
    },
    {
      "break", &cmd_break, "addr ...",
      "Set PC breakpoint."
    },
    {
      "breakpoint", &cmd_break, "addr ...",
      "Equivalent to 'break'"
    },
    {
      "accessPause", &cmd_access, "mem addr ...",
      "Set condition to pause run trace when access to selected memory."
    },
    {
      "access", &cmd_access, "mem addr ...",
      "Equivalent to 'AccessPause'."
    },
    {
      "conditionPause", &cmd_cond, "mem value operator addr ...",
      "Set condition to pause run trace."
    },
    {
      "cond", &cmd_cond, "mem value operator addr ...",
      "Equivalent to 'conditionalBreakpoint'."
    },
    {
      "clear", &cmd_clear, "addr ...",
      "Clear PC breakpoints."
    },
    {
      "clearAccess", &cmd_accessclear, "mem addr ...",
      "Clear access breakpoints."
    },
    {
      "clearCond", &cmd_clearcond, "mem addr ...",
      "Clear conditional pauses."
    },
    {
      "clearAll", &cmd_clearall, "",
      "Clear all breakpoints and coditional pauses."
    },
    {
      "breakpoints", &cmd_breakpoints, "",
      "Show all breakpoints and conditional pauses."
    },
    {
      "pauses", &cmd_breakpoints, "",
      "Equivalent to 'breakpoints'."
    },
    {
      "realtime", &cmd_realtime, "[y|n]",
      "Run simulator in realtime."
    },
    {
      "auto_ti", &cmd_auto_ti, "[y|n]",
      "Auto read from sbuf. Defult active."
    },
    {
      "auto_ri", &cmd_auto_ri, "[y|n]",
      "Wrtie last presed key to sbuf. Defult active."
    },
    {
      "format", &cmd_format, "format",
      "Change assembly format."
    },
    {
      "int0", &cmd_int0, "[1|0]",
      "Change int0 state."
    },
    {
      "int1", &cmd_int1, "[1|0]",
      "Change int1 state."
    },
    {
      "pauseOnError", &cmd_pauseonerror, "[y|n]",
      "Pause when MCU return error."
    },
    {
      "ea", &cmd_ea, "[1|0]",
      "Change EA pin state."
    },
    {
      "connect_ea", &cmd_connect_ea, "port pin",
      "Connect EA pin to another pin."
    },
#ifndef NDEBUG
    {
      "setExitKey", &cmd_exitkey, "",
      "Set key to stop simulation. Default: Ctrl+D."
    },
    {
      "unsetExitKey", &cmd_unsetexitkey, "",
      "Unset exitkey."
    },
    {
      "system", &cmd_system, "",
      "Run system command."
    },
#endif // NDEBUG
  };
  int numOfParams = sizeof(params) / sizeof(params[0]);

  while (1) {

    /* Wyjście zmieniane jest tylko dla jednej komendy więc je przywraca. */
    AppSettings()->out = AppSettings()->defaultOut;

    /* Tytuł okna konsoli. */
    if (!g_MCUThreadRunning) {
      char title[128];
      snprintf(title, 128, "%s - %s",
               APP_NAME_LONG,
               APP_AUTHOR);
      setConsoleTitle(title);
    }

    /*
     * Wczytywanie komend.
     */
    if (AppSettings()->in == AppSettings()->defaultIn) {
      print(C_PROMPT);
      printf("%s", AppSettings()->commandPrompt);
      print(C_RESET);
    }

    int argc = 0;
    char** argv = NULL;
    size_t i = 0;
    size_t c = 0;
    size_t inputSize = 0;
    bool inQuotes = false;
    bool specialChar = false;
    char input[MAX_LINE];

    if (fgets(input, MAX_LINE, AppSettings()->in) != NULL) {
      inputSize = strlen(input) + 1;

      for (; i < inputSize; ++i) {

        /* Gdy natrafi na jeden z symboli rodzielająch argumenty dodaj \0
           ma końcu ostatniego i ustaw c na 0 aby poinformować, o nowym
           argumencie. */
        if ( ( (!inQuotes && !specialChar) &&
               (input[i] == ' ' || input[i] == '\t')
             ) || (input[i] == '\n' || input[i] == '\0' || input[i] == '#')) {
          if (c > 0) {
            if (argc >= 1) {
              argv[argc - 1] = realloc(argv[argc - 1], c + 1);
              argv[argc - 1][c] = '\0';
            }
            c = 0;
          }

          /* Komentarz. Nie czytaj dalej. */
          if (input[i] == '#')
            break;
        }
        /* Tekst w cudyszłowach, dzięki temu można łatwo używać spacji. */
        else if (!specialChar && input[i] == '"') {
          inQuotes = !inQuotes;
        }
        /* Obsługa znaku "\". WIdomo co robi. */
        else if (!specialChar && input[i] == '\\') {
          specialChar = true;
        } else {
          char in = input[i];

          //if (specialChar && input[i] == 'n')
          //  in = '\n';
          //else if (specialChar && input[i] == 'r')
          //  in = '\r';
          //else if (specialChar && input[i] == 't')
          //  in = '\t';

          if (c == 0) {
            argc += 1;
            argv = realloc(argv, argc * sizeof(char*));
            argv[argc - 1] = NULL;
          }

          argv[argc - 1] = realloc(argv[argc - 1], c + 1);
          argv[argc - 1][c] = in;

          c += 1;
          specialChar = false;
        }
      }
    } else {
      if (feof(AppSettings()->in))
        exit(EXIT_SUCCESS);

      fclose(AppSettings()->in);
      if (AppSettings()->in != AppSettings()->defaultIn)
        AppSettings()->in = AppSettings()->defaultIn;
    }

    /*
     * Przerywa jeśli nic nie wpisano.
     */
    if (argc <= 0)
      continue;

    /*
     * Domyślne komendy: "quit" i "help".
     */
    if (0 == stricmp(argv[0], "quit")) {
      freeArg(argc, argv);
      STOP_IF_THREAD_RUN(continue);
      break;
    }
#ifdef _WIN32
    else if (argv[0][0] == AppSettings()->exitkey) { // Windows...
      freeArg(argc, argv);
      STOP_IF_THREAD_RUN(continue);
      break;
    }
#endif
    else if (0 == stricmp(argv[0], "exec")) {
      if (argc >= 2)
        AppSettings()->in = fopen(argv[1], "r");
      if (AppSettings()->in == NULL)
        AppSettings()->in = AppSettings()->defaultIn;

      freeArg(argc, argv);
      continue;
    } else if (0 == stricmp(argv[0], "help")) {
      print("Available commands:\n");
      print("   " C_FGREEN "%-27s" C_RESET " %s\n", "quit", "Quit.");
      print("   " C_FGREEN "%-27s" C_RESET " %s\n", "exec", "Execute commands from file.");
      print("   " C_FGREEN "%-27s" C_RESET " %s\n", "help", "Show help.");

      for (i = 0; i < numOfParams; ++i) {
        char command[80];
        snprintf(command, 80, "" C_FGREEN "%s " C_BOLD C_FGREEN "%s" C_RESET "",
                 params[i].name, params[i].paramsDesc);
        print("   %-44s %s\n", command, params[i].shortDesc);
      }

      putchar('\n');
      print("* " C_BOLD C_FGREEN "mem"      C_RESET " - Memory type. Types:\n");
      print("  " C_FGREEN "%-8s" C_RESET " - %s\n", memoryToString(IDATA), "Internal RAM.");
      print("  " C_FGREEN "%-8s" C_RESET " - %s\n", memoryToString(XDATA), "External RAM.");
      print("  " C_FGREEN "%-8s" C_RESET " - %s\n", memoryToString(IROM), "Internal ROM.");
      print("  " C_FGREEN "%-8s" C_RESET " - %s\n", memoryToString(XROM), "External ROM.");
      print("  " C_FGREEN "%-8s" C_RESET " - %s\n", memoryToString(SFR), "SFR.");
      print("* " C_BOLD C_FGREEN "addr"     C_RESET " - Addres in selected memory.\n");
      print("* " C_BOLD C_FGREEN "operator" C_RESET " - Avaiable operators:\n");
      print("  " C_FGREEN "e"   C_RESET "   - equal '='\n");
      print("  " C_FGREEN "ne"  C_RESET "  - not equal '!='\n");
      print("  " C_FGREEN "l"   C_RESET "   - less '<'\n");
      print("  " C_FGREEN "le"  C_RESET "  - less equal '<='\n");
      print("  " C_FGREEN "g"   C_RESET "   - greater '>'\n");
      print("  " C_FGREEN "ge"  C_RESET "  - greater equal '>='\n");
      print("  " C_FGREEN "and" C_RESET " - binary and '&'\n");
      print("  " C_FGREEN "or"  C_RESET "  - binary or '|'\n");
      print("  " C_FGREEN "xor" C_RESET " - binary xor '^'\n");

      freeArg(argc, argv);
      continue;
    }

    /*
     * Sprawdza, czy podano inne wyjście.
     */
    for (i = 1; i < argc; ++i) {
      if (0 == strcmp(argv[i], ">") && i < argc - 1) {
        if (AppSettings()->out != AppSettings()->defaultOut)
          fclose(AppSettings()->out);

        AppSettings()->out = fopen (argv[i + 1], "w");

        if (AppSettings()->out == NULL) {
          printf("Can not open file %s.", argv[1]);
          AppSettings()->out = AppSettings()->defaultOut;
        }

        for (int j = i; j < argc; j++)
          free(argv[j]);
        argc = i;
      } else if (0 == strcmp(argv[i], ">>") && i < argc - 1) {
        if (AppSettings()->out != AppSettings()->defaultOut)
          fclose(AppSettings()->out);

        AppSettings()->out = fopen (argv[i + 1], "a");

        if (AppSettings()->out == NULL) {
          printf("Can not open file %s.", argv[1]);
          AppSettings()->out = AppSettings()->defaultOut;
        }

        for (int j = i; j < argc; j++)
          free(argv[j]);
        argc = i;
      }
    }

    /*
     * Ładny komunikat dla wygenerowanych plików.
     */
    if (AppSettings()->out != AppSettings()->defaultOut) {
      time_t rawtime;
      time(&rawtime);
      fprintf(AppSettings()->out,
              "# File generated by:\n"
              "# %s\n"
              "# Copyright (C) 2011 %s\n"
              "# Created at %s"
              "# %s%s\n",
              APP_NAME_LONG,
              APP_AUTHOR,
              ctime(&rawtime),
              AppSettings()->commandPrompt,
              input);
    }

    /*
     * Wykonywanie komend.
     */
    bool find = false;
    for (i = 0; i < numOfParams; ++i) {
      if (0 == stricmp(argv[0], params[i].name)) {
        if (NULL == params[i].function)
          fprintf(AppSettings()->errorOut, "Command `%s` is not available yet.\n", argv[0]);
        else
          params[i].function(argc, argv);
        find = true;
        break;
      }
    }
    if (!find)
      fprintf(AppSettings()->errorOut, "Unrecognized command: `%s`.\n", argv[0]);
    freeArg(argc, argv);
    if (AppSettings()->out != AppSettings()->defaultOut)
      fclose(AppSettings()->out);
  }
}

/*
vi:ts=4:et:nowrap
*/
