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

#ifndef _8051_H_
#define _8051_H_

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t  BYTE;
typedef uint16_t WORD;

extern const int CYCLE_COUNT[];

#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

// PSW
#define PSW_P   0xD001
#define PSW_OV  0xD004
#define PSW_RS0 0xD008
#define PSW_RS1 0xD010
#define PSW_AC  0xD040
#define PSW_CY  0xD080

// IP
#define IP_PX0 0xC001
#define IP_PT0 0xC002
#define IP_PX1 0xC004
#define IP_PT1 0xC008
#define IP_PS  0xC010
#define IP_PT2 0xC020

// P3
#define P3_RD   0xB080
#define P3_WR   0xB040
#define P3_T1   0xB020
#define P3_T0   0xB010
#define P3_INT1 0xB008
#define P3_INT0 0xB004
#define P3_TxD  0xB002
#define P3_RxD  0xB001

// IE
#define IE_EA  0xA880
#define IE_ET2 0xA820
#define IE_ES  0xA810
#define IE_ET1 0xA808
#define IE_EX1 0xA804
#define IE_ET0 0xA802
#define IE_EX0 0xA801

// SCON
#define SCON_SM0 0x9880
#define SCON_SM1 0x9840
#define SCON_SM2 0x9820
#define SCON_REN 0x9810
#define SCON_TB8 0x9808
#define SCON_RB8 0x9804
#define SCON_TI  0x9802
#define SCON_RI  0x9801

// TMOD
#define TMOD_GATE1 0x8980
#define TMOD_CT1   0x8940
#define TMOD_M1_1  0x8920
#define TMOD_M0_1  0x8910
#define TMOD_GATE0 0x8908
#define TMOD_CT0   0x8904
#define TMOD_M1_0  0x8902
#define TMOD_M0_0  0x8901

// TCON
#define TCON_TF1 0x8880
#define TCON_TR1 0x8840
#define TCON_TF0 0x8820
#define TCON_TR0 0x8810
#define TCON_IE1 0x8808
#define TCON_IT1 0x8804
#define TCON_IE0 0x8802
#define TCON_IT0 0x8801

// T2CON
#define T2CON_TF2    0xC880
#define T2CON_EXF2   0xC840
#define T2CON_RCLK   0xC820
#define T2CON_TCLK   0xC810
#define T2CON_EXEN2  0xC808
#define T2CON_TR2    0xC804
#define T2CON_C_T2   0xC802
#define T2CON_CP_RL2 0xC801

// PCON
#define PCON_SMOD 0x8780
#define PCON_GF1  0x8708
#define PCON_GF0  0x8704
#define PCON_PD   0x8702
#define PCON_IDL  0x8701

/*
 * AT89S51
 */
#define AUXR1_DPS  0xA201

// Error id
#define E_NOERRORS 0 // Everything ok!
#define E_IRAMOUTSIDE 1 // Using an address outside the available internal ram memory.
#define E_XRAMOUTSIDE 2 // Using an address outside the available external ram memory.
#define E_UNSUPPORTED 6 // Using unsupported instruction.

#define SFR_SIZE 0x80
#define INT_RAM_SIZE 0x100
#define MAX_EXT_RAM_SIZE 0x10000
#define MAX_ROM_SIZE 0x10000

typedef enum {
  EQUAL = 1,
  NOT_EQUAL,
  LESS,
  LESS_EQUAL,
  GREATER,
  GREATER_EQUAL,
  AND,
  OR,
  XOR,
} BreakpointType;

typedef enum {
  IDATA = 1,
  XDATA,
  SFR,
  IROM,
  XROM,
} MemoryType;

typedef enum {
  M_8031,
  M_8051,
  M_8052,
  M_89S51,
  M_89S52,
} MCUType;

typedef struct {
  WORD address;
  BYTE value;
  BreakpointType type;
} MCUConditionBreakpoint;

typedef struct _mcu {
  WORD PC;
  BYTE lastInstruction;

  MCUType mcuType;
  bool noDebug;

  unsigned long long cycles;
  unsigned long long instructions;
  unsigned oscillator;

  /*
   * Memory.
   */
  BYTE idata[INT_RAM_SIZE];
  BYTE xdata[MAX_EXT_RAM_SIZE];
  BYTE irom[MAX_ROM_SIZE];
  BYTE xrom[MAX_ROM_SIZE];
  BYTE sfr[SFR_SIZE];

  BYTE* currentRom;

  /*
   * Pins
   */
  bool EA;

  /*
   * Funny thing. Connect pin to another pins (exactly register).
   */
  BYTE EAconnect;

  /*
   * Memory size.
   */
  unsigned idataMemorySize;
  unsigned xdataMemorySize;
  unsigned iromMemorySize;
  unsigned xromMemorySize;

  /*
   * Last input and output.
   */
  int OUTPUT;
  int INPUT;

  /*
   * Error ID. 0 means no error.
   */
  int errid;

  /*
   * Lowest and highest used adresses.
   */
  int maxValueOfStackPointer;
  int minIntRamAddress;
  int maxIntRamAddress;
  int minExtRamAddress;
  int maxExtRamAddress;
  int maxIntRomAddress;
  int maxExtRomAddress;

  /*
   * pointers to registers in sfr
   * do not change this pointers!!!
   */

  BYTE* SP;
  BYTE* P0;
  WORD* DPTR;
  BYTE* DPL;
  BYTE* DPH;
  BYTE* DP0L;   // 89S5x
  BYTE* DP0H;   // 89S5x
  BYTE* DP1L;   // 89S5x
  BYTE* DP1H;   // 89S5x
  BYTE* PCON;
  BYTE* TCON;
  BYTE* TMOD;
  BYTE* TL0;
  BYTE* TL1;
  BYTE* TH0;
  BYTE* TH1;
  BYTE* AUXR;   // 89S5x
  BYTE* WDTRST; // 89S5x
  BYTE* P1;
  BYTE* SCON;
  BYTE* SBUF;
  BYTE* AUXR1;  // 89S5x
  BYTE* P2;
  BYTE* IE;
  BYTE* P3;
  BYTE* IP;
  BYTE* T2CON;  // 8052
  BYTE* RCAP2L; // 8052
  BYTE* RCAP2H; // 8052
  BYTE* TL2;    // 8052
  BYTE* TH2;    // 8052
  BYTE* PSW;
  BYTE* ACC;
  BYTE* B;
  BYTE* R;

  /*
   * Auto I/O.
   */
  bool autoRead;
  bool autoWrite;

  /*
   * Breakpoints and conditional pauses.
   */
  WORD* PCBreakpoints;
  WORD* accessIntRAMPauses;
  WORD* accessExtRAMPauses;
  WORD* accessIntROMPauses;
  WORD* accessExtROMPauses;
  WORD* accessSFRPauses;
  MCUConditionBreakpoint* intRAMPauses;
  MCUConditionBreakpoint* extRAMPauses;
  MCUConditionBreakpoint* SFRPauses;

  unsigned numOfPCBreakpoints;
  unsigned numOfaccessIntRAMPauses;
  unsigned numOfAccessExtRAMPauses;
  unsigned numOfAccessIntROMPauses;
  unsigned numOfAccessExtROMPauses;
  unsigned numOfAccessSFRPauses;
  unsigned numOfIntRAMPauses;
  unsigned numOfExtRAMPauses;
  unsigned numOfSFRPauses;

  /*
   * Pointers to last changed memory.
   */
  int accessedSFR;
  int accessedIntRAM;
  int accessedExtRAM;
  int accessedIntROM;
  int accessedExtROM;

  /*
   * Pointers to last changed memory but only when changed by instruction.
   */
  BYTE changedSFR;
  BYTE changedIntRAM;
  WORD changedExtRAM;

  /*
   * Before access (helps to finds changes).
   */
  BYTE _beforeAccessedSFR;
  BYTE _beforeAccessedIntRAM;
  BYTE _beforeAccessedExtRAM;

  /*
   * Watchdog timer. (only for 89S5x)
   */
  bool WDTEnable;
  WORD WDTTimerValue;

  /*
   * Instruction tables.
   */
  int cycleCount[256];
  int byteCount[256];
  char* mnemonicTable[256];
  char* mnemonicParams[256];
  char* SFRNames[256];
  char* SFRBits[256];

  /*
   * Pointers to microcontroller specific functions.
   */
  void (*_instructions[256])(struct _mcu* mcu);
  void (*_timers)(struct _mcu* mcu);
  void (*_interrupts)(struct _mcu* mcu);
  void (*_additionalCode)(struct _mcu* mcu);
  //BYTE* (*_intRam)(struct _mcu* mcu, BYTE address, bool direct, bool info);
  //BYTE* (*_setIntRam)(struct _mcu* mcu, BYTE address, bool direct, BYTE value, bool info);
  //BYTE* (*_extRam)(struct _mcu* mcu, WORD address, bool info);
  //BYTE* (*_rom)(struct _mcu* mcu, WORD address, bool info);

} MCU;


void init8051MCU(MCU* mcu);
void init8031MCU(MCU* mcu);
void init8052MCU(MCU* mcu);
void init8032MCU(MCU* mcu);
void init89S51MCU(MCU* mcu);
void init89S52MCU(MCU* mcu);

void resetMCU(MCU* mcu);
void removeMCU(MCU* mcu);

char* getError(MCU* mcu);

BYTE* intRAM(MCU* mcu, BYTE address, bool direct);
BYTE* extRAM(MCU* mcu, WORD address);
BYTE* ROM(MCU* mcu, WORD address);

bool checkRegister(MCU* mcu, WORD flag);
void setRegister(MCU* mcu, WORD flag, bool state);

bool isBreakpointOrPause(MCU* mcu);
void setPCBreakpoint(MCU* mcu, WORD address);
void setAccessPause(MCU* mcu, MemoryType memoryType, WORD address);
void setCondPause(MCU* mcu, MemoryType memoryType,
                  WORD address, BreakpointType breakType, BYTE value);

void clearPCBreakpoint(MCU* mcu, WORD address);
void clearAccessPause(MCU* mcu, MemoryType memoryType, WORD address);
void clearCondPause(MCU* mcu, MemoryType memoryType, WORD address);
void clearAllBreakpointsAndPauses(MCU* mcu);

double getMCUTime(MCU* mcu);

/*
 * The heart of microcontroller simulator. Proccess only one instruction.
 * This instruction also update timers and following members of MCU structure:
 * accessedSFR, accessedIntRAM, accessedExtRAM, changedSFR, changedIntRAM,
 * changedExtRAM
 * TODO: T2 Timer.
 */
void processMCU(MCU* mcu);

/*
 * Extended processMCU function. Return false if one of breakpoints
 * or conditional pauses become be satisfied. Parametrs out, in shuld be use
 * for write and read data from uart, useOut and needIn determines whether
 * uart was used.
 */
bool processMCUEx(MCU* mcu, BYTE* out, BYTE* in, bool* useOut, bool* needIn);

char* disassembler(MCU* mcu, WORD ip, char* format, WORD* next);

#endif /* _8051_H_ */

/*
vi:ts=4:et:nowrap
*/
