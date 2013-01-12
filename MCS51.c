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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MCS51.h"
#include "Utils.h"

/*
 * Default memory wrappers.
 */
inline BYTE* _mcuIntRAM(MCU* mcu, BYTE address, bool direct, bool info)
{
  if (address >= mcu->idataMemorySize && !direct) {
    address &= mcu->idataMemorySize - 1;
    mcu->errid = E_IRAMOUTSIDE;
  }

  if (address < 0x80) {
    if (info) {
      mcu->_beforeAccessedIntRAM = mcu->idata[address];
      mcu->accessedIntRAM = address;
    }

    return &mcu->idata[address];
  } else {
    if (direct) {
      if (info) {
        mcu->_beforeAccessedSFR = mcu->sfr[address - 0x80];
        mcu->accessedSFR = address;
      }

      return &mcu->sfr[address - 0x80];
    } else { // indirect
      if (info) {
        mcu->_beforeAccessedIntRAM = mcu->idata[address];
        mcu->accessedIntRAM = address;
      }

      return &mcu->idata[address];
    }
  }
}

inline void _mcuSetIntRAM(MCU* mcu, BYTE address, bool direct, BYTE value, bool info)
{
  if (direct && address == 0x99)
    mcu->OUTPUT = value;

  *_mcuIntRAM(mcu, address, direct, info) = value;
}

inline BYTE* _mcuExtRAM(MCU* mcu, WORD address, bool info)
{
  if (address >= mcu->xdataMemorySize) {
    address %= mcu->xdataMemorySize == 0 ? 1 : mcu->xdataMemorySize;
    mcu->errid = E_XRAMOUTSIDE;
  }

  if (info) {
    mcu->_beforeAccessedExtRAM = mcu->xdata[address];
    mcu->accessedExtROM = address;
  }

  return &mcu->xdata[address];
}

inline BYTE* _mcuROM(MCU* mcu, WORD address, bool info)
{
  if (mcu->xromMemorySize != 0 && (address >= mcu->iromMemorySize || !mcu->EA)) {
    if (address >= mcu->xromMemorySize) {
      address %= mcu->xdataMemorySize == 0 ? 1 : mcu->xdataMemorySize;
    }

    if (info)
      mcu->accessedExtROM = address;

    return &mcu->xrom[address];

  }

  if (address >= mcu->iromMemorySize) {
    address %= mcu->idataMemorySize == 0 ? 1 : mcu->idataMemorySize;
  }

  if (info)
    mcu->accessedIntROM = address;

  return &mcu->irom[address];
}

inline BYTE* mcuIntRAM(MCU* mcu, BYTE address, bool direct)
{
  return  _mcuIntRAM(mcu, address, direct, true);
}

inline void mcuSetIntRAM(MCU* mcu, BYTE address, bool direct, BYTE value)
{
  _mcuSetIntRAM(mcu, address, direct, value, true);
}

inline BYTE* mcuExtRAM(MCU* mcu, WORD address)
{
  return _mcuExtRAM(mcu, address, true);
}

inline BYTE* mcuROM(MCU* mcu, WORD address)
{
  return _mcuROM(mcu, address, true);
}

inline bool mcuCheckBit(MCU* mcu, BYTE bit)
{
  BYTE address;
  BYTE bitToCheck;
  address = bit & 0xF8;
  bitToCheck = bit & 0x07;
  if (bit >= 0x80) { // sfr
    if (*mcuIntRAM(mcu, address, true) & 1 << bitToCheck) {
      return true;
    } else {
      return false;
    }
  } else { // int ram
    address = (address >> 3) + 32;
    if (*mcuIntRAM(mcu, address, true) & 1 << bitToCheck) {
      return true;
    } else {
      return false;
    }
  }
}

inline void mcuSetBit(MCU* mcu, BYTE bit, bool state)
{
  BYTE address;
  BYTE bitToCheck;

  address = bit & 0xF8;
  bitToCheck = bit & 0x07;

  if (bit >= 0x80) { // sfr
    if (state)
      *mcuIntRAM(mcu, address, true) |= 1 << bitToCheck;
    else
      *mcuIntRAM(mcu, address, true) &= ~(1 << bitToCheck);
  } else { // int ram
    address = (address >> 3) + 32;
    if (state)
      *mcuIntRAM(mcu, address, true) |= 1 << bitToCheck;
    else
      *mcuIntRAM(mcu, address, true) &= ~(1 << bitToCheck);
  }
}

inline void mcuAdd(MCU* mcu, BYTE* a, BYTE* b)
{
  int temp1, temp2;
  if (((0x0F & *a) + (0x0F & *b)) > 0x0F)
    setRegister(mcu, PSW_AC, true);
  else
    setRegister(mcu, PSW_AC, false);

  temp1 = ((0x7F & *a) + (0x7F & *b)) > 0x7F;
  temp2 = *a + *b;

  if (temp2 > 0xFF) {
    setRegister(mcu, PSW_CY, true);

    if (temp1)
      setRegister(mcu, PSW_OV, false);
    else
      setRegister(mcu, PSW_OV, true);
  } else {
    setRegister(mcu, PSW_CY, false);

    if (temp1)
      setRegister(mcu, PSW_OV, true);
    else
      setRegister(mcu, PSW_OV, false);
  }
  *a = (BYTE) temp2 & 0xFF;
}

inline void mcuAddc(MCU* mcu, BYTE* a, BYTE* b)
{
  int temp1, temp2, c = checkRegister(mcu, PSW_CY);
  if (((0x0F & *a) + (0x0F & (*b + c))) > 0x0F)
    setRegister(mcu, PSW_AC, true);
  else
    setRegister(mcu, PSW_AC, false);

  temp1 = ((0x7F & *a) + (0x7F & *b)) + c > 0x7F;
  temp2 = *a + *b + c;

  if (temp2 > 0xFF) {
    setRegister(mcu, PSW_CY, true);

    if (temp1)
      setRegister(mcu, PSW_OV, false);
    else
      setRegister(mcu, PSW_OV, true);
  } else {
    setRegister(mcu, PSW_CY, false);

    if (temp1)
      setRegister(mcu, PSW_OV, true);
    else
      setRegister(mcu, PSW_OV, false);
  }
  *a = (BYTE) temp2 & 0xFF;
}

extern inline void mcuSub(MCU* mcu, BYTE* a, BYTE* b)
{
  int temp1, temp2, c = checkRegister(mcu, PSW_CY);
  if (((char) (0x0F & *a) - (char) (0xF & *b) - c))
    setRegister(mcu, PSW_AC, true);
  else
    setRegister(mcu, PSW_AC, false);

  temp1 = ((char) (0x7F & *a) - (char) (0x7F & *b) - c) < 0;
  temp2 = *a - *b - c;

  if (temp2 < 0) {
    setRegister(mcu, PSW_CY, true);

    if (temp1)
      setRegister(mcu, PSW_OV, false);
    else
      setRegister(mcu, PSW_OV, true);
  } else {
    setRegister(mcu, PSW_CY, false);

    if (temp1)
      setRegister(mcu, PSW_OV, true);
    else
      setRegister(mcu, PSW_OV, false);
  }
  *a = (BYTE) temp2 & 0xFF;
}

// nop
void nop_00(MCU* mcu)
{
  mcu->PC += 1;
}

// mov a,#(byte)
void mov_74(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// mov (adress),#(byte)
void mov_75(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, *mcuROM(mcu, mcu->PC + 1));
  mcu->PC += 2;
}

// ljmp 16bit_adres
void ljmp_02(MCU* mcu)
{
  mcu->PC += 1;
  mcu->PC = (WORD) *mcuROM(mcu, mcu->PC) << 8 | *mcuROM(mcu, mcu->PC + 1);
}

// sjmp offset
void sjmp_80(MCU* mcu)
{
  mcu->PC += 1;
  mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
}

// mov (adress1),(ardess2)
void mov_85(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC + 1), true, *mcuIntRAM(mcu,
               *mcuROM(mcu, mcu->PC), true));
  mcu->PC += 2;
}

// mov A,(adress)
void mov_E5(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// mov (bit),C
void mov_92(MCU* mcu)
{
  mcu->PC += 1;
  if (checkRegister(mcu, PSW_CY))
    mcuSetBit(mcu, *mcuROM(mcu, mcu->PC), true);
  else
    mcuSetBit(mcu, *mcuROM(mcu, mcu->PC), false);
  mcu->PC += 1;
}

// inc a
void inc_04(MCU* mcu)
{
  (*mcu->ACC)++;
  mcu->PC += 1;
}

// inc (adress)
void inc_05(MCU* mcu)
{
  mcu->PC += 1;
  (*mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true))++;
  mcu->PC += 1;
}

// inc DPTR
void inc_A3(MCU* mcu)
{
  *mcu->DPTR = *mcu->DPTR + 1;
  mcu->PC += 1;
}

// inc @R0
void inc_06(MCU* mcu)
{
  (*mcuIntRAM(mcu, mcu->R[0], false))++;
  mcu->PC += 1;
}

// inc @R1
void inc_07(MCU* mcu)
{
  (*mcuIntRAM(mcu, mcu->R[1], false))++;
  mcu->PC += 1;
}

// inc R0
void inc_08(MCU* mcu)
{
  mcu->R[0]++;
  mcu->PC += 1;
}

// inc R1
void inc_09(MCU* mcu)
{
  mcu->R[1]++;
  mcu->PC += 1;
}

// inc R2
void inc_0A(MCU* mcu)
{
  mcu->R[2]++;
  mcu->PC += 1;
}

// inc R3
void inc_0B(MCU* mcu)
{
  mcu->R[3]++;
  mcu->PC += 1;
}

// inc R4
void inc_0C(MCU* mcu)
{
  mcu->R[4]++;
  mcu->PC += 1;
}

// inc R5
void inc_0D(MCU* mcu)
{
  mcu->R[5]++;
  mcu->PC += 1;
}

// inc R6
void inc_0E(MCU* mcu)
{
  mcu->R[6]++;
  mcu->PC += 1;
}

// inc R7
void inc_0F(MCU* mcu)
{
  mcu->R[7]++;
  mcu->PC += 1;
}

// dec A
void dec_14(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC - 1;
  mcu->PC += 1;
}

// dec (adress)
void dec_15(MCU* mcu)
{
  mcu->PC += 1;
  (*mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true))--;
  mcu->PC += 1;
}

// dec @R0
void dec_16(MCU* mcu)
{
  (*mcuIntRAM(mcu, mcu->R[0], false))--;
  mcu->PC += 1;
}

// dec @R1
void dec_17(MCU* mcu)
{
  (*mcuIntRAM(mcu, mcu->R[1], false))--;
  mcu->PC += 1;
}

// dec R0
void dec_18(MCU* mcu)
{
  mcu->R[0]--;
  mcu->PC += 1;
}

// dec R1
void dec_19(MCU* mcu)
{
  mcu->R[1]--;
  mcu->PC += 1;
}

// dec R2
void dec_1A(MCU* mcu)
{
  mcu->R[2]--;
  mcu->PC += 1;
}

// dec R3
void dec_1B(MCU* mcu)
{
  mcu->R[3]--;
  mcu->PC += 1;
}

// dec R4
void dec_1C(MCU* mcu)
{
  mcu->R[4]--;
  mcu->PC += 1;
}

// dec R5
void dec_1D(MCU* mcu)
{
  mcu->R[5]--;
  mcu->PC += 1;
}

// dec R6
void dec_1E(MCU* mcu)
{
  mcu->R[6]--;
  mcu->PC += 1;
}

// dec R7
void dec_1F(MCU* mcu)
{
  mcu->R[7]--;
  mcu->PC += 1;
}

// div AB
void div_84(MCU* mcu)
{
  BYTE tempB;

  setRegister(mcu, PSW_CY, false);
  if (*mcu->B != 0) {
    tempB = *mcu->ACC;
    *mcu->ACC = tempB / *mcu->B;
    *mcu->B = tempB % *mcu->B;
  } else
    setRegister(mcu, PSW_OV, true);
  mcu->PC += 1;
}

// mul AB
void mul_A4(MCU* mcu)
{
  WORD tempW;

  tempW = (WORD) *mcu->ACC * (WORD) *mcu->B;
  if (tempW > 255)
    setRegister(mcu, PSW_OV, true);
  else
    setRegister(mcu, PSW_OV, false);
  *mcu->B = (BYTE) tempW >> 8;
  *mcu->ACC = (BYTE) (tempW & 0xFF);
  mcu->PC += 1;
}

// rl A
void rl_23(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = tempB << 1 | tempB >> 7;
  mcu->PC += 1;
}

// rr A
void rr_03(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = tempB >> 1 | tempB << 7;
  mcu->PC += 1;
}

// rlc A
void rlc_33(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = tempB << 1 | checkRegister(mcu, PSW_CY);
  if (tempB & 0x80)
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  mcu->PC += 1;
}

// rrc A
void rrc_13(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = tempB >> 1 | checkRegister(mcu, PSW_CY) << 7;
  if (tempB & 0x01)
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  mcu->PC += 1;
}

// mov DPTR,#(16-bit data)
void mov_90(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->DPH = *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
  *mcu->DPL = *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// mov (adress),A
void mov_F5(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, *mcu->ACC);
  mcu->PC += 1;
}

// add A,#(data)
void add_24(MCU* mcu)
{
  mcu->PC += 1;
  mcuAdd(mcu, mcu->ACC, &*mcuROM(mcu, mcu->PC));
  mcu->PC += 1;
}

// add A,(adres)
void add_25(MCU* mcu)
{
  mcu->PC += 1;
  mcuAdd(mcu, mcu->ACC, mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true));
  mcu->PC += 1;
}

// add A,@R0
void add_26(MCU* mcu)
{
  mcuAdd(mcu, mcu->ACC, mcuIntRAM(mcu, mcu->R[0], false));
  mcu->PC += 1;
}

// add A,@R1
void add_27(MCU* mcu)
{
  mcuAdd(mcu, mcu->ACC, mcuIntRAM(mcu, mcu->R[1], false));
  mcu->PC += 1;
}

// add A,R0
void add_28(MCU* mcu)
{
  mcuAdd(mcu, mcu->ACC, &mcu->R[0]);
  mcu->PC += 1;
}

// add A,R1
void add_29(MCU* mcu)
{
  mcuAdd(mcu, mcu->ACC, &mcu->R[1]);
  mcu->PC += 1;
}

// add A,R2
void add_2A(MCU* mcu)
{
  mcuAdd(mcu, mcu->ACC, &mcu->R[2]);
  mcu->PC += 1;
}

// add A,R3
void add_2B(MCU* mcu)
{
  mcuAdd(mcu, mcu->ACC, &mcu->R[3]);
  mcu->PC += 1;
}

// add A,R4
void add_2C(MCU* mcu)
{
  mcuAdd(mcu, mcu->ACC, &mcu->R[4]);
  mcu->PC += 1;
}

// add A,R5
void add_2D(MCU* mcu)
{
  mcuAdd(mcu, mcu->ACC, &mcu->R[5]);
  mcu->PC += 1;
}

// add A,R6
void add_2E(MCU* mcu)
{
  mcuAdd(mcu, mcu->ACC, &mcu->R[6]);
  mcu->PC += 1;
}

// add A,R7
void add_2F(MCU* mcu)
{
  mcuAdd(mcu, mcu->ACC, &mcu->R[7]);
  mcu->PC += 1;
}

// ADDC
// addc A,#(data)
void addc_34(MCU* mcu)
{
  mcu->PC += 1;
  mcuAddc(mcu, mcu->ACC, &*mcuROM(mcu, mcu->PC));
  mcu->PC += 1;
}

// addc A,(adres)
void addc_35(MCU* mcu)
{
  mcu->PC += 1;
  mcuAddc(mcu, mcu->ACC, mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true));
  mcu->PC += 1;
}

// addc A,@R0
void addc_36(MCU* mcu)
{
  mcuAddc(mcu, mcu->ACC, mcuIntRAM(mcu, mcu->R[0], false));
  mcu->PC += 1;
}

// addc A,@R1
void addc_37(MCU* mcu)
{
  mcuAddc(mcu, mcu->ACC, mcuIntRAM(mcu, mcu->R[1], false));
  mcu->PC += 1;
}

// addc A,R0
void addc_38(MCU* mcu)
{
  mcuAddc(mcu, mcu->ACC, &mcu->R[0]);
  mcu->PC += 1;
}

// addc A,R1
void addc_39(MCU* mcu)
{
  mcuAddc(mcu, mcu->ACC, &mcu->R[1]);
  mcu->PC += 1;
}

// addc A,R2
void addc_3A(MCU* mcu)
{
  mcuAddc(mcu, mcu->ACC, &mcu->R[2]);
  mcu->PC += 1;
}

// addc A,R3
void addc_3B(MCU* mcu)
{
  mcuAddc(mcu, mcu->ACC, &mcu->R[3]);
  mcu->PC += 1;
}

// addc A,R4
void addc_3C(MCU* mcu)
{
  mcuAddc(mcu, mcu->ACC, &mcu->R[4]);
  mcu->PC += 1;
}

// addc A,R5
void addc_3D(MCU* mcu)
{
  mcuAddc(mcu, mcu->ACC, &mcu->R[5]);
  mcu->PC += 1;
}

// addc A,R6
void addc_3E(MCU* mcu)
{
  mcuAddc(mcu, mcu->ACC, &mcu->R[6]);
  mcu->PC += 1;
}

// addc A,R7
void addc_3F(MCU* mcu)
{
  mcuAddc(mcu, mcu->ACC, &mcu->R[7]);
  mcu->PC += 1;
}

// movx A,@DPTR
void movx_E0(MCU* mcu)
{
  *mcu->ACC = *mcuExtRAM(mcu, *mcu->DPTR);
  mcu->PC += 1;
}

// movx @DPTR,A
void movx_F0(MCU* mcu)
{
  *mcuExtRAM(mcu, *mcu->DPTR) = *mcu->ACC;
  mcu->PC += 1;
}

// push (adress)
void push_C0(MCU* mcu)
{
  *mcu->SP += 1;
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true));
  mcu->PC += 1;
}

// pop (adress)
void pop_D0(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, *mcuIntRAM(mcu, *mcu->SP, false));
  *mcu->SP = *mcu->SP - 1;
  mcu->PC += 1;
}

// clr C
void clr_C3(MCU* mcu)
{
  setRegister(mcu, PSW_CY, false);
  mcu->PC += 1;
}

// setb C
void setb_D3(MCU* mcu)
{
  setRegister(mcu, PSW_CY, true);
  mcu->PC += 1;
}

// setb (bit)
void setb_D2(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetBit(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// clr (bit)
void clr_C2(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetBit(mcu, *mcuROM(mcu, mcu->PC), false);
  mcu->PC += 1;
}

// xrl (adress),A
void xrl_62(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, *mcuIntRAM(mcu,
               *mcuROM(mcu, mcu->PC), true) ^ (*mcu->ACC));
  mcu->PC += 1;
}

// xrl (adress),#(data)
void xrl_63(MCU* mcu)
{
  mcu->PC += 1;
  *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true) ^= *mcuROM(mcu, mcu->PC + 1);
  mcu->PC += 2;
}

// xrl A,#(data)
void xrl_64(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = *mcu->ACC ^ *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// xrl A,(adress)
void xrl_65(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = *mcu->ACC ^ *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// xrl A,@R0
void xrl_66(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC ^ *mcuIntRAM(mcu, mcu->R[0], false);
  mcu->PC += 1;
}

// xrl A,@R1
void xrl_67(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC ^ *mcuIntRAM(mcu, mcu->R[1], false);
  mcu->PC += 1;
}

// xrl A,R0
void xrl_68(MCU* mcu)
{
  *mcu->ACC = (*mcu->ACC) ^ mcu->R[0];
  mcu->PC += 1;
}

// xrl A,R1
void xrl_69(MCU* mcu)
{
  *mcu->ACC = (*mcu->ACC) ^ mcu->R[1];
  mcu->PC += 1;
}

// xrl A,R2
void xrl_6A(MCU* mcu)
{
  *mcu->ACC = (*mcu->ACC) ^ mcu->R[2];
  mcu->PC += 1;
}

// xrl A,R3
void xrl_6B(MCU* mcu)
{
  *mcu->ACC = (*mcu->ACC) ^ mcu->R[3];
  mcu->PC += 1;
}

// xrl A,R4
void xrl_6C(MCU* mcu)
{
  *mcu->ACC = (*mcu->ACC) ^ mcu->R[4];
  mcu->PC += 1;
}

// xrl A,R5
void xrl_6D(MCU* mcu)
{
  *mcu->ACC = (*mcu->ACC) ^ mcu->R[5];
  mcu->PC += 1;
}

// xrl A,R6
void xrl_6E(MCU* mcu)
{
  *mcu->ACC = (*mcu->ACC) ^ mcu->R[6];
  mcu->PC += 1;
}

// xrl A,R7
void xrl_6F(MCU* mcu)
{
  *mcu->ACC = (*mcu->ACC) ^ mcu->R[7];
  mcu->PC += 1;
}

// xch A,(adress)
void xch_C5(MCU* mcu)
{
  BYTE tempB;

  mcu->PC += 1;
  tempB = *mcu->ACC;
  *mcu->ACC = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true);
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, tempB);
  mcu->PC += 1;
}

// xch A,@R0
void xch_C6(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = *mcuIntRAM(mcu, mcu->R[0], false);
  mcuSetIntRAM(mcu, mcu->R[0], false, tempB);
  mcu->PC += 1;
}

// xch A,@R1
void xch_C7(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = *mcuIntRAM(mcu, mcu->R[1], false);
  mcuSetIntRAM(mcu, mcu->R[1], false, tempB);
  mcu->PC += 1;
}

// xch A,R0
void xch_C8(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = mcu->R[0];
  mcu->R[0] = tempB;
  mcu->PC += 1;
}

// xch A,R1
void xch_C9(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = mcu->R[1];
  mcu->R[1] = tempB;
  mcu->PC += 1;
}

// xch A,R2
void xch_CA(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = mcu->R[2];
  mcu->R[2] = tempB;
  mcu->PC += 1;
}

// xch A,R3
void xch_CB(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = mcu->R[3];
  mcu->R[3] = tempB;
  mcu->PC += 1;
}

// xch A,R4
void xch_CC(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = mcu->R[4];
  mcu->R[4] = tempB;
  mcu->PC += 1;
}

// xch A,R5
void xch_CD(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = mcu->R[5];
  mcu->R[5] = tempB;
  mcu->PC += 1;
}

// xch A,R6
void xch_CE(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = mcu->R[6];
  mcu->R[6] = tempB;
  mcu->PC += 1;
}

// xch A,R7
void xch_CF(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  *mcu->ACC = mcu->R[7];
  mcu->R[7] = tempB;
  mcu->PC += 1;
}

// lcall 16bit_adres
void lcall_12(MCU* mcu)
{
  WORD tempW;

  tempW = mcu->PC + 3;
  mcu->PC += 1;
  mcu->PC = (WORD) *mcuROM(mcu, mcu->PC) << 8 | *mcuROM(mcu, mcu->PC + 1);
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW & 0x00FF));
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW >> 8));
}

// ret
void ret_22(MCU* mcu)
{
  mcu->PC = (WORD) *mcuIntRAM(mcu, *mcu->SP, false) << 8 | (WORD) *mcuIntRAM(mcu,
            *mcu->SP - 1, false);
  *mcu->SP = *mcu->SP - 2;
}

// jnz (offset)
void jnz_70(MCU* mcu)
{
  mcu->PC += 1;
  if (*mcu->ACC != 0)
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  else
    mcu->PC += 1;
}

// jz (offset)
void jz_60(MCU* mcu)
{
  mcu->PC += 1;
  if (*mcu->ACC == 0)
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  else
    mcu->PC += 1;
}

// jc (offset)
void jc_40(MCU* mcu)
{
  mcu->PC += 1;
  if (checkRegister(mcu, PSW_CY))
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  else
    mcu->PC += 1;
}

// jnc (offset)
void jnc_50(MCU* mcu)
{
  mcu->PC += 1;
  if (!checkRegister(mcu, PSW_CY))
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  else
    mcu->PC += 1;
}

// swap A
void swap_C4(MCU* mcu)
{
  BYTE tempB;

  tempB = *mcu->ACC;
  tempB = (tempB << 4) & 0xF0;
  *mcu->ACC = (*mcu->ACC >> 4) & 0x0F;
  *mcu->ACC = *mcu->ACC | tempB;
  mcu->PC += 1;
}

// movc A,@DPTR+A
void movc_93(MCU* mcu)
{
  *mcu->ACC = *mcuROM(mcu, *mcu->ACC + *mcu->DPTR);
  mcu->PC += 1;
}

// movc A,@mcu->PC+A
void movc_83(MCU* mcu)
{
  *mcu->ACC = *mcuROM(mcu, *mcu->ACC + mcu->PC);
  mcu->PC += 1;
}

// mov A,@R0
void mov_E6(MCU* mcu)
{
  *mcu->ACC = *mcuIntRAM(mcu, mcu->R[0], false);
  mcu->PC += 1;
}

// mov A,@R1
void mov_E7(MCU* mcu)
{
  *mcu->ACC = *mcuIntRAM(mcu, mcu->R[1], false);
  mcu->PC += 1;
}

// mov @R0,A
void mov_F6(MCU* mcu)
{
  mcuSetIntRAM(mcu, mcu->R[0], false, *mcu->ACC);
  mcu->PC += 1;
}

// mov @R1,a
void mov_F7(MCU* mcu)
{
  mcuSetIntRAM(mcu, mcu->R[1], false, *mcu->ACC);
  mcu->PC += 1;
}

// mov R0,#(data)
void mov_78(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[0] = *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// mov R1,#(data)
void mov_79(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[1] = *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// mov R2,#(data)
void mov_7A(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[2] = *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// mov R3,#(data)
void mov_7B(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[3] = *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// mov R4,#(data)
void mov_7C(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[4] = *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// mov R5,#(data)
void mov_7D(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[5] = *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// mov R6,#(data)
void mov_7E(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[6] = *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// mov R7,#(data)
void mov_7F(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[7] = *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// mov R0,(adress)
void mov_A8(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[0] = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// mov R1,(adress)
void mov_A9(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[1] = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// mov R2,(adress)
void mov_AA(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[2] = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// mov R3,(adress)
void mov_AB(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[3] = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// mov R4,(adress)
void mov_AC(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[4] = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// mov R5,(adress)
void mov_AD(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[5] = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// mov R6,(adress)
void mov_AE(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[6] = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// mov R7,(adress)
void mov_AF(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[7] = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// mov (adress),R0
void mov_88(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, mcu->R[0]);
  mcu->PC += 1;
}

// mov (adress),R1
void mov_89(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, mcu->R[1]);
  mcu->PC += 1;
}

// mov (adress),R2
void mov_8A(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, mcu->R[2]);
  mcu->PC += 1;
}

// mov (adress),R3
void mov_8B(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, mcu->R[3]);
  mcu->PC += 1;
}

// mov (adress),R4
void mov_8C(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, mcu->R[4]);
  mcu->PC += 1;
}

// mov (adress),R5
void mov_8D(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, mcu->R[5]);
  mcu->PC += 1;
}

// mov (adress),R6
void mov_8E(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, mcu->R[6]);
  mcu->PC += 1;
}

// mov (adress),R7
void mov_8F(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, mcu->R[7]);
  mcu->PC += 1;
}

// mov R0,A
void mov_F8(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[0] = *mcu->ACC;
}

// mov R1,A
void mov_F9(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[1] = *mcu->ACC;
}

// mov R2,A
void mov_FA(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[2] = *mcu->ACC;
}

// mov R3,A
void mov_FB(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[3] = *mcu->ACC;
}

// mov R4,A
void mov_FC(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[4] = *mcu->ACC;
}

// mov R5,A
void mov_FD(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[5] = *mcu->ACC;
}

// mov R6,A
void mov_FE(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[6] = *mcu->ACC;
}

// mov R7,A
void mov_FF(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[7] = *mcu->ACC;
}

// mov A,R0
void mov_E8(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = mcu->R[0];
}

// mov A,R1
void mov_E9(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = mcu->R[1];
}

// mov A,R2
void mov_EA(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = mcu->R[2];
}

// mov A,R3
void mov_EB(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = mcu->R[3];
}

// mov A,R4
void mov_EC(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = mcu->R[4];
}

// mov A,R5
void mov_ED(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = mcu->R[5];
}

// mov A,R6
void mov_EE(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = mcu->R[6];
}

// mov A,R7
void mov_EF(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = mcu->R[7];
}

// clr A
void clr_E4(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = 0;
}

// ajmp 00xx
void ajmp_01(MCU* mcu)
{
  mcu->PC += 1;
  mcu->PC = (WORD) *mcuROM(mcu, mcu->PC);
}

// ajmp 01xx
void ajmp_21(MCU* mcu)
{
  mcu->PC += 1;
  mcu->PC = 0x0100 | (WORD) *mcuROM(mcu, mcu->PC);
}

// ajmp 02xx
void ajmp_41(MCU* mcu)
{
  mcu->PC += 1;
  mcu->PC = 0x0200 | (WORD) *mcuROM(mcu, mcu->PC);
}

// ajmp 03xx
void ajmp_61(MCU* mcu)
{
  mcu->PC += 1;
  mcu->PC = 0x0300 | (WORD) *mcuROM(mcu, mcu->PC);
}

// ajmp 04xx
void ajmp_81(MCU* mcu)
{
  mcu->PC += 1;
  mcu->PC = 0x0400 | (WORD) *mcuROM(mcu, mcu->PC);
}

// ajmp 05xx
void ajmp_A1(MCU* mcu)
{
  mcu->PC += 1;
  mcu->PC = 0x0500 | (WORD) *mcuROM(mcu, mcu->PC);
}

// ajmp 06xx
void ajmp_C1(MCU* mcu)
{
  mcu->PC += 1;
  mcu->PC = 0x0600 | (WORD) *mcuROM(mcu, mcu->PC);
}

// ajmp 07xx
void ajmp_E1(MCU* mcu)
{
  mcu->PC += 1;
  mcu->PC = 0x0700 | (WORD) *mcuROM(mcu, mcu->PC);
}

// acall 00xx
void acall_11(MCU* mcu)
{
  WORD tempW;

  tempW = mcu->PC + 2;
  mcu->PC += 1;
  mcu->PC = (WORD) *mcuROM(mcu, mcu->PC);
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW & 0x00FF));
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW >> 8));
}

// acall 01xx
void acall_31(MCU* mcu)
{
  WORD tempW;

  tempW = mcu->PC + 2;
  mcu->PC += 1;
  mcu->PC = 0x0100 | *mcuROM(mcu, mcu->PC);
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW & 0x00FF));
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW >> 8));
}

// acall 02xx
void acall_51(MCU* mcu)
{
  WORD tempW;

  tempW = mcu->PC + 2;
  mcu->PC += 1;
  mcu->PC = 0x0200 | *mcuROM(mcu, mcu->PC);
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW & 0x00FF));
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW >> 8));
}

// acall 03xx
void acall_71(MCU* mcu)
{
  WORD tempW;

  tempW = mcu->PC + 2;
  mcu->PC += 1;
  mcu->PC = 0x0300 | *mcuROM(mcu, mcu->PC);
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW & 0x00FF));
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW >> 8));
}

// acall 04xx
void acall_91(MCU* mcu)
{
  WORD tempW;

  tempW = mcu->PC + 2;
  mcu->PC += 1;
  mcu->PC = 0x0400 | *mcuROM(mcu, mcu->PC);
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW & 0x00FF));
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW >> 8));
}

// acall 05xx
void acall_B1(MCU* mcu)
{
  WORD tempW;

  tempW = mcu->PC + 2;
  mcu->PC += 1;
  mcu->PC = 0x0500 | *mcuROM(mcu, mcu->PC);
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW & 0x00FF));
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW >> 8));
}

// acall 06xx
void acall_D1(MCU* mcu)
{
  WORD tempW;

  tempW = mcu->PC + 2;
  mcu->PC += 1;
  mcu->PC = 0x0600 | *mcuROM(mcu, mcu->PC);
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW & 0x00FF));
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW >> 8));
}

// acall 07xx
void acall_F1(MCU* mcu)
{
  WORD tempW;

  tempW = mcu->PC + 2;
  mcu->PC += 1;
  mcu->PC = 0x0700 | *mcuROM(mcu, mcu->PC);
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW & 0x00FF));
  *mcu->SP += 1;
  mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (tempW >> 8));
}

// djnz R0,(offset)
void djnz_D8(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[0]--;
  if (mcu->R[0] != 0)
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// djnz R1,(offset)
void djnz_D9(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[1]--;
  if (mcu->R[1] != 0)
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// djnz R2,(offset)
void djnz_DA(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[2]--;
  if (mcu->R[2] != 0)
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// djnz R3,(offset)
void djnz_DB(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[3]--;
  if (mcu->R[3] != 0)
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// djnz R4,(offset)
void djnz_DC(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[4]--;
  if (mcu->R[4] != 0)
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// djnz R5,(offset)
void djnz_DD(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[5]--;
  if (mcu->R[5] != 0)
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// djnz R6,(offset)
void djnz_DE(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[6]--;
  if (mcu->R[6] != 0)
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// djnz R7,(offset)
void djnz_DF(MCU* mcu)
{
  mcu->PC += 1;
  mcu->R[7]--;
  if (mcu->R[7] != 0)
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// djnz (adress),(offset)
void djnz_D5(MCU* mcu)
{
  BYTE tempB;

  mcu->PC += 1;
  tempB = *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
  (*mcuIntRAM(mcu, tempB, true))--;
  if (*mcuIntRAM(mcu, tempB, true) != 0)
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// orl (adress),#(data)
void orl_43(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, *mcuIntRAM(mcu,
               *mcuROM(mcu, mcu->PC), true) | *mcuROM(mcu, mcu->PC + 1));
  mcu->PC += 2;
}

// orl A,#(data)
void orl_44(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = *mcuROM(mcu, mcu->PC) | *mcu->ACC;
  mcu->PC += 1;
}

// orl A,(adress)
void orl_45(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true) | *mcu->ACC;
  mcu->PC += 1;
}

// orl (adress),A
void orl_42(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, *mcuIntRAM(mcu,
               *mcuROM(mcu, mcu->PC), true) | *mcu->ACC);
  mcu->PC += 1;
}

// orl A,R0
void orl_48(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC | mcu->R[0];
  mcu->PC += 1;
}

// orl A,R1
void orl_49(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC | mcu->R[1];
  mcu->PC += 1;
}

// orl A,R2
void orl_4A(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC | mcu->R[2];
  mcu->PC += 1;
}

// orl A,R3
void orl_4B(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC | mcu->R[3];
  mcu->PC += 1;
}

// orl A,R4
void orl_4C(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC | mcu->R[4];
  mcu->PC += 1;
}

// orl A,R5
void orl_4D(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC | mcu->R[5];
  mcu->PC += 1;
}

// orl A,R6
void orl_4E(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC | mcu->R[6];
  mcu->PC += 1;
}

// orl A,R7
void orl_4F(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC | mcu->R[7];
  mcu->PC += 1;
}

// orl A,@R0
void orl_46(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC | *mcuIntRAM(mcu, mcu->R[0], false);
  mcu->PC += 1;
}

// orl A,@R1
void orl_47(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC | *mcuIntRAM(mcu, mcu->R[1], false);
  mcu->PC += 1;
}

// anl A,#(data)
void anl_54(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = *mcu->ACC & *mcuROM(mcu, mcu->PC);
  mcu->PC += 1;
}

// anl A,(adress)
void anl_55(MCU* mcu)
{
  mcu->PC += 1;
  *mcu->ACC = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true) & *mcu->ACC;
  mcu->PC += 1;
}

// anl (adress),A
void anl_52(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, *mcuIntRAM(mcu,
               *mcuROM(mcu, mcu->PC), true) & *mcu->ACC);
  mcu->PC += 1;
}

// anl (adress),#(data)
void anl_53(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, *mcuROM(mcu, mcu->PC), true, *mcuIntRAM(mcu,
               *mcuROM(mcu, mcu->PC), true) & *mcuROM(mcu, mcu->PC + 1));
  mcu->PC += 2;
}

// anl A,R0
void anl_58(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC & mcu->R[0];
  mcu->PC += 1;
}

// anl A,R1
void anl_59(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC & mcu->R[1];
  mcu->PC += 1;
}

// anl A,R2
void anl_5A(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC & mcu->R[2];
  mcu->PC += 1;
}

// anl A,R3
void anl_5B(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC & mcu->R[3];
  mcu->PC += 1;
}

// anl A,R4
void anl_5C(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC & mcu->R[4];
  mcu->PC += 1;
}

// anl A,R5
void anl_5D(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC & mcu->R[5];
  mcu->PC += 1;
}

// anl A,R6
void anl_5E(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC & mcu->R[6];
  mcu->PC += 1;
}

// anl A,R7
void anl_5F(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC & mcu->R[7];
  mcu->PC += 1;
}

// anl A,@R0
void anl_56(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC & *mcuIntRAM(mcu, mcu->R[0], false);
  mcu->PC += 1;
}

// anl A,@R1
void anl_57(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC & *mcuIntRAM(mcu, mcu->R[1], false);
  mcu->PC += 1;
}

// mov C,(bit)
void mov_A2(MCU* mcu)
{
  mcu->PC += 1;
  if (mcuCheckBit(mcu, *mcuROM(mcu, mcu->PC)))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  mcu->PC += 1;
}

// cpl (bit)
void cpl_B2(MCU* mcu)
{
  mcu->PC += 1;
  if (mcuCheckBit(mcu, *mcuROM(mcu, mcu->PC)))
    mcuSetBit(mcu, *mcuROM(mcu, mcu->PC), false);
  else
    mcuSetBit(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// cpl C
void cpl_B3(MCU* mcu)
{
  mcu->PC += 1;
  if (checkRegister(mcu, PSW_CY))
    setRegister(mcu, PSW_CY, false);
  else
    setRegister(mcu, PSW_CY, true);
}

// orl C,(bit)
void orl_72(MCU* mcu)
{
  mcu->PC += 1;
  if (checkRegister(mcu, PSW_CY) || mcuCheckBit(mcu, *mcuROM(mcu, mcu->PC)))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  mcu->PC += 1;
}

// orl C,/(bit)
void orl_A0(MCU* mcu)
{
  mcu->PC += 1;
  if (checkRegister(mcu, PSW_CY) || !mcuCheckBit(mcu, *mcuROM(mcu, mcu->PC)))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  mcu->PC += 1;
}

// anl C,(bit)
void anl_82(MCU* mcu)
{
  mcu->PC += 1;
  if (checkRegister(mcu, PSW_CY) && mcuCheckBit(mcu, *mcuROM(mcu, mcu->PC)))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  mcu->PC += 1;
}

// anl C,/(bit)
void anl_B0(MCU* mcu)
{
  mcu->PC += 1;
  if (checkRegister(mcu, PSW_CY) && !mcuCheckBit(mcu, *mcuROM(mcu, mcu->PC)))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  mcu->PC += 1;
}

// cpl A
void cpl_F4(MCU* mcu)
{
  *mcu->ACC = *mcu->ACC ^ 0xFF;
  mcu->PC += 1;
}

// mov @R0,(adress)
void mov_A6(MCU* mcu)
{
  mcu->PC += 1;
  *mcuIntRAM(mcu, mcu->R[0], false)
  = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// mov @R1,(adress)
void mov_A7(MCU* mcu)
{
  mcu->PC += 1;
  *mcuIntRAM(mcu, mcu->R[1], false)
  = *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true);
  mcu->PC += 1;
}

// mov @R0,#(data)
void mov_76(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, mcu->R[0], false, *mcuROM(mcu, mcu->PC));
  mcu->PC += 1;
}

// mov @R1,#(data)
void mov_77(MCU* mcu)
{
  mcu->PC += 1;
  mcuSetIntRAM(mcu, mcu->R[1], false, *mcuROM(mcu, mcu->PC));
  mcu->PC += 1;
}

// mov (adress),@R0
void mov_86(MCU* mcu)
{
  mcu->PC += 1;
  *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true)
  = *mcuIntRAM(mcu, mcu->R[0], false);
  mcu->PC += 1;
}

// mov (adress),@R1
void mov_87(MCU* mcu)
{
  mcu->PC += 1;
  *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true)
  = *mcuIntRAM(mcu, mcu->R[1], false);
  mcu->PC += 1;
}

// jmp @A+DPTR
void jmp_73(MCU* mcu)
{
  mcu->PC = (WORD) (*mcu->ACC + *mcu->DPTR);
}

// cjne A,#(byte),offset
void cjne_B4(MCU* mcu)
{
  mcu->PC += 1;
  if (*mcu->ACC < *mcuROM(mcu, mcu->PC))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  if (*mcu->ACC != *mcuROM(mcu, mcu->PC)) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else {
    mcu->PC += 2;
  }
}

// cjne A,(adress),offset
void cjne_B5(MCU* mcu)
{
  mcu->PC += 1;
  if (*mcu->ACC < *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  if (*mcu->ACC != *mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true)) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else {
    mcu->PC += 2;
  }
}

// cjne @R0,#(byte),offset
void cjne_B6(MCU* mcu)
{
  mcu->PC += 1;
  if (*mcuIntRAM(mcu, mcu->R[0], false) < *mcuROM(mcu, mcu->PC))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  if (*mcuIntRAM(mcu, mcu->R[0], false) != *mcuROM(mcu, mcu->PC)) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else {
    mcu->PC += 2;
  }
}

// cjne @R1,#(byte),offset
void cjne_B7(MCU* mcu)
{
  mcu->PC += 1;
  if (*mcuIntRAM(mcu, mcu->R[1], false) < *mcuROM(mcu, mcu->PC))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  if (*mcuIntRAM(mcu, mcu->R[1], false) != *mcuROM(mcu, mcu->PC)) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else {
    mcu->PC += 2;
  }
}

// cjne R0,#(byte),offset
void cjne_B8(MCU* mcu)
{
  mcu->PC += 1;
  if (mcu->R[0] < *mcuROM(mcu, mcu->PC))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  if (mcu->R[0] != *mcuROM(mcu, mcu->PC)) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else {
    mcu->PC += 2;
  }
}

// cjne R1,#(byte),offset
void cjne_B9(MCU* mcu)
{
  mcu->PC += 1;
  if (mcu->R[1] < *mcuROM(mcu, mcu->PC))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  if (mcu->R[1] != *mcuROM(mcu, mcu->PC)) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else {
    mcu->PC += 2;
  }
}

// cjne R2,#(byte),offset
void cjne_BA(MCU* mcu)
{
  mcu->PC += 1;
  if (mcu->R[2] < *mcuROM(mcu, mcu->PC))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  if (mcu->R[2] != *mcuROM(mcu, mcu->PC)) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else {
    mcu->PC += 2;
  }
}

// cjne R3,#(byte),offset
void cjne_BB(MCU* mcu)
{
  mcu->PC += 1;
  if (mcu->R[3] < *mcuROM(mcu, mcu->PC))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  if (mcu->R[3] != *mcuROM(mcu, mcu->PC)) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else {
    mcu->PC += 2;
  }
}

// cjne R4,#(byte),offset
void cjne_BC(MCU* mcu)
{
  mcu->PC += 1;
  if (mcu->R[4] < *mcuROM(mcu, mcu->PC))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  if (mcu->R[4] != *mcuROM(mcu, mcu->PC)) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else {
    mcu->PC += 2;
  }
}

// cjne R5,#(byte),offset
void cjne_BD(MCU* mcu)
{
  mcu->PC += 1;
  if (mcu->R[5] < *mcuROM(mcu, mcu->PC))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  if (mcu->R[5] != *mcuROM(mcu, mcu->PC)) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else {
    mcu->PC += 2;
  }
}

// cjne R6,#(byte),offset
void cjne_BE(MCU* mcu)
{
  mcu->PC += 1;
  if (mcu->R[6] < *mcuROM(mcu, mcu->PC))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  if (mcu->R[6] != *mcuROM(mcu, mcu->PC)) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else {
    mcu->PC += 2;
  }
}

// cjne R7,#(byte),offset
void cjne_BF(MCU* mcu)
{
  mcu->PC += 1;
  if (mcu->R[7] < *mcuROM(mcu, mcu->PC))
    setRegister(mcu, PSW_CY, true);
  else
    setRegister(mcu, PSW_CY, false);
  if (mcu->R[7] != *mcuROM(mcu, mcu->PC)) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else {
    mcu->PC += 2;
  }
}

// reti
void reti_32(MCU* mcu)
{
  mcu->PC = (WORD) *mcuIntRAM(mcu, *mcu->SP, false) << 8 | (WORD) *mcuIntRAM(mcu,
            *mcu->SP - 1, false);
  *mcu->SP = *mcu->SP - 2;
}

// jnb (bit),(offset)
void jnb_30(MCU* mcu)
{
  mcu->PC += 1;
  if (!mcuCheckBit(mcu, *mcuROM(mcu, mcu->PC))) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else
    mcu->PC += 2;
}

// jb (bit),(offset)
void jb_20(MCU* mcu)
{
  mcu->PC += 1;
  if (mcuCheckBit(mcu, *mcuROM(mcu, mcu->PC))) {
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else
    mcu->PC += 2;
}

// jbc (bit),(offset)
void jbc_10(MCU* mcu)
{
  mcu->PC += 1;
  if (mcuCheckBit(mcu, *mcuROM(mcu, mcu->PC))) {
    mcuSetBit(mcu, *mcuROM(mcu, mcu->PC), false);
    mcu->PC += 1;
    mcu->PC += (signed char) *mcuROM(mcu, mcu->PC) + 1;
  } else
    mcu->PC += 2;
}

// subb A,#(data)
void subb_94(MCU* mcu)
{
  mcu->PC += 1;
  mcuSub(mcu, mcu->ACC, &*mcuROM(mcu, mcu->PC));
  mcu->PC += 1;
}

// subb A,(adress)
void subb_95(MCU* mcu)
{
  mcu->PC += 1;
  mcuSub(mcu, mcu->ACC, mcuIntRAM(mcu, *mcuROM(mcu, mcu->PC), true));
  mcu->PC += 1;
}

// subb A,@R0
void subb_96(MCU* mcu)
{
  mcu->PC += 1;
  mcuSub(mcu, mcu->ACC, mcuIntRAM(mcu, mcu->R[0], false));
  mcu->PC += 1;
}

// subb A,@R1
void subb_97(MCU* mcu)
{
  mcu->PC += 1;
  mcuSub(mcu, mcu->ACC, mcuIntRAM(mcu, mcu->R[1], false));
  mcu->PC += 1;
}

// subb A,R0
void subb_98(MCU* mcu)
{
  mcu->PC += 1;
  mcuSub(mcu, mcu->ACC, &mcu->R[0]);
}

// subb A,R1
void subb_99(MCU* mcu)
{
  mcu->PC += 1;
  mcuSub(mcu, mcu->ACC, &mcu->R[1]);
}

// subb A,R2
void subb_9A(MCU* mcu)
{
  mcu->PC += 1;
  mcuSub(mcu, mcu->ACC, &mcu->R[2]);
}

// subb A,R3
void subb_9B(MCU* mcu)
{
  mcu->PC += 1;
  mcuSub(mcu, mcu->ACC, &mcu->R[3]);
}

// subb A,R4
void subb_9C(MCU* mcu)
{
  mcu->PC += 1;
  mcuSub(mcu, mcu->ACC, &mcu->R[4]);
}

// subb A,R5
void subb_9D(MCU* mcu)
{
  mcu->PC += 1;
  mcuSub(mcu, mcu->ACC, &mcu->R[5]);
}

// subb A,R6
void subb_9E(MCU* mcu)
{
  mcu->PC += 1;
  mcuSub(mcu, mcu->ACC, &mcu->R[6]);
}

// subb A,R7
void subb_9F(MCU* mcu)
{
  mcu->PC += 1;
  mcuSub(mcu, mcu->ACC, &mcu->R[7]);
}

// da A
void da_D4(MCU* mcu)
{
  mcu->PC += 1;
  if (checkRegister(mcu, PSW_AC) || ((*mcu->ACC & 0x0F) > 0x09)) {
    *mcu->ACC += 0x06;
    if ((*mcu->ACC & 0x0F) > 0x0F)
      setRegister(mcu, PSW_CY, true);
  }
  if (checkRegister(mcu, PSW_CY) || ((*mcu->ACC & 0xF0) > 0x90)) {
    *mcu->ACC += 0x60;
    if ((*mcu->ACC & 0xF0) > 0xF0)
      setRegister(mcu, PSW_CY, true);
  }
}

// xchd A,@R0
void xchd_D6(MCU* mcu)
{
  BYTE tempB;

  mcu->PC += 1;
  tempB = *mcuIntRAM(mcu, mcu->R[0], false) & 0xF;
  *mcuIntRAM(mcu, mcu->R[0], false) &= 0xF;
  *mcuIntRAM(mcu, mcu->R[0], false) |= (*mcu->ACC) & 0xF;
  (*mcu->ACC) &= 0xF;
  (*mcu->ACC) |= tempB;
}

// xchd A,@R1
void xchd_D7(MCU* mcu)
{
  BYTE tempB;

  mcu->PC += 1;
  tempB = *mcuIntRAM(mcu, mcu->R[1], false) & 0xF;
  *mcuIntRAM(mcu, mcu->R[1], false) &= 0xF;
  *mcuIntRAM(mcu, mcu->R[1], false) |= (*mcu->ACC) & 0xF;
  (*mcu->ACC) &= 0xF;
  (*mcu->ACC) |= tempB;
}

// movx A,@R0
void movx_E2(MCU* mcu)
{
  *mcu->ACC = *mcuExtRAM(mcu, mcu->R[0]);
  mcu->PC += 1;
}

// A,@R1
void movx_E3(MCU* mcu)
{
  *mcu->ACC = *mcuExtRAM(mcu, mcu->R[1]);
  mcu->PC += 1;
}

// movx @R0,A
void movx_F2(MCU* mcu)
{
  *mcuExtRAM(mcu, mcu->R[0]) = *mcu->ACC;
  mcu->PC += 1;
}

// movx @R1,A
void movx_F3(MCU* mcu)
{
  *mcuExtRAM(mcu, mcu->R[1]) = *mcu->ACC;
  mcu->PC += 1;
}

// Timers
void timers8051(MCU* mcu)
{
  BYTE mode = *mcu->TMOD & 0x03;
  if (checkRegister(mcu, TCON_TR0) && (!checkRegister(mcu, TMOD_GATE0) || checkRegister(
                                         mcu, P3_INT0))) {

    switch (mode) {
    case 0:
      if ((int) (*mcu->TL0) + (int) mcu->cycleCount[mcu->lastInstruction] <= 0x1F)
        *mcu->TL0 += mcu->cycleCount[mcu->lastInstruction];
      else {
        *mcu->TL0 = (BYTE) ((int) *mcu->TL0 - (int) 0x20 + mcu->cycleCount[mcu->lastInstruction]);
        if ((int) (*mcu->TH0) < 0xFF)
          *mcu->TH0 += 1;
        else {
          *mcu->TH0 = 0;
          setRegister(mcu, TCON_TF0, true);
        }
      }
      break;
    case 1:
      if ((int) (*mcu->TL0) + (int) mcu->cycleCount[mcu->lastInstruction] <= 0xFF)
        *mcu->TL0 += mcu->cycleCount[mcu->lastInstruction];
      else {
        *mcu->TL0
        = (BYTE) ((int) *mcu->TL0 - (int) 0x100 + mcu->cycleCount[mcu->lastInstruction]);
        if ((int) (*mcu->TH0) < 0xFF)
          *mcu->TH0 += 1;
        else {
          *mcu->TH0 = 0;
          setRegister(mcu, TCON_TF0, true);
        }
      }
      break;
    case 2:
      if ((int) (*mcu->TL0) + (int) mcu->cycleCount[mcu->lastInstruction] <= 0xFF)
        *mcu->TL0 += mcu->cycleCount[mcu->lastInstruction];
      else {
        *mcu->TL0 = *mcu->TH0;
        setRegister(mcu, TCON_TF0, true);
      }
      break;
    case 3:
      if ((int) (*mcu->TL0) + (int) mcu->cycleCount[mcu->lastInstruction] <= 0xFF)
        *mcu->TL0 += mcu->cycleCount[mcu->lastInstruction];
      else {
        *mcu->TL0
        = (BYTE) ((int) *mcu->TL0 - (int) 0x100 + mcu->cycleCount[mcu->lastInstruction]);
        setRegister(mcu, TCON_TF0, true);
      }
      break;
    }
  }

  // TH0 in mode 3
  if (mode == 3 && checkRegister(mcu, TCON_TR1)) {
    if ((int) (*mcu->TH0) + (int) mcu->cycleCount[mcu->lastInstruction] <= 0xFF)
      *mcu->TH0 += mcu->cycleCount[mcu->lastInstruction];
    else {
      *mcu->TH0 = (BYTE) ((int) *mcu->TH0 - (int) 0x100 + mcu->cycleCount[mcu->lastInstruction]);
      setRegister(mcu, TCON_TF1, true);
    }
  }

  mode = (*mcu->TMOD & 0x30) >> 4;
  if (checkRegister(mcu, TCON_TR1) && (!checkRegister(mcu, TMOD_GATE1) || checkRegister(
                                         mcu, P3_INT1))) {

    switch (mode) {
    case 0:
      if ((int) (*mcu->TL1) + (int) mcu->cycleCount[mcu->lastInstruction] <= 0x1F)
        *mcu->TL1 += mcu->cycleCount[mcu->lastInstruction];
      else {
        *mcu->TL1 = (BYTE) ((int) *mcu->TL1 - (int) 0x20 + mcu->cycleCount[mcu->lastInstruction]);
        if ((int) (*mcu->TH1) < 0xFF)
          *mcu->TH1 += 1;
        else {
          *mcu->TH1 = 0;
          setRegister(mcu, TCON_TF1, true);
        }
      }
      break;
    case 1:
      if ((int) (*mcu->TL1) + (int) mcu->cycleCount[mcu->lastInstruction] <= 0xFF)
        *mcu->TL1 += mcu->cycleCount[mcu->lastInstruction];
      else {
        *mcu->TL1
        = (BYTE) ((int) *mcu->TL1 - (int) 0x100 + mcu->cycleCount[mcu->lastInstruction]);
        if ((int) (*mcu->TH1) < 0xFF)
          *mcu->TH1 += 1;
        else {
          *mcu->TH1 = 0;
          setRegister(mcu, TCON_TF1, true);
        }
      }
      break;
    case 2:
      if ((int) (*mcu->TL1) + (int) mcu->cycleCount[mcu->lastInstruction] <= 0xFF)
        *mcu->TL1 += mcu->cycleCount[mcu->lastInstruction];
      else {
        *mcu->TL1 = *mcu->TH1;
        setRegister(mcu, TCON_TF1, true);
      }
      break;
    case 3:
      // stop
      break;
    }
  }
}

void timers8052(MCU* mcu)
{
  timers8051(mcu);

  // TODO! Prosta obsługa i nie jestem pewien czy dobra ;/.
  if (checkRegister(mcu, T2CON_TR2) && (!checkRegister(mcu, TMOD_GATE0) || checkRegister(
                                          mcu, P3_INT0))) {
    if ((int) (*mcu->TL2) + (int) mcu->cycleCount[mcu->lastInstruction] <= 0xFF)
      *mcu->TL2 += mcu->cycleCount[mcu->lastInstruction];
    else {
      *mcu->TL2 = *mcu->RCAP2L;
      setRegister(mcu, TCON_TF1, true);
    }

    if ((int) (*mcu->TH2) + (int) mcu->cycleCount[mcu->lastInstruction] <= 0xFF)
      *mcu->TH2 += mcu->cycleCount[mcu->lastInstruction];
    else {
      *mcu->TH2 = *mcu->RCAP2H;
      setRegister(mcu, TCON_TF1, true);
    }
  }
}

// Interrupts
void interrupts8051(MCU* mcu)
{
  if (checkRegister(mcu, IE_EA)) {
    // T0
    if (checkRegister(mcu, TCON_TF0) && checkRegister(mcu, IE_ET0)) {
      *mcu->SP += 1;
      mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (mcu->PC & 0x00FF));
      *mcu->SP += 1;
      mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (mcu->PC >> 8));
      mcu->PC = 0x000B;
    }
    // INT1
    if (checkRegister(mcu, TCON_IE1) && checkRegister(mcu, IE_EX1)) {
      *mcu->SP += 1;
      mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (mcu->PC & 0x00FF));
      *mcu->SP += 1;
      mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (mcu->PC >> 8));
      mcu->PC = 0x0013;
    }
    // T1
    if (checkRegister(mcu, TCON_TF1) && checkRegister(mcu, IE_ET1)) {
      *mcu->SP += 1;
      mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (mcu->PC & 0x00FF));
      *mcu->SP += 1;
      mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (mcu->PC >> 8));
      mcu->PC = 0x001B;
    }    // INT0
    if (checkRegister(mcu, TCON_IE0) && checkRegister(mcu, IE_EX0)) {
      *mcu->SP += 1;
      mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (mcu->PC & 0x00FF));
      *mcu->SP += 1;
      mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (mcu->PC >> 8));
      mcu->PC = 0x0003;
    }
  }
}

void interrupts8052(MCU* mcu)
{
  if (checkRegister(mcu, IE_EA)) {
    // T2
    if (checkRegister(mcu, T2CON_TF2) && checkRegister(mcu, T2CON_EXF2)
        && checkRegister(mcu, IE_ET2)) {
      *mcu->SP += 1;
      mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (mcu->PC & 0x00FF));
      *mcu->SP += 1;
      mcuSetIntRAM(mcu, *mcu->SP, false, (BYTE) (mcu->PC >> 8));
      mcu->PC = 0x002B;
    }

    interrupts8051(mcu);
  }
}

// Additional Code
void Mcu89S5x(MCU* mcu)
{
  /*
   * Two DPTR
   */
  if(checkRegister(mcu, AUXR1_DPS))
    mcu->DPTR = (WORD*)&mcu->sfr[0x84 - 0x80];
  else
    mcu->DPTR = (WORD*)&mcu->sfr[0x82 - 0x80];

  /*
   * WDT
   */
  if (mcu->accessedSFR == 0xA6 && mcu->_beforeAccessedSFR == 0x1E && *mcu->WDTRST == 0xE1) {
    mcu->WDTEnable = true;
    mcu->WDTTimerValue = 0;
  }

  if (mcu->WDTEnable) {
    mcu->WDTTimerValue += mcu->cycleCount[mcu->lastInstruction];
    if (mcu->WDTTimerValue > 0x3FFF)
      resetMCU(mcu);
  }
}

void init8051MCU(MCU* mcu)
{
  mcu->mcuType = M_8052;
  mcu->oscillator = 11059200;

  memset(mcu->idata, 0, INT_RAM_SIZE);

  for (int i = 0; i < INT_RAM_SIZE; ++i)
    mcu->idata[i] = rand();

  for (int i = 0; i < MAX_EXT_RAM_SIZE; ++i)
    mcu->xdata[i] = rand();

  mcu->idataMemorySize = 0x80;
  mcu->xdataMemorySize = MAX_EXT_RAM_SIZE;
  mcu->iromMemorySize = 0x1000;
  mcu->xromMemorySize = MAX_ROM_SIZE;

  mcu->EA = 1;

  if (mcu->EA)
    mcu->currentRom = mcu->irom;
  else
    mcu->currentRom = mcu->xrom;

  mcu->autoRead = true;
  mcu->autoWrite = true;

  mcu->PCBreakpoints = NULL;
  mcu->accessIntRAMPauses = NULL;
  mcu->accessExtRAMPauses = NULL;
  mcu->accessIntROMPauses = NULL;
  mcu->accessExtROMPauses = NULL;
  mcu->accessSFRPauses = NULL;
  mcu->intRAMPauses = NULL;
  mcu->extRAMPauses = NULL;
  mcu->SFRPauses = NULL;

  mcu->numOfPCBreakpoints = 0;
  mcu->numOfaccessIntRAMPauses = 0;
  mcu->numOfAccessExtRAMPauses = 0;
  mcu->numOfAccessIntROMPauses = 0;
  mcu->numOfAccessExtROMPauses = 0;
  mcu->numOfAccessSFRPauses = 0;
  mcu->numOfIntRAMPauses = 0;
  mcu->numOfExtRAMPauses = 0;
  mcu->numOfSFRPauses = 0;

  mcu->P0 = &mcu->sfr[0x80 - 0x80];
  mcu->SP = &mcu->sfr[0x81 - 0x80];
  mcu->DPTR = (WORD*)&mcu->sfr[0x82 - 0x80];
  mcu->DPL = &mcu->sfr[0x82 - 0x80];
  mcu->DPH = &mcu->sfr[0x83 - 0x80];
  mcu->PCON = &mcu->sfr[0x87 - 0x80];
  mcu->TCON = &mcu->sfr[0x88 - 0x80];
  mcu->TMOD = &mcu->sfr[0x89 - 0x80];
  mcu->TL0 = &mcu->sfr[0x8A - 0x80];
  mcu->TL1 = &mcu->sfr[0x8B - 0x80];
  mcu->TH0 = &mcu->sfr[0x8C - 0x80];
  mcu->TH1 = &mcu->sfr[0x8D - 0x80];
  mcu->P1 = &mcu->sfr[0x90 - 0x80];
  mcu->SCON = &mcu->sfr[0x98 - 0x80];
  mcu->SBUF = &mcu->sfr[0x99 - 0x80];
  mcu->P2 = &mcu->sfr[0xA0 - 0x80];
  mcu->IE = &mcu->sfr[0xA8 - 0x80];
  mcu->P3 = &mcu->sfr[0xB0 - 0x80];
  mcu->IP = &mcu->sfr[0xC0 - 0x80];
  mcu->T2CON = &mcu->sfr[0xC8 - 0x80];
  mcu->RCAP2L = &mcu->sfr[0xCA - 0x80];
  mcu->RCAP2H = &mcu->sfr[0xCB - 0x80];
  mcu->TL2 = &mcu->sfr[0xCD - 0x80];
  mcu->TH2 = &mcu->sfr[0xCE - 0x80];
  mcu->PSW = &mcu->sfr[0xD0 - 0x80];
  mcu->ACC = &mcu->sfr[0xE0 - 0x80];
  mcu->B = &mcu->sfr[0xF0 - 0x80];
  mcu->R = &mcu->sfr[0x00 - 0x80];

  mcu->cycleCount[0] = 1;
  mcu->cycleCount[1] = 2;
  mcu->cycleCount[2] = 2;
  mcu->cycleCount[3] = 1;
  mcu->cycleCount[4] = 1;
  mcu->cycleCount[5] = 1;
  mcu->cycleCount[6] = 1;
  mcu->cycleCount[7] = 1;
  mcu->cycleCount[8] = 1;
  mcu->cycleCount[9] = 1;
  mcu->cycleCount[10] = 1;
  mcu->cycleCount[11] = 1;
  mcu->cycleCount[12] = 1;
  mcu->cycleCount[13] = 1;
  mcu->cycleCount[14] = 1;
  mcu->cycleCount[15] = 1;
  mcu->cycleCount[16] = 2;
  mcu->cycleCount[17] = 2;
  mcu->cycleCount[18] = 2;
  mcu->cycleCount[19] = 1;
  mcu->cycleCount[20] = 1;
  mcu->cycleCount[21] = 1;
  mcu->cycleCount[22] = 1;
  mcu->cycleCount[23] = 1;
  mcu->cycleCount[24] = 1;
  mcu->cycleCount[25] = 1;
  mcu->cycleCount[26] = 1;
  mcu->cycleCount[27] = 1;
  mcu->cycleCount[28] = 1;
  mcu->cycleCount[29] = 1;
  mcu->cycleCount[30] = 1;
  mcu->cycleCount[31] = 1;
  mcu->cycleCount[32] = 2;
  mcu->cycleCount[33] = 2;
  mcu->cycleCount[34] = 2;
  mcu->cycleCount[35] = 1;
  mcu->cycleCount[36] = 1;
  mcu->cycleCount[37] = 1;
  mcu->cycleCount[38] = 1;
  mcu->cycleCount[39] = 1;
  mcu->cycleCount[40] = 1;
  mcu->cycleCount[41] = 1;
  mcu->cycleCount[42] = 1;
  mcu->cycleCount[43] = 1;
  mcu->cycleCount[44] = 1;
  mcu->cycleCount[45] = 1;
  mcu->cycleCount[46] = 1;
  mcu->cycleCount[47] = 1;
  mcu->cycleCount[48] = 2;
  mcu->cycleCount[49] = 2;
  mcu->cycleCount[50] = 2;
  mcu->cycleCount[51] = 1;
  mcu->cycleCount[52] = 1;
  mcu->cycleCount[53] = 1;
  mcu->cycleCount[54] = 1;
  mcu->cycleCount[55] = 1;
  mcu->cycleCount[56] = 1;
  mcu->cycleCount[57] = 1;
  mcu->cycleCount[58] = 1;
  mcu->cycleCount[59] = 1;
  mcu->cycleCount[60] = 1;
  mcu->cycleCount[61] = 1;
  mcu->cycleCount[62] = 1;
  mcu->cycleCount[63] = 1;
  mcu->cycleCount[64] = 2;
  mcu->cycleCount[65] = 2;
  mcu->cycleCount[66] = 1;
  mcu->cycleCount[67] = 2;
  mcu->cycleCount[68] = 1;
  mcu->cycleCount[69] = 1;
  mcu->cycleCount[70] = 1;
  mcu->cycleCount[71] = 1;
  mcu->cycleCount[72] = 1;
  mcu->cycleCount[73] = 1;
  mcu->cycleCount[74] = 1;
  mcu->cycleCount[75] = 1;
  mcu->cycleCount[76] = 1;
  mcu->cycleCount[77] = 1;
  mcu->cycleCount[78] = 1;
  mcu->cycleCount[79] = 1;
  mcu->cycleCount[80] = 2;
  mcu->cycleCount[81] = 2;
  mcu->cycleCount[82] = 1;
  mcu->cycleCount[83] = 2;
  mcu->cycleCount[84] = 1;
  mcu->cycleCount[85] = 1;
  mcu->cycleCount[86] = 1;
  mcu->cycleCount[87] = 1;
  mcu->cycleCount[88] = 1;
  mcu->cycleCount[89] = 1;
  mcu->cycleCount[90] = 1;
  mcu->cycleCount[91] = 1;
  mcu->cycleCount[92] = 1;
  mcu->cycleCount[93] = 1;
  mcu->cycleCount[94] = 1;
  mcu->cycleCount[95] = 1;
  mcu->cycleCount[96] = 2;
  mcu->cycleCount[97] = 2;
  mcu->cycleCount[98] = 1;
  mcu->cycleCount[99] = 2;
  mcu->cycleCount[100] = 1;
  mcu->cycleCount[101] = 1;
  mcu->cycleCount[102] = 1;
  mcu->cycleCount[103] = 1;
  mcu->cycleCount[104] = 1;
  mcu->cycleCount[105] = 1;
  mcu->cycleCount[106] = 1;
  mcu->cycleCount[107] = 1;
  mcu->cycleCount[108] = 1;
  mcu->cycleCount[109] = 1;
  mcu->cycleCount[110] = 1;
  mcu->cycleCount[111] = 1;
  mcu->cycleCount[112] = 2;
  mcu->cycleCount[113] = 2;
  mcu->cycleCount[114] = 2;
  mcu->cycleCount[115] = 2;
  mcu->cycleCount[116] = 1;
  mcu->cycleCount[117] = 2;
  mcu->cycleCount[118] = 1;
  mcu->cycleCount[119] = 1;
  mcu->cycleCount[120] = 1;
  mcu->cycleCount[121] = 1;
  mcu->cycleCount[122] = 1;
  mcu->cycleCount[123] = 1;
  mcu->cycleCount[124] = 1;
  mcu->cycleCount[125] = 1;
  mcu->cycleCount[126] = 1;
  mcu->cycleCount[127] = 1;
  mcu->cycleCount[128] = 2;
  mcu->cycleCount[129] = 2;
  mcu->cycleCount[130] = 2;
  mcu->cycleCount[131] = 2;
  mcu->cycleCount[132] = 4;
  mcu->cycleCount[133] = 2;
  mcu->cycleCount[134] = 2;
  mcu->cycleCount[135] = 2;
  mcu->cycleCount[136] = 2;
  mcu->cycleCount[137] = 2;
  mcu->cycleCount[138] = 2;
  mcu->cycleCount[139] = 2;
  mcu->cycleCount[140] = 2;
  mcu->cycleCount[141] = 2;
  mcu->cycleCount[142] = 2;
  mcu->cycleCount[143] = 2;
  mcu->cycleCount[144] = 2;
  mcu->cycleCount[145] = 2;
  mcu->cycleCount[146] = 2;
  mcu->cycleCount[147] = 2;
  mcu->cycleCount[148] = 1;
  mcu->cycleCount[149] = 1;
  mcu->cycleCount[150] = 1;
  mcu->cycleCount[151] = 1;
  mcu->cycleCount[152] = 1;
  mcu->cycleCount[153] = 1;
  mcu->cycleCount[154] = 1;
  mcu->cycleCount[155] = 1;
  mcu->cycleCount[156] = 1;
  mcu->cycleCount[157] = 1;
  mcu->cycleCount[158] = 1;
  mcu->cycleCount[159] = 1;
  mcu->cycleCount[160] = 2;
  mcu->cycleCount[161] = 2;
  mcu->cycleCount[162] = 1;
  mcu->cycleCount[163] = 2;
  mcu->cycleCount[164] = 4;
  mcu->cycleCount[165] = 1;
  mcu->cycleCount[166] = 2;
  mcu->cycleCount[167] = 2;
  mcu->cycleCount[168] = 2;
  mcu->cycleCount[169] = 2;
  mcu->cycleCount[170] = 2;
  mcu->cycleCount[171] = 2;
  mcu->cycleCount[172] = 2;
  mcu->cycleCount[173] = 2;
  mcu->cycleCount[174] = 2;
  mcu->cycleCount[175] = 2;
  mcu->cycleCount[176] = 2;
  mcu->cycleCount[177] = 2;
  mcu->cycleCount[178] = 1;
  mcu->cycleCount[179] = 1;
  mcu->cycleCount[180] = 2;
  mcu->cycleCount[181] = 2;
  mcu->cycleCount[182] = 2;
  mcu->cycleCount[183] = 2;
  mcu->cycleCount[184] = 2;
  mcu->cycleCount[185] = 2;
  mcu->cycleCount[186] = 2;
  mcu->cycleCount[187] = 2;
  mcu->cycleCount[188] = 2;
  mcu->cycleCount[189] = 2;
  mcu->cycleCount[190] = 2;
  mcu->cycleCount[191] = 2;
  mcu->cycleCount[192] = 2;
  mcu->cycleCount[193] = 2;
  mcu->cycleCount[194] = 1;
  mcu->cycleCount[195] = 1;
  mcu->cycleCount[196] = 1;
  mcu->cycleCount[197] = 1;
  mcu->cycleCount[198] = 1;
  mcu->cycleCount[199] = 1;
  mcu->cycleCount[200] = 1;
  mcu->cycleCount[201] = 1;
  mcu->cycleCount[202] = 1;
  mcu->cycleCount[203] = 1;
  mcu->cycleCount[204] = 1;
  mcu->cycleCount[205] = 1;
  mcu->cycleCount[206] = 1;
  mcu->cycleCount[207] = 1;
  mcu->cycleCount[208] = 2;
  mcu->cycleCount[209] = 2;
  mcu->cycleCount[210] = 1;
  mcu->cycleCount[211] = 1;
  mcu->cycleCount[212] = 1;
  mcu->cycleCount[213] = 2;
  mcu->cycleCount[214] = 1;
  mcu->cycleCount[215] = 1;
  mcu->cycleCount[216] = 2;
  mcu->cycleCount[217] = 2;
  mcu->cycleCount[218] = 2;
  mcu->cycleCount[219] = 2;
  mcu->cycleCount[220] = 2;
  mcu->cycleCount[221] = 2;
  mcu->cycleCount[222] = 2;
  mcu->cycleCount[223] = 2;
  mcu->cycleCount[224] = 2;
  mcu->cycleCount[225] = 2;
  mcu->cycleCount[226] = 2;
  mcu->cycleCount[227] = 2;
  mcu->cycleCount[228] = 1;
  mcu->cycleCount[229] = 1;
  mcu->cycleCount[230] = 1;
  mcu->cycleCount[231] = 1;
  mcu->cycleCount[232] = 1;
  mcu->cycleCount[233] = 1;
  mcu->cycleCount[234] = 1;
  mcu->cycleCount[235] = 1;
  mcu->cycleCount[236] = 1;
  mcu->cycleCount[237] = 1;
  mcu->cycleCount[238] = 1;
  mcu->cycleCount[239] = 1;
  mcu->cycleCount[240] = 2;
  mcu->cycleCount[241] = 2;
  mcu->cycleCount[242] = 2;
  mcu->cycleCount[243] = 2;
  mcu->cycleCount[244] = 1;
  mcu->cycleCount[245] = 1;
  mcu->cycleCount[246] = 1;
  mcu->cycleCount[247] = 1;
  mcu->cycleCount[248] = 1;
  mcu->cycleCount[249] = 1;
  mcu->cycleCount[250] = 1;
  mcu->cycleCount[251] = 1;
  mcu->cycleCount[252] = 1;
  mcu->cycleCount[253] = 1;
  mcu->cycleCount[254] = 1;
  mcu->cycleCount[255] = 1;

  mcu->byteCount[0] = 1;
  mcu->byteCount[1] = 2;
  mcu->byteCount[2] = 3;
  mcu->byteCount[3] = 1;
  mcu->byteCount[4] = 1;
  mcu->byteCount[5] = 2;
  mcu->byteCount[6] = 1;
  mcu->byteCount[7] = 1;
  mcu->byteCount[8] = 1;
  mcu->byteCount[9] = 1;
  mcu->byteCount[10] = 1;
  mcu->byteCount[11] = 1;
  mcu->byteCount[12] = 1;
  mcu->byteCount[13] = 1;
  mcu->byteCount[14] = 1;
  mcu->byteCount[15] = 1;
  mcu->byteCount[16] = 3;
  mcu->byteCount[17] = 2;
  mcu->byteCount[18] = 3;
  mcu->byteCount[19] = 1;
  mcu->byteCount[20] = 1;
  mcu->byteCount[21] = 2;
  mcu->byteCount[22] = 1;
  mcu->byteCount[23] = 1;
  mcu->byteCount[24] = 1;
  mcu->byteCount[25] = 1;
  mcu->byteCount[26] = 1;
  mcu->byteCount[27] = 1;
  mcu->byteCount[28] = 1;
  mcu->byteCount[29] = 1;
  mcu->byteCount[30] = 1;
  mcu->byteCount[31] = 1;
  mcu->byteCount[32] = 3;
  mcu->byteCount[33] = 2;
  mcu->byteCount[34] = 1;
  mcu->byteCount[35] = 1;
  mcu->byteCount[36] = 2;
  mcu->byteCount[37] = 2;
  mcu->byteCount[38] = 1;
  mcu->byteCount[39] = 1;
  mcu->byteCount[40] = 1;
  mcu->byteCount[41] = 1;
  mcu->byteCount[42] = 1;
  mcu->byteCount[43] = 1;
  mcu->byteCount[44] = 1;
  mcu->byteCount[45] = 1;
  mcu->byteCount[46] = 1;
  mcu->byteCount[47] = 1;
  mcu->byteCount[48] = 3;
  mcu->byteCount[49] = 2;
  mcu->byteCount[50] = 1;
  mcu->byteCount[51] = 1;
  mcu->byteCount[52] = 2;
  mcu->byteCount[53] = 2;
  mcu->byteCount[54] = 1;
  mcu->byteCount[55] = 1;
  mcu->byteCount[56] = 1;
  mcu->byteCount[57] = 1;
  mcu->byteCount[58] = 1;
  mcu->byteCount[59] = 1;
  mcu->byteCount[60] = 1;
  mcu->byteCount[61] = 1;
  mcu->byteCount[62] = 1;
  mcu->byteCount[63] = 1;
  mcu->byteCount[64] = 2;
  mcu->byteCount[65] = 2;
  mcu->byteCount[66] = 2;
  mcu->byteCount[67] = 3;
  mcu->byteCount[68] = 2;
  mcu->byteCount[69] = 2;
  mcu->byteCount[70] = 1;
  mcu->byteCount[71] = 1;
  mcu->byteCount[72] = 1;
  mcu->byteCount[73] = 1;
  mcu->byteCount[74] = 1;
  mcu->byteCount[75] = 1;
  mcu->byteCount[76] = 1;
  mcu->byteCount[77] = 1;
  mcu->byteCount[78] = 1;
  mcu->byteCount[79] = 1;
  mcu->byteCount[80] = 2;
  mcu->byteCount[81] = 2;
  mcu->byteCount[82] = 2;
  mcu->byteCount[83] = 3;
  mcu->byteCount[84] = 2;
  mcu->byteCount[85] = 2;
  mcu->byteCount[86] = 1;
  mcu->byteCount[87] = 1;
  mcu->byteCount[88] = 1;
  mcu->byteCount[89] = 1;
  mcu->byteCount[90] = 1;
  mcu->byteCount[91] = 1;
  mcu->byteCount[92] = 1;
  mcu->byteCount[93] = 1;
  mcu->byteCount[94] = 1;
  mcu->byteCount[95] = 1;
  mcu->byteCount[96] = 2;
  mcu->byteCount[97] = 2;
  mcu->byteCount[98] = 2;
  mcu->byteCount[99] = 3;
  mcu->byteCount[100] = 2;
  mcu->byteCount[101] = 2;
  mcu->byteCount[102] = 1;
  mcu->byteCount[103] = 1;
  mcu->byteCount[104] = 1;
  mcu->byteCount[105] = 1;
  mcu->byteCount[106] = 1;
  mcu->byteCount[107] = 1;
  mcu->byteCount[108] = 1;
  mcu->byteCount[109] = 1;
  mcu->byteCount[110] = 1;
  mcu->byteCount[111] = 1;
  mcu->byteCount[112] = 2;
  mcu->byteCount[113] = 2;
  mcu->byteCount[114] = 2;
  mcu->byteCount[115] = 1;
  mcu->byteCount[116] = 2;
  mcu->byteCount[117] = 3;
  mcu->byteCount[118] = 2;
  mcu->byteCount[119] = 2;
  mcu->byteCount[120] = 2;
  mcu->byteCount[121] = 2;
  mcu->byteCount[122] = 2;
  mcu->byteCount[123] = 2;
  mcu->byteCount[124] = 2;
  mcu->byteCount[125] = 2;
  mcu->byteCount[126] = 2;
  mcu->byteCount[127] = 2;
  mcu->byteCount[128] = 2;
  mcu->byteCount[129] = 2;
  mcu->byteCount[130] = 2;
  mcu->byteCount[131] = 1;
  mcu->byteCount[132] = 1;
  mcu->byteCount[133] = 3;
  mcu->byteCount[134] = 2;
  mcu->byteCount[135] = 2;
  mcu->byteCount[136] = 2;
  mcu->byteCount[137] = 2;
  mcu->byteCount[138] = 2;
  mcu->byteCount[139] = 2;
  mcu->byteCount[140] = 2;
  mcu->byteCount[141] = 2;
  mcu->byteCount[142] = 2;
  mcu->byteCount[143] = 2;
  mcu->byteCount[144] = 3;
  mcu->byteCount[145] = 2;
  mcu->byteCount[146] = 2;
  mcu->byteCount[147] = 1;
  mcu->byteCount[148] = 2;
  mcu->byteCount[149] = 2;
  mcu->byteCount[150] = 1;
  mcu->byteCount[151] = 1;
  mcu->byteCount[152] = 1;
  mcu->byteCount[153] = 1;
  mcu->byteCount[154] = 1;
  mcu->byteCount[155] = 1;
  mcu->byteCount[156] = 1;
  mcu->byteCount[157] = 1;
  mcu->byteCount[158] = 1;
  mcu->byteCount[159] = 1;
  mcu->byteCount[160] = 2;
  mcu->byteCount[161] = 2;
  mcu->byteCount[162] = 2;
  mcu->byteCount[163] = 1;
  mcu->byteCount[164] = 1;
  mcu->byteCount[165] = 1;
  mcu->byteCount[166] = 2;
  mcu->byteCount[167] = 2;
  mcu->byteCount[168] = 2;
  mcu->byteCount[169] = 2;
  mcu->byteCount[170] = 2;
  mcu->byteCount[171] = 2;
  mcu->byteCount[172] = 2;
  mcu->byteCount[173] = 2;
  mcu->byteCount[174] = 2;
  mcu->byteCount[175] = 2;
  mcu->byteCount[176] = 2;
  mcu->byteCount[177] = 2;
  mcu->byteCount[178] = 2;
  mcu->byteCount[179] = 1;
  mcu->byteCount[180] = 3;
  mcu->byteCount[181] = 3;
  mcu->byteCount[182] = 3;
  mcu->byteCount[183] = 3;
  mcu->byteCount[184] = 3;
  mcu->byteCount[185] = 3;
  mcu->byteCount[186] = 3;
  mcu->byteCount[187] = 3;
  mcu->byteCount[188] = 3;
  mcu->byteCount[189] = 3;
  mcu->byteCount[190] = 3;
  mcu->byteCount[191] = 3;
  mcu->byteCount[192] = 2;
  mcu->byteCount[193] = 2;
  mcu->byteCount[194] = 2;
  mcu->byteCount[195] = 1;
  mcu->byteCount[196] = 1;
  mcu->byteCount[197] = 2;
  mcu->byteCount[198] = 1;
  mcu->byteCount[199] = 1;
  mcu->byteCount[200] = 1;
  mcu->byteCount[201] = 1;
  mcu->byteCount[202] = 1;
  mcu->byteCount[203] = 1;
  mcu->byteCount[204] = 1;
  mcu->byteCount[205] = 1;
  mcu->byteCount[206] = 1;
  mcu->byteCount[207] = 1;
  mcu->byteCount[208] = 2;
  mcu->byteCount[209] = 2;
  mcu->byteCount[210] = 2;
  mcu->byteCount[211] = 1;
  mcu->byteCount[212] = 1;
  mcu->byteCount[213] = 3;
  mcu->byteCount[214] = 1;
  mcu->byteCount[215] = 1;
  mcu->byteCount[216] = 2;
  mcu->byteCount[217] = 2;
  mcu->byteCount[218] = 2;
  mcu->byteCount[219] = 2;
  mcu->byteCount[220] = 2;
  mcu->byteCount[221] = 2;
  mcu->byteCount[222] = 2;
  mcu->byteCount[223] = 2;
  mcu->byteCount[224] = 1;
  mcu->byteCount[225] = 2;
  mcu->byteCount[226] = 1;
  mcu->byteCount[227] = 1;
  mcu->byteCount[228] = 1;
  mcu->byteCount[229] = 2;
  mcu->byteCount[230] = 1;
  mcu->byteCount[231] = 1;
  mcu->byteCount[232] = 1;
  mcu->byteCount[233] = 1;
  mcu->byteCount[234] = 1;
  mcu->byteCount[235] = 1;
  mcu->byteCount[236] = 1;
  mcu->byteCount[237] = 1;
  mcu->byteCount[238] = 1;
  mcu->byteCount[239] = 1;
  mcu->byteCount[240] = 1;
  mcu->byteCount[241] = 2;
  mcu->byteCount[242] = 1;
  mcu->byteCount[243] = 1;
  mcu->byteCount[244] = 1;
  mcu->byteCount[245] = 2;
  mcu->byteCount[246] = 1;
  mcu->byteCount[247] = 1;
  mcu->byteCount[248] = 1;
  mcu->byteCount[249] = 1;
  mcu->byteCount[250] = 1;
  mcu->byteCount[251] = 1;
  mcu->byteCount[252] = 1;
  mcu->byteCount[253] = 1;
  mcu->byteCount[254] = 1;
  mcu->byteCount[255] = 1;

  mcu->mnemonicTable[0] = "nop  ";
  mcu->mnemonicTable[1] = "ajmp ";
  mcu->mnemonicTable[2] = "ljmp ";
  mcu->mnemonicTable[3] = "rr   ";
  mcu->mnemonicTable[4] = "inc  ";
  mcu->mnemonicTable[5] = "inc  ";
  mcu->mnemonicTable[6] = "inc  ";
  mcu->mnemonicTable[7] = "inc  ";
  mcu->mnemonicTable[8] = "inc  ";
  mcu->mnemonicTable[9] = "inc  ";
  mcu->mnemonicTable[10] = "inc  ";
  mcu->mnemonicTable[11] = "inc  ";
  mcu->mnemonicTable[12] = "inc  ";
  mcu->mnemonicTable[13] = "inc  ";
  mcu->mnemonicTable[14] = "inc  ";
  mcu->mnemonicTable[15] = "inc  ";
  mcu->mnemonicTable[16] = "jbc  ";
  mcu->mnemonicTable[17] = "acall";
  mcu->mnemonicTable[18] = "lcall";
  mcu->mnemonicTable[19] = "rrc  ";
  mcu->mnemonicTable[20] = "dec  ";
  mcu->mnemonicTable[21] = "dec  ";
  mcu->mnemonicTable[22] = "dec  ";
  mcu->mnemonicTable[23] = "dec  ";
  mcu->mnemonicTable[24] = "dec  ";
  mcu->mnemonicTable[25] = "dec  ";
  mcu->mnemonicTable[26] = "dec  ";
  mcu->mnemonicTable[27] = "dec  ";
  mcu->mnemonicTable[28] = "dec  ";
  mcu->mnemonicTable[29] = "dec  ";
  mcu->mnemonicTable[30] = "dec  ";
  mcu->mnemonicTable[31] = "dec  ";
  mcu->mnemonicTable[32] = "jb   ";
  mcu->mnemonicTable[33] = "ajmp ";
  mcu->mnemonicTable[34] = "ret  ";
  mcu->mnemonicTable[35] = "rl   ";
  mcu->mnemonicTable[36] = "add  ";
  mcu->mnemonicTable[37] = "add  ";
  mcu->mnemonicTable[38] = "add  ";
  mcu->mnemonicTable[39] = "add  ";
  mcu->mnemonicTable[40] = "add  ";
  mcu->mnemonicTable[41] = "add  ";
  mcu->mnemonicTable[42] = "add  ";
  mcu->mnemonicTable[43] = "add  ";
  mcu->mnemonicTable[44] = "add  ";
  mcu->mnemonicTable[45] = "add  ";
  mcu->mnemonicTable[46] = "add  ";
  mcu->mnemonicTable[47] = "add  ";
  mcu->mnemonicTable[48] = "jnb  ";
  mcu->mnemonicTable[49] = "acall";
  mcu->mnemonicTable[50] = "reti ";
  mcu->mnemonicTable[51] = "rlc  ";
  mcu->mnemonicTable[52] = "addc ";
  mcu->mnemonicTable[53] = "addc ";
  mcu->mnemonicTable[54] = "addc ";
  mcu->mnemonicTable[55] = "addc ";
  mcu->mnemonicTable[56] = "addc ";
  mcu->mnemonicTable[57] = "addc ";
  mcu->mnemonicTable[58] = "addc ";
  mcu->mnemonicTable[59] = "addc ";
  mcu->mnemonicTable[60] = "addc ";
  mcu->mnemonicTable[61] = "addc ";
  mcu->mnemonicTable[62] = "addc ";
  mcu->mnemonicTable[63] = "addc ";
  mcu->mnemonicTable[64] = "jc   ";
  mcu->mnemonicTable[65] = "ajmp ";
  mcu->mnemonicTable[66] = "orl  ";
  mcu->mnemonicTable[67] = "orl  ";
  mcu->mnemonicTable[68] = "orl  ";
  mcu->mnemonicTable[69] = "orl  ";
  mcu->mnemonicTable[70] = "orl  ";
  mcu->mnemonicTable[71] = "orl  ";
  mcu->mnemonicTable[72] = "orl  ";
  mcu->mnemonicTable[73] = "orl  ";
  mcu->mnemonicTable[74] = "orl  ";
  mcu->mnemonicTable[75] = "orl  ";
  mcu->mnemonicTable[76] = "orl  ";
  mcu->mnemonicTable[77] = "orl  ";
  mcu->mnemonicTable[78] = "orl  ";
  mcu->mnemonicTable[79] = "orl  ";
  mcu->mnemonicTable[80] = "jnc  ";
  mcu->mnemonicTable[81] = "acall";
  mcu->mnemonicTable[82] = "anl  ";
  mcu->mnemonicTable[83] = "anl  ";
  mcu->mnemonicTable[84] = "anl  ";
  mcu->mnemonicTable[85] = "anl  ";
  mcu->mnemonicTable[86] = "anl  ";
  mcu->mnemonicTable[87] = "anl  ";
  mcu->mnemonicTable[88] = "anl  ";
  mcu->mnemonicTable[89] = "anl  ";
  mcu->mnemonicTable[90] = "anl  ";
  mcu->mnemonicTable[91] = "anl  ";
  mcu->mnemonicTable[92] = "anl  ";
  mcu->mnemonicTable[93] = "anl  ";
  mcu->mnemonicTable[94] = "anl  ";
  mcu->mnemonicTable[95] = "anl  ";
  mcu->mnemonicTable[96] = "jz   ";
  mcu->mnemonicTable[97] = "ajmp ";
  mcu->mnemonicTable[98] = "xrl  ";
  mcu->mnemonicTable[99] = "xrl  ";
  mcu->mnemonicTable[100] = "xrl  ";
  mcu->mnemonicTable[101] = "xrl  ";
  mcu->mnemonicTable[102] = "xrl  ";
  mcu->mnemonicTable[103] = "xrl  ";
  mcu->mnemonicTable[104] = "xrl  ";
  mcu->mnemonicTable[105] = "xrl  ";
  mcu->mnemonicTable[106] = "xrl  ";
  mcu->mnemonicTable[107] = "xrl  ";
  mcu->mnemonicTable[108] = "xrl  ";
  mcu->mnemonicTable[109] = "xrl  ";
  mcu->mnemonicTable[110] = "xrl  ";
  mcu->mnemonicTable[111] = "xrl  ";
  mcu->mnemonicTable[112] = "jnz  ";
  mcu->mnemonicTable[113] = "acall";
  mcu->mnemonicTable[114] = "orl  ";
  mcu->mnemonicTable[115] = "jmp  ";
  mcu->mnemonicTable[116] = "mov  ";
  mcu->mnemonicTable[117] = "mov  ";
  mcu->mnemonicTable[118] = "mov  ";
  mcu->mnemonicTable[119] = "mov  ";
  mcu->mnemonicTable[120] = "mov  ";
  mcu->mnemonicTable[121] = "mov  ";
  mcu->mnemonicTable[122] = "mov  ";
  mcu->mnemonicTable[123] = "mov  ";
  mcu->mnemonicTable[124] = "mov  ";
  mcu->mnemonicTable[125] = "mov  ";
  mcu->mnemonicTable[126] = "mov  ";
  mcu->mnemonicTable[127] = "mov  ";
  mcu->mnemonicTable[128] = "sjmp ";
  mcu->mnemonicTable[129] = "ajmp ";
  mcu->mnemonicTable[130] = "anl  ";
  mcu->mnemonicTable[131] = "movc ";
  mcu->mnemonicTable[132] = "div  ";
  mcu->mnemonicTable[133] = "mov  ";
  mcu->mnemonicTable[134] = "mov  ";
  mcu->mnemonicTable[135] = "mov  ";
  mcu->mnemonicTable[136] = "mov  ";
  mcu->mnemonicTable[137] = "mov  ";
  mcu->mnemonicTable[138] = "mov  ";
  mcu->mnemonicTable[139] = "mov  ";
  mcu->mnemonicTable[140] = "mov  ";
  mcu->mnemonicTable[141] = "mov  ";
  mcu->mnemonicTable[142] = "mov  ";
  mcu->mnemonicTable[143] = "mov  ";
  mcu->mnemonicTable[144] = "mov  ";
  mcu->mnemonicTable[145] = "acall";
  mcu->mnemonicTable[146] = "mov  ";
  mcu->mnemonicTable[147] = "movc ";
  mcu->mnemonicTable[148] = "subb ";
  mcu->mnemonicTable[149] = "subb ";
  mcu->mnemonicTable[150] = "subb ";
  mcu->mnemonicTable[151] = "subb ";
  mcu->mnemonicTable[152] = "subb ";
  mcu->mnemonicTable[153] = "subb ";
  mcu->mnemonicTable[154] = "subb ";
  mcu->mnemonicTable[155] = "subb ";
  mcu->mnemonicTable[156] = "subb ";
  mcu->mnemonicTable[157] = "subb ";
  mcu->mnemonicTable[158] = "subb ";
  mcu->mnemonicTable[159] = "subb ";
  mcu->mnemonicTable[160] = "orl  ";
  mcu->mnemonicTable[161] = "ajmp ";
  mcu->mnemonicTable[162] = "mov  ";
  mcu->mnemonicTable[163] = "inc  ";
  mcu->mnemonicTable[164] = "mul  ";
  mcu->mnemonicTable[165] = "RESVD";
  mcu->mnemonicTable[166] = "mov  ";
  mcu->mnemonicTable[167] = "mov  ";
  mcu->mnemonicTable[168] = "mov  ";
  mcu->mnemonicTable[169] = "mov  ";
  mcu->mnemonicTable[170] = "mov  ";
  mcu->mnemonicTable[171] = "mov  ";
  mcu->mnemonicTable[172] = "mov  ";
  mcu->mnemonicTable[173] = "mov  ";
  mcu->mnemonicTable[174] = "mov  ";
  mcu->mnemonicTable[175] = "mov  ";
  mcu->mnemonicTable[176] = "anl  ";
  mcu->mnemonicTable[177] = "acall";
  mcu->mnemonicTable[178] = "cpl  ";
  mcu->mnemonicTable[179] = "cpl  ";
  mcu->mnemonicTable[180] = "cjne ";
  mcu->mnemonicTable[181] = "cjne ";
  mcu->mnemonicTable[182] = "cjne ";
  mcu->mnemonicTable[183] = "cjne ";
  mcu->mnemonicTable[184] = "cjne ";
  mcu->mnemonicTable[185] = "cjne ";
  mcu->mnemonicTable[186] = "cjne ";
  mcu->mnemonicTable[187] = "cjne ";
  mcu->mnemonicTable[188] = "cjne ";
  mcu->mnemonicTable[189] = "cjne ";
  mcu->mnemonicTable[190] = "cjne ";
  mcu->mnemonicTable[191] = "cjne ";
  mcu->mnemonicTable[192] = "push ";
  mcu->mnemonicTable[193] = "ajmp ";
  mcu->mnemonicTable[194] = "clr  ";
  mcu->mnemonicTable[195] = "clr  ";
  mcu->mnemonicTable[196] = "swap ";
  mcu->mnemonicTable[197] = "xch  ";
  mcu->mnemonicTable[198] = "xch  ";
  mcu->mnemonicTable[199] = "xch  ";
  mcu->mnemonicTable[200] = "xch  ";
  mcu->mnemonicTable[201] = "xch  ";
  mcu->mnemonicTable[202] = "xch  ";
  mcu->mnemonicTable[203] = "xch  ";
  mcu->mnemonicTable[204] = "xch  ";
  mcu->mnemonicTable[205] = "xch  ";
  mcu->mnemonicTable[206] = "xch  ";
  mcu->mnemonicTable[207] = "xch  ";
  mcu->mnemonicTable[208] = "pop  ";
  mcu->mnemonicTable[209] = "acall";
  mcu->mnemonicTable[210] = "setb ";
  mcu->mnemonicTable[211] = "setb ";
  mcu->mnemonicTable[212] = "da   ";
  mcu->mnemonicTable[213] = "djnz ";
  mcu->mnemonicTable[214] = "xchd ";
  mcu->mnemonicTable[215] = "xchd ";
  mcu->mnemonicTable[216] = "djnz ";
  mcu->mnemonicTable[217] = "djnz ";
  mcu->mnemonicTable[218] = "djnz ";
  mcu->mnemonicTable[219] = "djnz ";
  mcu->mnemonicTable[220] = "djnz ";
  mcu->mnemonicTable[221] = "djnz ";
  mcu->mnemonicTable[222] = "djnz ";
  mcu->mnemonicTable[223] = "djnz ";
  mcu->mnemonicTable[224] = "movx ";
  mcu->mnemonicTable[225] = "ajmp ";
  mcu->mnemonicTable[226] = "movx ";
  mcu->mnemonicTable[227] = "movx ";
  mcu->mnemonicTable[228] = "clr  ";
  mcu->mnemonicTable[229] = "mov  ";
  mcu->mnemonicTable[230] = "mov  ";
  mcu->mnemonicTable[231] = "mov  ";
  mcu->mnemonicTable[232] = "mov  ";
  mcu->mnemonicTable[233] = "mov  ";
  mcu->mnemonicTable[234] = "mov  ";
  mcu->mnemonicTable[235] = "mov  ";
  mcu->mnemonicTable[236] = "mov  ";
  mcu->mnemonicTable[237] = "mov  ";
  mcu->mnemonicTable[238] = "mov  ";
  mcu->mnemonicTable[239] = "mov  ";
  mcu->mnemonicTable[240] = "movx ";
  mcu->mnemonicTable[241] = "acall";
  mcu->mnemonicTable[242] = "movx ";
  mcu->mnemonicTable[243] = "movx ";
  mcu->mnemonicTable[244] = "cpl  ";
  mcu->mnemonicTable[245] = "mov  ";
  mcu->mnemonicTable[246] = "mov  ";
  mcu->mnemonicTable[247] = "mov  ";
  mcu->mnemonicTable[248] = "mov  ";
  mcu->mnemonicTable[249] = "mov  ";
  mcu->mnemonicTable[250] = "mov  ";
  mcu->mnemonicTable[251] = "mov  ";
  mcu->mnemonicTable[252] = "mov  ";
  mcu->mnemonicTable[253] = "mov  ";
  mcu->mnemonicTable[254] = "mov  ";
  mcu->mnemonicTable[255] = "mov  ";

  mcu->mnemonicParams[0] = "";
  mcu->mnemonicParams[1] = "00N%1";
  mcu->mnemonicParams[2] = "N%1N%2";
  mcu->mnemonicParams[3] = "A";
  mcu->mnemonicParams[4] = "A";
  mcu->mnemonicParams[5] = "%1";
  mcu->mnemonicParams[6] = "@R0";
  mcu->mnemonicParams[7] = "@R1";
  mcu->mnemonicParams[8] = "R0";
  mcu->mnemonicParams[9] = "R1";
  mcu->mnemonicParams[10] = "R2";
  mcu->mnemonicParams[11] = "R3";
  mcu->mnemonicParams[12] = "R4";
  mcu->mnemonicParams[13] = "R5";
  mcu->mnemonicParams[14] = "R6";
  mcu->mnemonicParams[15] = "R7";
  mcu->mnemonicParams[16] = "0%1, O%2";
  mcu->mnemonicParams[17] = "00N%1";
  mcu->mnemonicParams[18] = "N%1N%2";
  mcu->mnemonicParams[19] = "A";
  mcu->mnemonicParams[20] = "A";
  mcu->mnemonicParams[21] = "%1";
  mcu->mnemonicParams[22] = "@R0";
  mcu->mnemonicParams[23] = "@R1";
  mcu->mnemonicParams[24] = "R0";
  mcu->mnemonicParams[25] = "R1";
  mcu->mnemonicParams[26] = "R2";
  mcu->mnemonicParams[27] = "R3";
  mcu->mnemonicParams[28] = "R4";
  mcu->mnemonicParams[29] = "R5";
  mcu->mnemonicParams[30] = "R6";
  mcu->mnemonicParams[31] = "R7";
  mcu->mnemonicParams[32] = "0%1, O%2";
  mcu->mnemonicParams[33] = "01N%1";
  mcu->mnemonicParams[34] = "";
  mcu->mnemonicParams[35] = "A";
  mcu->mnemonicParams[36] = "A, #%1";
  mcu->mnemonicParams[37] = "A, %1";
  mcu->mnemonicParams[38] = "A, @R0";
  mcu->mnemonicParams[39] = "A, @R1";
  mcu->mnemonicParams[40] = "A, R0";
  mcu->mnemonicParams[41] = "A, R1";
  mcu->mnemonicParams[42] = "A, R2";
  mcu->mnemonicParams[43] = "A, R3";
  mcu->mnemonicParams[44] = "A, R4";
  mcu->mnemonicParams[45] = "A, R5";
  mcu->mnemonicParams[46] = "A, R6";
  mcu->mnemonicParams[47] = "A, R7";
  mcu->mnemonicParams[48] = "0%1, O%2";
  mcu->mnemonicParams[49] = "01N%1";
  mcu->mnemonicParams[50] = "";
  mcu->mnemonicParams[51] = "A";
  mcu->mnemonicParams[52] = "A, #%1";
  mcu->mnemonicParams[53] = "A, %1";
  mcu->mnemonicParams[54] = "A, @R0";
  mcu->mnemonicParams[55] = "A, @R1";
  mcu->mnemonicParams[56] = "A, R0";
  mcu->mnemonicParams[57] = "A, R1";
  mcu->mnemonicParams[58] = "A, R2";
  mcu->mnemonicParams[59] = "A, R3";
  mcu->mnemonicParams[60] = "A, R4";
  mcu->mnemonicParams[61] = "A, R5";
  mcu->mnemonicParams[62] = "A, R6";
  mcu->mnemonicParams[63] = "A, R7";
  mcu->mnemonicParams[64] = "O%1";
  mcu->mnemonicParams[65] = "02N%1";
  mcu->mnemonicParams[66] = "%1, A";
  mcu->mnemonicParams[67] = "%1, #%2";
  mcu->mnemonicParams[68] = "A, #%1";
  mcu->mnemonicParams[69] = "A, %1";
  mcu->mnemonicParams[70] = "A, @R0";
  mcu->mnemonicParams[71] = "A, @R1";
  mcu->mnemonicParams[72] = "A, R0";
  mcu->mnemonicParams[73] = "A, R1";
  mcu->mnemonicParams[74] = "A, R2";
  mcu->mnemonicParams[75] = "A, R3";
  mcu->mnemonicParams[76] = "A, R4";
  mcu->mnemonicParams[77] = "A, R5";
  mcu->mnemonicParams[78] = "A, R6";
  mcu->mnemonicParams[79] = "A, R7";
  mcu->mnemonicParams[80] = "O%1";
  mcu->mnemonicParams[81] = "02N%1";
  mcu->mnemonicParams[82] = "%1, A";
  mcu->mnemonicParams[83] = "%1, #%2";
  mcu->mnemonicParams[84] = "A, #%1";
  mcu->mnemonicParams[85] = "A, %1";
  mcu->mnemonicParams[86] = "A, @R0";
  mcu->mnemonicParams[87] = "A, @R1";
  mcu->mnemonicParams[88] = "A, R0";
  mcu->mnemonicParams[89] = "A, R1";
  mcu->mnemonicParams[90] = "A, R2";
  mcu->mnemonicParams[91] = "A, R3";
  mcu->mnemonicParams[92] = "A, R4";
  mcu->mnemonicParams[93] = "A, R5";
  mcu->mnemonicParams[94] = "A, R6";
  mcu->mnemonicParams[95] = "A, R7";
  mcu->mnemonicParams[96] = "O%1";
  mcu->mnemonicParams[97] = "03N%1";
  mcu->mnemonicParams[98] = "%1, A";
  mcu->mnemonicParams[99] = "%1, #%2";
  mcu->mnemonicParams[100] = "A, #%1";
  mcu->mnemonicParams[101] = "A, %1";
  mcu->mnemonicParams[102] = "A, @R0";
  mcu->mnemonicParams[103] = "A, @R1";
  mcu->mnemonicParams[104] = "A, R0";
  mcu->mnemonicParams[105] = "A, R1";
  mcu->mnemonicParams[106] = "A, R2";
  mcu->mnemonicParams[107] = "A, R3";
  mcu->mnemonicParams[108] = "A, R4";
  mcu->mnemonicParams[109] = "A, R5";
  mcu->mnemonicParams[110] = "A, R6";
  mcu->mnemonicParams[111] = "A, R7";
  mcu->mnemonicParams[112] = "O%1";
  mcu->mnemonicParams[113] = "03N%1";
  mcu->mnemonicParams[114] = "C, 0%1";
  mcu->mnemonicParams[115] = "@A+DPTR";
  mcu->mnemonicParams[116] = "A, #%1";
  mcu->mnemonicParams[117] = "%1, #%2";
  mcu->mnemonicParams[118] = "@R0, #%1";
  mcu->mnemonicParams[119] = "@R1, #%1";
  mcu->mnemonicParams[120] = "R0, #%1";
  mcu->mnemonicParams[121] = "R1, #%1";
  mcu->mnemonicParams[122] = "R2, #%1";
  mcu->mnemonicParams[123] = "R3, #%1";
  mcu->mnemonicParams[124] = "R4, #%1";
  mcu->mnemonicParams[125] = "R5, #%1";
  mcu->mnemonicParams[126] = "R6, #%1";
  mcu->mnemonicParams[127] = "R7, #%1";
  mcu->mnemonicParams[128] = "O%1";
  mcu->mnemonicParams[129] = "04N%1";
  mcu->mnemonicParams[130] = "C, 0%1";
  mcu->mnemonicParams[131] = "A, @A+PC";
  mcu->mnemonicParams[132] = "AB";
  mcu->mnemonicParams[133] = "%2, %1";
  mcu->mnemonicParams[134] = "%1, @R0";
  mcu->mnemonicParams[135] = "%1, @R1";
  mcu->mnemonicParams[136] = "%1, R0";
  mcu->mnemonicParams[137] = "%1, R1";
  mcu->mnemonicParams[138] = "%1, R2";
  mcu->mnemonicParams[139] = "%1, R3";
  mcu->mnemonicParams[140] = "%1, R4";
  mcu->mnemonicParams[141] = "%1, R5";
  mcu->mnemonicParams[142] = "%1, R6";
  mcu->mnemonicParams[143] = "%1, R7";
  mcu->mnemonicParams[144] = "DPTR, #N%1N%2";
  mcu->mnemonicParams[145] = "04N%1";
  mcu->mnemonicParams[146] = "0%1, C";
  mcu->mnemonicParams[147] = "A, @A+DPTR";
  mcu->mnemonicParams[148] = "A, #%1";
  mcu->mnemonicParams[149] = "A, %1";
  mcu->mnemonicParams[150] = "A, @R0";
  mcu->mnemonicParams[151] = "A, @R1";
  mcu->mnemonicParams[152] = "A, R0";
  mcu->mnemonicParams[153] = "A, R1";
  mcu->mnemonicParams[154] = "A, R2";
  mcu->mnemonicParams[155] = "A, R3";
  mcu->mnemonicParams[156] = "A, R4";
  mcu->mnemonicParams[157] = "A, R5";
  mcu->mnemonicParams[158] = "A, R6";
  mcu->mnemonicParams[159] = "A, R7";
  mcu->mnemonicParams[160] = "C, /0%1";
  mcu->mnemonicParams[161] = "05N%1";
  mcu->mnemonicParams[162] = "C, 0%1";
  mcu->mnemonicParams[163] = "DPTR";
  mcu->mnemonicParams[164] = "AB";
  mcu->mnemonicParams[165] = "";
  mcu->mnemonicParams[166] = "@R0, %1";
  mcu->mnemonicParams[167] = "@R1, %1";
  mcu->mnemonicParams[168] = "R0, %1";
  mcu->mnemonicParams[169] = "R1, %1";
  mcu->mnemonicParams[170] = "R2, %1";
  mcu->mnemonicParams[171] = "R3, %1";
  mcu->mnemonicParams[172] = "R4, %1";
  mcu->mnemonicParams[173] = "R5, %1";
  mcu->mnemonicParams[174] = "R6, %1";
  mcu->mnemonicParams[175] = "R7, %1";
  mcu->mnemonicParams[176] = "C, /0%1";
  mcu->mnemonicParams[177] = "05N%1";
  mcu->mnemonicParams[178] = "0%1";
  mcu->mnemonicParams[179] = "C";
  mcu->mnemonicParams[180] = "A, #%1, O%2";
  mcu->mnemonicParams[181] = "A, %1, O%2";
  mcu->mnemonicParams[182] = "@R0, #%1, O%2";
  mcu->mnemonicParams[183] = "@R1, #%1, O%2";
  mcu->mnemonicParams[184] = "R0, #%1, O%2";
  mcu->mnemonicParams[185] = "R1, #%1, O%2";
  mcu->mnemonicParams[186] = "R2, #%1, O%2";
  mcu->mnemonicParams[187] = "R3, #%1, O%2";
  mcu->mnemonicParams[188] = "R4, #%1, O%2";
  mcu->mnemonicParams[189] = "R5, #%1, O%2";
  mcu->mnemonicParams[190] = "R6, #%1, O%2";
  mcu->mnemonicParams[191] = "R7, #%1, O%2";
  mcu->mnemonicParams[192] = "%1";
  mcu->mnemonicParams[193] = "06N%1";
  mcu->mnemonicParams[194] = "0%1";
  mcu->mnemonicParams[195] = "C";
  mcu->mnemonicParams[196] = "A";
  mcu->mnemonicParams[197] = "A, %1";
  mcu->mnemonicParams[198] = "A, @R0";
  mcu->mnemonicParams[199] = "A, @R1";
  mcu->mnemonicParams[200] = "A, R0";
  mcu->mnemonicParams[201] = "A, R1";
  mcu->mnemonicParams[202] = "A, R2";
  mcu->mnemonicParams[203] = "A, R3";
  mcu->mnemonicParams[204] = "A, R4";
  mcu->mnemonicParams[205] = "A, R5";
  mcu->mnemonicParams[206] = "A, R6";
  mcu->mnemonicParams[207] = "A, R7";
  mcu->mnemonicParams[208] = "%1";
  mcu->mnemonicParams[209] = "06N%1";
  mcu->mnemonicParams[210] = "0%1";
  mcu->mnemonicParams[211] = "C";
  mcu->mnemonicParams[212] = "A";
  mcu->mnemonicParams[213] = "%1, O%2";
  mcu->mnemonicParams[214] = "A, @R0";
  mcu->mnemonicParams[215] = "A, @R1";
  mcu->mnemonicParams[216] = "R0, O%1";
  mcu->mnemonicParams[217] = "R1, O%1";
  mcu->mnemonicParams[218] = "R2, O%1";
  mcu->mnemonicParams[219] = "R3, O%1";
  mcu->mnemonicParams[220] = "R4, O%1";
  mcu->mnemonicParams[221] = "R5, O%1";
  mcu->mnemonicParams[222] = "R6, O%1";
  mcu->mnemonicParams[223] = "R7, O%1";
  mcu->mnemonicParams[224] = "A, @DPTR";
  mcu->mnemonicParams[225] = "07N%1";
  mcu->mnemonicParams[226] = "A, @R0";
  mcu->mnemonicParams[227] = "A, @R1";
  mcu->mnemonicParams[228] = "A";
  mcu->mnemonicParams[229] = "A, %1";
  mcu->mnemonicParams[230] = "A, @R0";
  mcu->mnemonicParams[231] = "A, @R1";
  mcu->mnemonicParams[232] = "A, R0";
  mcu->mnemonicParams[233] = "A, R1";
  mcu->mnemonicParams[234] = "A, R2";
  mcu->mnemonicParams[235] = "A, R3";
  mcu->mnemonicParams[236] = "A, R4";
  mcu->mnemonicParams[237] = "A, R5";
  mcu->mnemonicParams[238] = "A, R6";
  mcu->mnemonicParams[239] = "A, R7";
  mcu->mnemonicParams[240] = "@DPTR, A";
  mcu->mnemonicParams[241] = "07N%1";
  mcu->mnemonicParams[242] = "@R0, A";
  mcu->mnemonicParams[243] = "@R1, A";
  mcu->mnemonicParams[244] = "A";
  mcu->mnemonicParams[245] = "%1, A";
  mcu->mnemonicParams[246] = "@R0, A";
  mcu->mnemonicParams[247] = "@R1, A";
  mcu->mnemonicParams[248] = "R0, A";
  mcu->mnemonicParams[249] = "R1, A";
  mcu->mnemonicParams[250] = "R2, A";
  mcu->mnemonicParams[251] = "R3, A";
  mcu->mnemonicParams[252] = "R4, A";
  mcu->mnemonicParams[253] = "R5, A";
  mcu->mnemonicParams[254] = "R6, A";
  mcu->mnemonicParams[255] = "R7, A";

  mcu->SFRNames[0] = "00h";
  mcu->SFRNames[1] = "01h";
  mcu->SFRNames[2] = "02h";
  mcu->SFRNames[3] = "03h";
  mcu->SFRNames[4] = "04h";
  mcu->SFRNames[5] = "05h";
  mcu->SFRNames[6] = "06h";
  mcu->SFRNames[7] = "07h";
  mcu->SFRNames[8] = "08h";
  mcu->SFRNames[9] = "09h";
  mcu->SFRNames[10] = "0Ah";
  mcu->SFRNames[11] = "0Bh";
  mcu->SFRNames[12] = "0Ch";
  mcu->SFRNames[13] = "0Dh";
  mcu->SFRNames[14] = "0Eh";
  mcu->SFRNames[15] = "0Fh";
  mcu->SFRNames[16] = "10h";
  mcu->SFRNames[17] = "11h";
  mcu->SFRNames[18] = "12h";
  mcu->SFRNames[19] = "13h";
  mcu->SFRNames[20] = "14h";
  mcu->SFRNames[21] = "15h";
  mcu->SFRNames[22] = "16h";
  mcu->SFRNames[23] = "17h";
  mcu->SFRNames[24] = "18h";
  mcu->SFRNames[25] = "19h";
  mcu->SFRNames[26] = "1Ah";
  mcu->SFRNames[27] = "1Bh";
  mcu->SFRNames[28] = "1Ch";
  mcu->SFRNames[29] = "1Dh";
  mcu->SFRNames[30] = "1Eh";
  mcu->SFRNames[31] = "1Fh";
  mcu->SFRNames[32] = "20h";
  mcu->SFRNames[33] = "21h";
  mcu->SFRNames[34] = "22h";
  mcu->SFRNames[35] = "23h";
  mcu->SFRNames[36] = "24h";
  mcu->SFRNames[37] = "25h";
  mcu->SFRNames[38] = "26h";
  mcu->SFRNames[39] = "27h";
  mcu->SFRNames[40] = "28h";
  mcu->SFRNames[41] = "29h";
  mcu->SFRNames[42] = "2Ah";
  mcu->SFRNames[43] = "2Bh";
  mcu->SFRNames[44] = "2Ch";
  mcu->SFRNames[45] = "2Dh";
  mcu->SFRNames[46] = "2Eh";
  mcu->SFRNames[47] = "2Fh";
  mcu->SFRNames[48] = "30h";
  mcu->SFRNames[49] = "31h";
  mcu->SFRNames[50] = "32h";
  mcu->SFRNames[51] = "33h";
  mcu->SFRNames[52] = "34h";
  mcu->SFRNames[53] = "35h";
  mcu->SFRNames[54] = "36h";
  mcu->SFRNames[55] = "37h";
  mcu->SFRNames[56] = "38h";
  mcu->SFRNames[57] = "39h";
  mcu->SFRNames[58] = "3Ah";
  mcu->SFRNames[59] = "3Bh";
  mcu->SFRNames[60] = "3Ch";
  mcu->SFRNames[61] = "3Dh";
  mcu->SFRNames[62] = "3Eh";
  mcu->SFRNames[63] = "3Fh";
  mcu->SFRNames[64] = "40h";
  mcu->SFRNames[65] = "41h";
  mcu->SFRNames[66] = "42h";
  mcu->SFRNames[67] = "43h";
  mcu->SFRNames[68] = "44h";
  mcu->SFRNames[69] = "45h";
  mcu->SFRNames[70] = "46h";
  mcu->SFRNames[71] = "47h";
  mcu->SFRNames[72] = "48h";
  mcu->SFRNames[73] = "49h";
  mcu->SFRNames[74] = "4Ah";
  mcu->SFRNames[75] = "4Bh";
  mcu->SFRNames[76] = "4Ch";
  mcu->SFRNames[77] = "4Dh";
  mcu->SFRNames[78] = "4Eh";
  mcu->SFRNames[79] = "4Fh";
  mcu->SFRNames[80] = "50h";
  mcu->SFRNames[81] = "51h";
  mcu->SFRNames[82] = "52h";
  mcu->SFRNames[83] = "53h";
  mcu->SFRNames[84] = "54h";
  mcu->SFRNames[85] = "55h";
  mcu->SFRNames[86] = "56h";
  mcu->SFRNames[87] = "57h";
  mcu->SFRNames[88] = "58h";
  mcu->SFRNames[89] = "59h";
  mcu->SFRNames[90] = "5Ah";
  mcu->SFRNames[91] = "5Bh";
  mcu->SFRNames[92] = "5Ch";
  mcu->SFRNames[93] = "5Dh";
  mcu->SFRNames[94] = "5Eh";
  mcu->SFRNames[95] = "5Fh";
  mcu->SFRNames[96] = "60h";
  mcu->SFRNames[97] = "61h";
  mcu->SFRNames[98] = "62h";
  mcu->SFRNames[99] = "63h";
  mcu->SFRNames[100] = "64h";
  mcu->SFRNames[101] = "65h";
  mcu->SFRNames[102] = "66h";
  mcu->SFRNames[103] = "67h";
  mcu->SFRNames[104] = "68h";
  mcu->SFRNames[105] = "69h";
  mcu->SFRNames[106] = "6Ah";
  mcu->SFRNames[107] = "6Bh";
  mcu->SFRNames[108] = "6Ch";
  mcu->SFRNames[109] = "6Dh";
  mcu->SFRNames[110] = "6Eh";
  mcu->SFRNames[111] = "6Fh";
  mcu->SFRNames[112] = "70h";
  mcu->SFRNames[113] = "71h";
  mcu->SFRNames[114] = "72h";
  mcu->SFRNames[115] = "73h";
  mcu->SFRNames[116] = "74h";
  mcu->SFRNames[117] = "75h";
  mcu->SFRNames[118] = "76h";
  mcu->SFRNames[119] = "77h";
  mcu->SFRNames[120] = "78h";
  mcu->SFRNames[121] = "79h";
  mcu->SFRNames[122] = "7Ah";
  mcu->SFRNames[123] = "7Bh";
  mcu->SFRNames[124] = "7Ch";
  mcu->SFRNames[125] = "7Dh";
  mcu->SFRNames[126] = "7Eh";
  mcu->SFRNames[127] = "7Fh";
  mcu->SFRNames[128] = "P0";
  mcu->SFRNames[129] = "SP";
  mcu->SFRNames[130] = "DPL";
  mcu->SFRNames[131] = "DPh";
  mcu->SFRNames[132] = "84h";
  mcu->SFRNames[133] = "85h";
  mcu->SFRNames[134] = "86h";
  mcu->SFRNames[135] = "PCON";
  mcu->SFRNames[136] = "TCON";
  mcu->SFRNames[137] = "TMOD";
  mcu->SFRNames[138] = "TL0";
  mcu->SFRNames[139] = "TL1";
  mcu->SFRNames[140] = "Th0";
  mcu->SFRNames[141] = "Th1";
  mcu->SFRNames[142] = "8Eh";
  mcu->SFRNames[143] = "8Fh";
  mcu->SFRNames[144] = "P1";
  mcu->SFRNames[145] = "91h";
  mcu->SFRNames[146] = "92h";
  mcu->SFRNames[147] = "93h";
  mcu->SFRNames[148] = "94h";
  mcu->SFRNames[149] = "95h";
  mcu->SFRNames[150] = "96h";
  mcu->SFRNames[151] = "97h";
  mcu->SFRNames[152] = "SCON";
  mcu->SFRNames[153] = "SBUF";
  mcu->SFRNames[154] = "9Ah";
  mcu->SFRNames[155] = "9Bh";
  mcu->SFRNames[156] = "9Ch";
  mcu->SFRNames[157] = "9Dh";
  mcu->SFRNames[158] = "9Eh";
  mcu->SFRNames[159] = "9Fh";
  mcu->SFRNames[160] = "P2";
  mcu->SFRNames[161] = "A1h";
  mcu->SFRNames[162] = "A2h";
  mcu->SFRNames[163] = "A3h";
  mcu->SFRNames[164] = "A4h";
  mcu->SFRNames[165] = "A5h";
  mcu->SFRNames[166] = "A6h";
  mcu->SFRNames[167] = "A7h";
  mcu->SFRNames[168] = "IE";
  mcu->SFRNames[169] = "A9h";
  mcu->SFRNames[170] = "AAh";
  mcu->SFRNames[171] = "ABh";
  mcu->SFRNames[172] = "ACh";
  mcu->SFRNames[173] = "ADh";
  mcu->SFRNames[174] = "AEh";
  mcu->SFRNames[175] = "AFh";
  mcu->SFRNames[176] = "P3";
  mcu->SFRNames[177] = "B1h";
  mcu->SFRNames[178] = "B2h";
  mcu->SFRNames[179] = "B3h";
  mcu->SFRNames[180] = "B4h";
  mcu->SFRNames[181] = "B5h";
  mcu->SFRNames[182] = "B6h";
  mcu->SFRNames[183] = "B7h";
  mcu->SFRNames[184] = "IP";
  mcu->SFRNames[185] = "B9h";
  mcu->SFRNames[186] = "BAh";
  mcu->SFRNames[187] = "BBh";
  mcu->SFRNames[188] = "BCh";
  mcu->SFRNames[189] = "BDh";
  mcu->SFRNames[190] = "BEh";
  mcu->SFRNames[191] = "BFh";
  mcu->SFRNames[192] = "C0h";
  mcu->SFRNames[193] = "C1h";
  mcu->SFRNames[194] = "C2h";
  mcu->SFRNames[195] = "C3h";
  mcu->SFRNames[196] = "C4h";
  mcu->SFRNames[197] = "C5h";
  mcu->SFRNames[198] = "C6h";
  mcu->SFRNames[199] = "C7h";
  mcu->SFRNames[200] = "C8h";
  mcu->SFRNames[201] = "C9h";
  mcu->SFRNames[202] = "CAh";
  mcu->SFRNames[203] = "CBh";
  mcu->SFRNames[204] = "CCh";
  mcu->SFRNames[205] = "CDh";
  mcu->SFRNames[206] = "CEh";
  mcu->SFRNames[207] = "CFh";
  mcu->SFRNames[208] = "PSW";
  mcu->SFRNames[209] = "D1h";
  mcu->SFRNames[210] = "D2h";
  mcu->SFRNames[211] = "D3h";
  mcu->SFRNames[212] = "D4h";
  mcu->SFRNames[213] = "D5h";
  mcu->SFRNames[214] = "D6h";
  mcu->SFRNames[215] = "D7h";
  mcu->SFRNames[216] = "D8h";
  mcu->SFRNames[217] = "D9h";
  mcu->SFRNames[218] = "DAh";
  mcu->SFRNames[219] = "DBh";
  mcu->SFRNames[220] = "DCh";
  mcu->SFRNames[221] = "DDh";
  mcu->SFRNames[222] = "DEh";
  mcu->SFRNames[223] = "DFh";
  mcu->SFRNames[224] = "ACC";
  mcu->SFRNames[225] = "E1h";
  mcu->SFRNames[226] = "E2h";
  mcu->SFRNames[227] = "E3h";
  mcu->SFRNames[228] = "E4h";
  mcu->SFRNames[229] = "E5h";
  mcu->SFRNames[230] = "E6h";
  mcu->SFRNames[231] = "E7h";
  mcu->SFRNames[232] = "E8h";
  mcu->SFRNames[233] = "E9h";
  mcu->SFRNames[234] = "EAh";
  mcu->SFRNames[235] = "EBh";
  mcu->SFRNames[236] = "ECh";
  mcu->SFRNames[237] = "EDh";
  mcu->SFRNames[238] = "EEh";
  mcu->SFRNames[239] = "EFh";
  mcu->SFRNames[240] = "B";
  mcu->SFRNames[241] = "F1h";
  mcu->SFRNames[242] = "F2h";
  mcu->SFRNames[243] = "F3h";
  mcu->SFRNames[244] = "F4h";
  mcu->SFRNames[245] = "F5h";
  mcu->SFRNames[246] = "F6h";
  mcu->SFRNames[247] = "F7h";
  mcu->SFRNames[248] = "F8h";
  mcu->SFRNames[249] = "F9h";
  mcu->SFRNames[250] = "FAh";
  mcu->SFRNames[251] = "FBh";
  mcu->SFRNames[252] = "FCh";
  mcu->SFRNames[253] = "FDh";
  mcu->SFRNames[254] = "FEh";
  mcu->SFRNames[255] = "FFh";

  mcu->SFRBits[0] = "20h.1";
  mcu->SFRBits[1] = "20h.1";
  mcu->SFRBits[2] = "20h.2";
  mcu->SFRBits[3] = "20h.3";
  mcu->SFRBits[4] = "20h.4";
  mcu->SFRBits[5] = "20h.5";
  mcu->SFRBits[6] = "20h.6";
  mcu->SFRBits[7] = "20h.7";
  mcu->SFRBits[8] = "21h.0";
  mcu->SFRBits[9] = "21h.1";
  mcu->SFRBits[10] = "21h.2";
  mcu->SFRBits[11] = "21h.3";
  mcu->SFRBits[12] = "21h.4";
  mcu->SFRBits[13] = "21h.5";
  mcu->SFRBits[14] = "21h.6";
  mcu->SFRBits[15] = "21h.7";
  mcu->SFRBits[16] = "22h.0";
  mcu->SFRBits[17] = "22h.1";
  mcu->SFRBits[18] = "22h.2";
  mcu->SFRBits[19] = "22h.3";
  mcu->SFRBits[20] = "22h.4";
  mcu->SFRBits[21] = "22h.5";
  mcu->SFRBits[22] = "22h.6";
  mcu->SFRBits[23] = "22h.7";
  mcu->SFRBits[24] = "23h.0";
  mcu->SFRBits[25] = "23h.1";
  mcu->SFRBits[26] = "23h.2";
  mcu->SFRBits[27] = "23h.3";
  mcu->SFRBits[28] = "23h.4";
  mcu->SFRBits[29] = "23h.5";
  mcu->SFRBits[30] = "23h.6";
  mcu->SFRBits[31] = "23h.7";
  mcu->SFRBits[32] = "24h.0";
  mcu->SFRBits[33] = "24h.1";
  mcu->SFRBits[34] = "24h.2";
  mcu->SFRBits[35] = "24h.3";
  mcu->SFRBits[36] = "24h.4";
  mcu->SFRBits[37] = "24h.5";
  mcu->SFRBits[38] = "24h.6";
  mcu->SFRBits[39] = "24h.7";
  mcu->SFRBits[40] = "25h.0";
  mcu->SFRBits[41] = "25h.1";
  mcu->SFRBits[42] = "25h.2";
  mcu->SFRBits[43] = "25h.3";
  mcu->SFRBits[44] = "25h.4";
  mcu->SFRBits[45] = "25h.5";
  mcu->SFRBits[46] = "25h.6";
  mcu->SFRBits[47] = "25h.7";
  mcu->SFRBits[48] = "26h.0";
  mcu->SFRBits[49] = "26h.1";
  mcu->SFRBits[50] = "26h.2";
  mcu->SFRBits[51] = "26h.3";
  mcu->SFRBits[52] = "26h.4";
  mcu->SFRBits[53] = "26h.5";
  mcu->SFRBits[54] = "26h.6";
  mcu->SFRBits[55] = "26h.7";
  mcu->SFRBits[56] = "27h.0";
  mcu->SFRBits[57] = "27h.1";
  mcu->SFRBits[58] = "27h.2";
  mcu->SFRBits[59] = "27h.3";
  mcu->SFRBits[60] = "27h.4";
  mcu->SFRBits[61] = "27h.5";
  mcu->SFRBits[62] = "27h.6";
  mcu->SFRBits[63] = "27h.7";
  mcu->SFRBits[64] = "28h.0";
  mcu->SFRBits[65] = "28h.1";
  mcu->SFRBits[66] = "28h.2";
  mcu->SFRBits[67] = "28h.3";
  mcu->SFRBits[68] = "28h.4";
  mcu->SFRBits[69] = "28h.5";
  mcu->SFRBits[70] = "28h.6";
  mcu->SFRBits[71] = "28h.7";
  mcu->SFRBits[72] = "29h.0";
  mcu->SFRBits[73] = "29h.1";
  mcu->SFRBits[74] = "29h.2";
  mcu->SFRBits[75] = "29h.3";
  mcu->SFRBits[76] = "29h.4";
  mcu->SFRBits[77] = "29h.5";
  mcu->SFRBits[78] = "29h.6";
  mcu->SFRBits[79] = "29h.7";
  mcu->SFRBits[80] = "2Ah.0";
  mcu->SFRBits[81] = "2Ah.1";
  mcu->SFRBits[82] = "2Ah.2";
  mcu->SFRBits[83] = "2Ah.3";
  mcu->SFRBits[84] = "2Ah.4";
  mcu->SFRBits[85] = "2Ah.5";
  mcu->SFRBits[86] = "2Ah.6";
  mcu->SFRBits[87] = "2Ah.7";
  mcu->SFRBits[88] = "2Bh.0";
  mcu->SFRBits[89] = "2Bh.1";
  mcu->SFRBits[90] = "2Bh.2";
  mcu->SFRBits[91] = "2Bh.3";
  mcu->SFRBits[92] = "2Bh.4";
  mcu->SFRBits[93] = "2Bh.5";
  mcu->SFRBits[94] = "2Bh.6";
  mcu->SFRBits[95] = "2Bh.7";
  mcu->SFRBits[96] = "2Ch.0";
  mcu->SFRBits[97] = "2Ch.1";
  mcu->SFRBits[98] = "2Ch.2";
  mcu->SFRBits[99] = "2Ch.3";
  mcu->SFRBits[100] = "2Ch.4";
  mcu->SFRBits[101] = "2Ch.5";
  mcu->SFRBits[102] = "2Ch.6";
  mcu->SFRBits[103] = "2Ch.7";
  mcu->SFRBits[104] = "2Dh.0";
  mcu->SFRBits[105] = "2Dh.1";
  mcu->SFRBits[106] = "2Dh.2";
  mcu->SFRBits[107] = "2Dh.3";
  mcu->SFRBits[108] = "2Dh.4";
  mcu->SFRBits[109] = "2Dh.5";
  mcu->SFRBits[110] = "2Dh.6";
  mcu->SFRBits[111] = "2Dh.7";
  mcu->SFRBits[112] = "2Eh.0";
  mcu->SFRBits[113] = "2Eh.1";
  mcu->SFRBits[114] = "2Eh.2";
  mcu->SFRBits[115] = "2Eh.3";
  mcu->SFRBits[116] = "2Eh.4";
  mcu->SFRBits[117] = "2Eh.5";
  mcu->SFRBits[118] = "2Eh.6";
  mcu->SFRBits[119] = "2Eh.7";
  mcu->SFRBits[120] = "2Fh.0";
  mcu->SFRBits[121] = "2Fh.1";
  mcu->SFRBits[122] = "2Fh.2";
  mcu->SFRBits[123] = "2Fh.3";
  mcu->SFRBits[124] = "2Fh.4";
  mcu->SFRBits[125] = "2Fh.5";
  mcu->SFRBits[126] = "2Fh.6";
  mcu->SFRBits[127] = "2Fh.7";
  mcu->SFRBits[128] = "P0.0";
  mcu->SFRBits[129] = "P0.1";
  mcu->SFRBits[130] = "P0.2";
  mcu->SFRBits[131] = "P0.3";
  mcu->SFRBits[132] = "P0.4";
  mcu->SFRBits[133] = "P0.5";
  mcu->SFRBits[134] = "P0.6";
  mcu->SFRBits[135] = "P0.7";
  mcu->SFRBits[136] = "IT0";
  mcu->SFRBits[137] = "IE0";
  mcu->SFRBits[138] = "IT1";
  mcu->SFRBits[139] = "IE1";
  mcu->SFRBits[140] = "TR0";
  mcu->SFRBits[141] = "TF0";
  mcu->SFRBits[142] = "TR1";
  mcu->SFRBits[143] = "TF1";
  mcu->SFRBits[144] = "P1.0";
  mcu->SFRBits[145] = "P1.1";
  mcu->SFRBits[146] = "P1.2";
  mcu->SFRBits[147] = "P1.3";
  mcu->SFRBits[148] = "P1.4";
  mcu->SFRBits[149] = "P1.5";
  mcu->SFRBits[150] = "P1.6";
  mcu->SFRBits[151] = "P1.7";
  mcu->SFRBits[152] = "RI";
  mcu->SFRBits[153] = "TI";
  mcu->SFRBits[154] = "RB8";
  mcu->SFRBits[155] = "TB8";
  mcu->SFRBits[156] = "REN";
  mcu->SFRBits[157] = "SM2";
  mcu->SFRBits[158] = "SM1";
  mcu->SFRBits[159] = "SM0";
  mcu->SFRBits[160] = "P2.0";
  mcu->SFRBits[161] = "P2.1";
  mcu->SFRBits[162] = "P2.2";
  mcu->SFRBits[163] = "P2.3";
  mcu->SFRBits[164] = "P2.4";
  mcu->SFRBits[165] = "P2.5";
  mcu->SFRBits[166] = "P2.6";
  mcu->SFRBits[167] = "P2.7";
  mcu->SFRBits[168] = "EX0";
  mcu->SFRBits[169] = "ET0";
  mcu->SFRBits[170] = "EX1";
  mcu->SFRBits[171] = "ET1";
  mcu->SFRBits[172] = "ES";
  mcu->SFRBits[173] = "IE.5";
  mcu->SFRBits[174] = "IE.6";
  mcu->SFRBits[175] = "EA";
  mcu->SFRBits[176] = "RXD";
  mcu->SFRBits[177] = "TXD";
  mcu->SFRBits[178] = "INT0";
  mcu->SFRBits[179] = "INT1";
  mcu->SFRBits[180] = "T0";
  mcu->SFRBits[181] = "T1";
  mcu->SFRBits[182] = "WR";
  mcu->SFRBits[183] = "RD";
  mcu->SFRBits[184] = "PX0";
  mcu->SFRBits[185] = "PT0";
  mcu->SFRBits[186] = "PX1";
  mcu->SFRBits[187] = "PT1";
  mcu->SFRBits[188] = "PS";
  mcu->SFRBits[189] = "IP.5";
  mcu->SFRBits[190] = "IP.6";
  mcu->SFRBits[191] = "IP.7";
  mcu->SFRBits[192] = "C0h.0";
  mcu->SFRBits[193] = "C0h.1";
  mcu->SFRBits[194] = "C0h.2";
  mcu->SFRBits[195] = "C0h.3";
  mcu->SFRBits[196] = "C0h.4";
  mcu->SFRBits[197] = "C0h.5";
  mcu->SFRBits[198] = "C0h.6";
  mcu->SFRBits[199] = "C0h.7";
  mcu->SFRBits[200] = "C8h.0";
  mcu->SFRBits[201] = "C8h.1";
  mcu->SFRBits[202] = "C8h.2";
  mcu->SFRBits[203] = "C8h.3";
  mcu->SFRBits[204] = "C8h.4";
  mcu->SFRBits[205] = "C8h.5";
  mcu->SFRBits[206] = "C8h.6";
  mcu->SFRBits[207] = "C8h.7";
  mcu->SFRBits[208] = "P";
  mcu->SFRBits[209] = "PSW.1";
  mcu->SFRBits[210] = "OV";
  mcu->SFRBits[211] = "RS0";
  mcu->SFRBits[212] = "RS1";
  mcu->SFRBits[213] = "F0";
  mcu->SFRBits[214] = "AC";
  mcu->SFRBits[215] = "CY";
  mcu->SFRBits[216] = "D8h.0";
  mcu->SFRBits[217] = "D8h.1";
  mcu->SFRBits[218] = "D8h.2";
  mcu->SFRBits[219] = "D8h.3";
  mcu->SFRBits[220] = "D8h.4";
  mcu->SFRBits[221] = "D8h.5";
  mcu->SFRBits[222] = "D8h.6";
  mcu->SFRBits[223] = "D8h.7";
  mcu->SFRBits[224] = "ACC.0";
  mcu->SFRBits[225] = "ACC.1";
  mcu->SFRBits[226] = "ACC.2";
  mcu->SFRBits[227] = "ACC.3";
  mcu->SFRBits[228] = "ACC.4";
  mcu->SFRBits[229] = "ACC.5";
  mcu->SFRBits[230] = "ACC.6";
  mcu->SFRBits[231] = "ACC.7";
  mcu->SFRBits[232] = "E8h.0";
  mcu->SFRBits[233] = "E8h.1";
  mcu->SFRBits[234] = "E8h.2";
  mcu->SFRBits[235] = "E8h.3";
  mcu->SFRBits[236] = "E8h.4";
  mcu->SFRBits[237] = "E8h.5";
  mcu->SFRBits[238] = "E8h.6";
  mcu->SFRBits[239] = "E8h.7";
  mcu->SFRBits[240] = "B.0";
  mcu->SFRBits[241] = "B.1";
  mcu->SFRBits[242] = "B.2";
  mcu->SFRBits[243] = "B.3";
  mcu->SFRBits[244] = "B.4";
  mcu->SFRBits[245] = "B.5";
  mcu->SFRBits[246] = "B.6";
  mcu->SFRBits[247] = "B.7";
  mcu->SFRBits[248] = "F8h.0";
  mcu->SFRBits[249] = "F8h.1";
  mcu->SFRBits[250] = "F8h.2";
  mcu->SFRBits[251] = "F8h.3";
  mcu->SFRBits[252] = "F8h.4";
  mcu->SFRBits[253] = "F8h.5";
  mcu->SFRBits[254] = "F8h.6";
  mcu->SFRBits[255] = "F8h.7";

  mcu->_instructions[0x00] = &nop_00;
  mcu->_instructions[0xA5] = NULL;
  mcu->_instructions[0x74] = &mov_74;
  mcu->_instructions[0x75] = &mov_75;
  mcu->_instructions[0x02] = &ljmp_02;
  mcu->_instructions[0x80] = &sjmp_80;
  mcu->_instructions[0x85] = &mov_85;
  mcu->_instructions[0xE5] = &mov_E5;
  mcu->_instructions[0x92] = &mov_92;
  mcu->_instructions[0x04] = &inc_04;
  mcu->_instructions[0x05] = &inc_05;
  mcu->_instructions[0xA3] = &inc_A3;
  mcu->_instructions[0x06] = &inc_06;
  mcu->_instructions[0x07] = &inc_07;
  mcu->_instructions[0x08] = &inc_08;
  mcu->_instructions[0x09] = &inc_09;
  mcu->_instructions[0x0A] = &inc_0A;
  mcu->_instructions[0x0B] = &inc_0B;
  mcu->_instructions[0x0C] = &inc_0C;
  mcu->_instructions[0x0D] = &inc_0D;
  mcu->_instructions[0x0E] = &inc_0E;
  mcu->_instructions[0x0F] = &inc_0F;
  mcu->_instructions[0x14] = &dec_14;
  mcu->_instructions[0x15] = &dec_15;
  mcu->_instructions[0x16] = &dec_16;
  mcu->_instructions[0x17] = &dec_17;
  mcu->_instructions[0x18] = &dec_18;
  mcu->_instructions[0x19] = &dec_19;
  mcu->_instructions[0x1A] = &dec_1A;
  mcu->_instructions[0x1B] = &dec_1B;
  mcu->_instructions[0x1C] = &dec_1C;
  mcu->_instructions[0x1D] = &dec_1D;
  mcu->_instructions[0x1E] = &dec_1E;
  mcu->_instructions[0x1F] = &dec_1F;
  mcu->_instructions[0x84] = &div_84;
  mcu->_instructions[0xA4] = &mul_A4;
  mcu->_instructions[0x23] = &rl_23;
  mcu->_instructions[0x03] = &rr_03;
  mcu->_instructions[0x33] = &rlc_33;
  mcu->_instructions[0x13] = &rrc_13;
  mcu->_instructions[0x90] = &mov_90;
  mcu->_instructions[0xF5] = &mov_F5;
  mcu->_instructions[0x24] = &add_24;
  mcu->_instructions[0x25] = &add_25;
  mcu->_instructions[0x26] = &add_26;
  mcu->_instructions[0x27] = &add_27;
  mcu->_instructions[0x28] = &add_28;
  mcu->_instructions[0x29] = &add_29;
  mcu->_instructions[0x2A] = &add_2A;
  mcu->_instructions[0x2B] = &add_2B;
  mcu->_instructions[0x2C] = &add_2C;
  mcu->_instructions[0x2D] = &add_2D;
  mcu->_instructions[0x2E] = &add_2E;
  mcu->_instructions[0x2F] = &add_2F;
  mcu->_instructions[0x34] = &addc_34;
  mcu->_instructions[0x35] = &addc_35;
  mcu->_instructions[0x36] = &addc_36;
  mcu->_instructions[0x37] = &addc_37;
  mcu->_instructions[0x38] = &addc_38;
  mcu->_instructions[0x39] = &addc_39;
  mcu->_instructions[0x3A] = &addc_3A;
  mcu->_instructions[0x3B] = &addc_3B;
  mcu->_instructions[0x3C] = &addc_3C;
  mcu->_instructions[0x3D] = &addc_3D;
  mcu->_instructions[0x3E] = &addc_3E;
  mcu->_instructions[0x3F] = &addc_3F;
  mcu->_instructions[0xE0] = &movx_E0;
  mcu->_instructions[0xF0] = &movx_F0;
  mcu->_instructions[0xC0] = &push_C0;
  mcu->_instructions[0xD0] = &pop_D0;
  mcu->_instructions[0xC3] = &clr_C3;
  mcu->_instructions[0xD3] = &setb_D3;
  mcu->_instructions[0xD2] = &setb_D2;
  mcu->_instructions[0xC2] = &clr_C2;
  mcu->_instructions[0x62] = &xrl_62;
  mcu->_instructions[0x63] = &xrl_63;
  mcu->_instructions[0x64] = &xrl_64;
  mcu->_instructions[0x65] = &xrl_65;
  mcu->_instructions[0x66] = &xrl_66;
  mcu->_instructions[0x67] = &xrl_67;
  mcu->_instructions[0x68] = &xrl_68;
  mcu->_instructions[0x69] = &xrl_69;
  mcu->_instructions[0x6A] = &xrl_6A;
  mcu->_instructions[0x6B] = &xrl_6B;
  mcu->_instructions[0x6C] = &xrl_6C;
  mcu->_instructions[0x6D] = &xrl_6D;
  mcu->_instructions[0x6E] = &xrl_6E;
  mcu->_instructions[0x6F] = &xrl_6F;
  mcu->_instructions[0xC5] = &xch_C5;
  mcu->_instructions[0xC6] = &xch_C6;
  mcu->_instructions[0xC7] = &xch_C7;
  mcu->_instructions[0xC8] = &xch_C8;
  mcu->_instructions[0xC9] = &xch_C9;
  mcu->_instructions[0xCA] = &xch_CA;
  mcu->_instructions[0xCB] = &xch_CB;
  mcu->_instructions[0xCC] = &xch_CC;
  mcu->_instructions[0xCD] = &xch_CD;
  mcu->_instructions[0xCE] = &xch_CE;
  mcu->_instructions[0xCF] = &xch_CF;
  mcu->_instructions[0x12] = &lcall_12;
  mcu->_instructions[0x22] = &ret_22;
  mcu->_instructions[0x70] = &jnz_70;
  mcu->_instructions[0x60] = &jz_60;
  mcu->_instructions[0x40] = &jc_40;
  mcu->_instructions[0x50] = &jnc_50;
  mcu->_instructions[0xC4] = &swap_C4;
  mcu->_instructions[0x93] = &movc_93;
  mcu->_instructions[0x83] = &movc_83;
  mcu->_instructions[0xE6] = &mov_E6;
  mcu->_instructions[0xE7] = &mov_E7;
  mcu->_instructions[0xF6] = &mov_F6;
  mcu->_instructions[0xF7] = &mov_F7;
  mcu->_instructions[0x78] = &mov_78;
  mcu->_instructions[0x79] = &mov_79;
  mcu->_instructions[0x7A] = &mov_7A;
  mcu->_instructions[0x7B] = &mov_7B;
  mcu->_instructions[0x7C] = &mov_7C;
  mcu->_instructions[0x7D] = &mov_7D;
  mcu->_instructions[0x7E] = &mov_7E;
  mcu->_instructions[0x7F] = &mov_7F;
  mcu->_instructions[0xA8] = &mov_A8;
  mcu->_instructions[0xA9] = &mov_A9;
  mcu->_instructions[0xAA] = &mov_AA;
  mcu->_instructions[0xAB] = &mov_AB;
  mcu->_instructions[0xAC] = &mov_AC;
  mcu->_instructions[0xAD] = &mov_AD;
  mcu->_instructions[0xAE] = &mov_AE;
  mcu->_instructions[0xAF] = &mov_AF;
  mcu->_instructions[0x88] = &mov_88;
  mcu->_instructions[0x89] = &mov_89;
  mcu->_instructions[0x8A] = &mov_8A;
  mcu->_instructions[0x8B] = &mov_8B;
  mcu->_instructions[0x8C] = &mov_8C;
  mcu->_instructions[0x8D] = &mov_8D;
  mcu->_instructions[0x8E] = &mov_8E;
  mcu->_instructions[0x8F] = &mov_8F;
  mcu->_instructions[0xF8] = &mov_F8;
  mcu->_instructions[0xF9] = &mov_F9;
  mcu->_instructions[0xFA] = &mov_FA;
  mcu->_instructions[0xFB] = &mov_FB;
  mcu->_instructions[0xFC] = &mov_FC;
  mcu->_instructions[0xFD] = &mov_FD;
  mcu->_instructions[0xFE] = &mov_FE;
  mcu->_instructions[0xFF] = &mov_FF;
  mcu->_instructions[0xE8] = &mov_E8;
  mcu->_instructions[0xE9] = &mov_E9;
  mcu->_instructions[0xEA] = &mov_EA;
  mcu->_instructions[0xEB] = &mov_EB;
  mcu->_instructions[0xEC] = &mov_EC;
  mcu->_instructions[0xED] = &mov_ED;
  mcu->_instructions[0xEE] = &mov_EE;
  mcu->_instructions[0xEF] = &mov_EF;
  mcu->_instructions[0xE4] = &clr_E4;
  mcu->_instructions[0x01] = &ajmp_01;
  mcu->_instructions[0x21] = &ajmp_21;
  mcu->_instructions[0x41] = &ajmp_41;
  mcu->_instructions[0x61] = &ajmp_61;
  mcu->_instructions[0x81] = &ajmp_81;
  mcu->_instructions[0xA1] = &ajmp_A1;
  mcu->_instructions[0xC1] = &ajmp_C1;
  mcu->_instructions[0xE1] = &ajmp_E1;
  mcu->_instructions[0x11] = &acall_11;
  mcu->_instructions[0x31] = &acall_31;
  mcu->_instructions[0x51] = &acall_51;
  mcu->_instructions[0x71] = &acall_71;
  mcu->_instructions[0x91] = &acall_91;
  mcu->_instructions[0xB1] = &acall_B1;
  mcu->_instructions[0xD1] = &acall_D1;
  mcu->_instructions[0xF1] = &acall_F1;
  mcu->_instructions[0xD8] = &djnz_D8;
  mcu->_instructions[0xD9] = &djnz_D9;
  mcu->_instructions[0xDA] = &djnz_DA;
  mcu->_instructions[0xDB] = &djnz_DB;
  mcu->_instructions[0xDC] = &djnz_DC;
  mcu->_instructions[0xDD] = &djnz_DD;
  mcu->_instructions[0xDE] = &djnz_DE;
  mcu->_instructions[0xDF] = &djnz_DF;
  mcu->_instructions[0xD5] = &djnz_D5;
  mcu->_instructions[0x43] = &orl_43;
  mcu->_instructions[0x44] = &orl_44;
  mcu->_instructions[0x45] = &orl_45;
  mcu->_instructions[0x42] = &orl_42;
  mcu->_instructions[0x48] = &orl_48;
  mcu->_instructions[0x49] = &orl_49;
  mcu->_instructions[0x4A] = &orl_4A;
  mcu->_instructions[0x4B] = &orl_4B;
  mcu->_instructions[0x4C] = &orl_4C;
  mcu->_instructions[0x4D] = &orl_4D;
  mcu->_instructions[0x4E] = &orl_4E;
  mcu->_instructions[0x4F] = &orl_4F;
  mcu->_instructions[0x46] = &orl_46;
  mcu->_instructions[0x47] = &orl_47;
  mcu->_instructions[0x54] = &anl_54;
  mcu->_instructions[0x55] = &anl_55;
  mcu->_instructions[0x52] = &anl_52;
  mcu->_instructions[0x53] = &anl_53;
  mcu->_instructions[0x58] = &anl_58;
  mcu->_instructions[0x59] = &anl_59;
  mcu->_instructions[0x5A] = &anl_5A;
  mcu->_instructions[0x5B] = &anl_5B;
  mcu->_instructions[0x5C] = &anl_5C;
  mcu->_instructions[0x5D] = &anl_5D;
  mcu->_instructions[0x5E] = &anl_5E;
  mcu->_instructions[0x5F] = &anl_5F;
  mcu->_instructions[0x56] = &anl_56;
  mcu->_instructions[0x57] = &anl_57;
  mcu->_instructions[0xA2] = &mov_A2;
  mcu->_instructions[0xB2] = &cpl_B2;
  mcu->_instructions[0xB3] = &cpl_B3;
  mcu->_instructions[0x72] = &orl_72;
  mcu->_instructions[0xA0] = &orl_A0;
  mcu->_instructions[0x82] = &anl_82;
  mcu->_instructions[0xB0] = &anl_B0;
  mcu->_instructions[0xF4] = &cpl_F4;
  mcu->_instructions[0xA6] = &mov_A6;
  mcu->_instructions[0xA7] = &mov_A7;
  mcu->_instructions[0x76] = &mov_76;
  mcu->_instructions[0x77] = &mov_77;
  mcu->_instructions[0x86] = &mov_86;
  mcu->_instructions[0x87] = &mov_87;
  mcu->_instructions[0x73] = &jmp_73;
  mcu->_instructions[0xB4] = &cjne_B4;
  mcu->_instructions[0xB5] = &cjne_B5;
  mcu->_instructions[0xB6] = &cjne_B6;
  mcu->_instructions[0xB7] = &cjne_B7;
  mcu->_instructions[0xB8] = &cjne_B8;
  mcu->_instructions[0xB9] = &cjne_B9;
  mcu->_instructions[0xBA] = &cjne_BA;
  mcu->_instructions[0xBB] = &cjne_BB;
  mcu->_instructions[0xBC] = &cjne_BC;
  mcu->_instructions[0xBD] = &cjne_BD;
  mcu->_instructions[0xBE] = &cjne_BE;
  mcu->_instructions[0xBF] = &cjne_BF;
  mcu->_instructions[0x32] = &reti_32;
  mcu->_instructions[0x30] = &jnb_30;
  mcu->_instructions[0x20] = &jb_20;
  mcu->_instructions[0x10] = &jbc_10;
  mcu->_instructions[0x94] = &subb_94;
  mcu->_instructions[0x95] = &subb_95;
  mcu->_instructions[0x96] = &subb_96;
  mcu->_instructions[0x97] = &subb_97;
  mcu->_instructions[0x98] = &subb_98;
  mcu->_instructions[0x99] = &subb_99;
  mcu->_instructions[0x9A] = &subb_9A;
  mcu->_instructions[0x9B] = &subb_9B;
  mcu->_instructions[0x9C] = &subb_9C;
  mcu->_instructions[0x9D] = &subb_9D;
  mcu->_instructions[0x9E] = &subb_9E;
  mcu->_instructions[0x9F] = &subb_9F;
  mcu->_instructions[0xD4] = &da_D4;
  mcu->_instructions[0xD6] = &xchd_D6;
  mcu->_instructions[0xD7] = &xchd_D7;
  mcu->_instructions[0xE2] = &movx_E2;
  mcu->_instructions[0xE3] = &movx_E3;
  mcu->_instructions[0xF2] = &movx_F2;
  mcu->_instructions[0xF3] = &movx_F3;

  mcu->_timers = &timers8051;
  mcu->_interrupts = &interrupts8051;
  mcu->_additionalCode = NULL;

  resetMCU(mcu);
}

void init8031MCU(MCU* mcu)
{
  init8051MCU(mcu);

  mcu->idataMemorySize = 0x100;
  mcu->xdataMemorySize = MAX_EXT_RAM_SIZE;
  mcu->iromMemorySize = 0;
  mcu->xromMemorySize = MAX_ROM_SIZE;
}

void init8052MCU(MCU* mcu)
{
  init8051MCU(mcu);

  mcu->idataMemorySize = 0x100;
  mcu->xdataMemorySize = MAX_EXT_RAM_SIZE;
  mcu->iromMemorySize = 0x2000;
  mcu->xromMemorySize = MAX_ROM_SIZE;

  mcu->_timers = &timers8052;
  mcu->_interrupts = &interrupts8052;
  mcu->_additionalCode = NULL;
}

void init8032MCU(MCU* mcu)
{
  init8052MCU(mcu);

  mcu->idataMemorySize = 0x100;
  mcu->xdataMemorySize = MAX_EXT_RAM_SIZE;
  mcu->iromMemorySize = 0;
  mcu->xromMemorySize = MAX_ROM_SIZE;
}

void init89S51MCU(MCU* mcu)
{
  init8051MCU(mcu);
  mcu->WDTEnable = false;
  mcu->WDTTimerValue = 0;

  mcu->idataMemorySize = 0x100;
  mcu->xdataMemorySize = MAX_EXT_RAM_SIZE;
  mcu->iromMemorySize = 0x1000;
  mcu->xromMemorySize = MAX_ROM_SIZE;

  mcu->DP1H = &mcu->sfr[0x85 - 0x80];
  mcu->DP1L = &mcu->sfr[0x84 - 0x80];
  mcu->DP0H = &mcu->sfr[0x83 - 0x80];
  mcu->DP0L = &mcu->sfr[0x82 - 0x80];
  mcu->WDTRST = &mcu->sfr[0xA6 - 0x80];
  mcu->AUXR1 = &mcu->sfr[0xA2 - 0x80];
  mcu->AUXR = &mcu->sfr[0x8E - 0x80];
  mcu->DPTR = (WORD*)&mcu->sfr[0x82 - 0x80];

  mcu->_additionalCode = &Mcu89S5x;
}

void init89S52MCU(MCU* mcu)
{
  init8052MCU(mcu);
  mcu->WDTEnable = false;
  mcu->WDTTimerValue = 0;

  mcu->idataMemorySize = 0x100;
  mcu->xdataMemorySize = MAX_EXT_RAM_SIZE;
  mcu->iromMemorySize = 0x2000;
  mcu->xromMemorySize = MAX_ROM_SIZE;

  mcu->DP1H = &mcu->sfr[0x85 - 0x80];
  mcu->DP1L = &mcu->sfr[0x84 - 0x80];
  mcu->DP0H = &mcu->sfr[0x83 - 0x80];
  mcu->DP0L = &mcu->sfr[0x82 - 0x80];
  mcu->WDTRST = &mcu->sfr[0xA6 - 0x80];
  mcu->AUXR1 = &mcu->sfr[0xA2 - 0x80];
  mcu->AUXR = &mcu->sfr[0x8E - 0x80];
  mcu->DPTR = (WORD*)&mcu->sfr[0x82 - 0x80];

  mcu->_additionalCode = &Mcu89S5x;
}

void resetMCU(MCU* mcu)
{
  mcu->PC = 0;
  mcu->cycles = 0;
  mcu->instructions = 0;

  mcu->OUTPUT = -1;
  mcu->INPUT = -1;

  mcu->maxValueOfStackPointer = 0x07;
  mcu->minIntRamAddress = -1;
  mcu->maxIntRamAddress = -1;
  mcu->minExtRamAddress = -1;
  mcu->maxExtRamAddress = -1;
  mcu->maxIntRomAddress = 0;
  mcu->maxExtRomAddress = 0;

  memset(mcu->sfr, 0, SFR_SIZE);

  *mcu->P0 = 0xFF;
  *mcu->P1 = 0xFF;
  *mcu->P2 = 0xFF;
  *mcu->P3 = 0xFF;
  *mcu->SP = 0x07;

  mcu->accessedSFR = -1;
  mcu->accessedIntRAM = -1;
  mcu->accessedExtRAM = -1;
  mcu->accessedIntROM = -1;
  mcu->accessedExtROM = -1;
  mcu->changedSFR = 0x80;
  mcu->changedIntRAM = 0;
  mcu->changedExtRAM = 0;
}

void removeMCU(MCU* mcu)
{
  clearAllBreakpointsAndPauses(mcu);
}

char* getError(MCU* mcu)
{
  switch (mcu->errid) {
  case E_NOERRORS:
    return NULL;
    break;
  case E_IRAMOUTSIDE:
    return "Using an address outside the available internal ram memory.";
    break;
  case E_XRAMOUTSIDE:
    return "Using an address outside the available external ram memory.";
    break;
  case E_UNSUPPORTED:
    return "Using unsupported instruction.";
    break;
  default:
    return "Undefined error.";
    break;
  }
}

inline BYTE* intRAM(MCU* mcu, BYTE address, bool direct)
{
  return _mcuIntRAM(mcu, address, direct, false);
}

inline BYTE* extRAM(MCU* mcu, WORD address)
{
  return _mcuExtRAM(mcu, address, false);
}

inline BYTE* ROM(MCU* mcu, WORD address)
{
  return _mcuROM(mcu, address, false);
}

extern inline bool checkRegister(MCU* mcu, WORD flag)
{
  return (*mcuIntRAM(mcu, (BYTE) (flag >> 8), true) & (BYTE) (flag & 0x00FF)) > 0;
}

extern inline void setRegister(MCU* mcu, WORD flag, bool state)
{
  if (state)
    *mcuIntRAM(mcu, (BYTE) (flag >> 8), true) |= (BYTE) (flag & 0x00FF);
  else
    *mcuIntRAM(mcu, (BYTE) (flag >> 8), true) &= ~((BYTE) (flag & 0x00FF));
}


bool isBreakpointOrPause(MCU* mcu)
{
  int i = 0;

  /*
   * PC Breakpoints.
   */
  for (i = 0; i < mcu->numOfPCBreakpoints; ++i)
    if (mcu->PCBreakpoints[i] == mcu->PC)
      return true;

  /*
   * Access pauses.
   */
  for (i = 0; i < mcu->numOfaccessIntRAMPauses; ++i)
    if (mcu->accessIntRAMPauses[i] == mcu->accessedIntRAM)
      return true;

  for (i = 0; i < mcu->numOfAccessExtRAMPauses; ++i) {
    if (mcu->accessExtRAMPauses[i] == mcu->accessedExtRAM)
      return true;
  }

  for (i = 0; i < mcu->numOfAccessIntROMPauses; ++i)
    if (mcu->accessIntROMPauses[i] == mcu->accessedIntROM)
      return true;

  for (i = 0; i < mcu->numOfAccessExtROMPauses; ++i)
    if (mcu->accessExtROMPauses[i] == mcu->accessedExtROM)
      return true;

  for (i = 0; i < mcu->numOfAccessSFRPauses; ++i)
    if (mcu->accessSFRPauses[i] == mcu->accessedSFR)
      return true;

  /*
   * Conditional pauses.
   */
  for (int n = 0; n < 3; ++n) {

    int num = 0;
    BYTE val1 = 0;
    BYTE val2 = 0;
    BreakpointType type = 0;

    if (n == 0)
      num = mcu->numOfIntRAMPauses;
    else if (n == 1)
      num = mcu->numOfExtRAMPauses;
    else if (n == 2)
      num = mcu->numOfSFRPauses;

    for (i = 0; i < num; ++i) {
      if (n == 0) {
        val1 = mcu->idata[mcu->intRAMPauses[i].address];
        val2 = mcu->intRAMPauses[i].value;
        type = mcu->intRAMPauses[i].type;
      } else if (n == 1) {
        val1 = mcu->xdata[mcu->extRAMPauses[i].address];
        val2 = mcu->extRAMPauses[i].value;
        type = mcu->extRAMPauses[i].type;
      } else if (n == 2) {
        val1 = mcu->sfr[mcu->SFRPauses[i].address];
        val2 = mcu->SFRPauses[i].value;
        type = mcu->SFRPauses[i].type;
      }

      switch (type) {
      case EQUAL:
        if (val1 == val2)
          return true;
        break;
      case NOT_EQUAL:
        if (val1 != val2)
          return true;
        break;
      case LESS:
        if (val1 < val2)
          return true;
        break;
      case LESS_EQUAL:
        if (val1 <= val2)
          return true;
        break;
      case GREATER:
        if (val1 > val2)
          return true;
        break;
      case GREATER_EQUAL:
        if (val1 >= val2)
          return true;
        break;
      case AND:
        if (val1 & val2)
          return true;
        break;
      case OR:
        if (val1 | val2)
          return true;
        break;
      case XOR:
        if (val1 ^ val2)
          return true;
        break;
      }
    }
  }

  return false;
}

void setPCBreakpoint(MCU* mcu, WORD address)
{
  /* Sprawdz, czy już nie istnieje taki. */
  for (int i = 0; i < mcu->numOfPCBreakpoints; ++i)
    if (mcu->PCBreakpoints[i] == address)
      return;

  mcu->numOfPCBreakpoints += 1;
  mcu->PCBreakpoints = realloc(mcu->PCBreakpoints,
                               mcu->numOfPCBreakpoints * sizeof(WORD*));
  mcu->PCBreakpoints[mcu->numOfPCBreakpoints - 1] = address;
}

void setAccessPause(MCU* mcu, MemoryType memoryType, WORD address)
{
  int* num = NULL;
  WORD** breakpoints = NULL;

  switch (memoryType) {
  case IDATA:
    num = &mcu->numOfaccessIntRAMPauses;
    breakpoints = &mcu->accessIntRAMPauses;
    break;
  case XDATA:
    num = &mcu->numOfAccessExtRAMPauses;
    breakpoints = &mcu->accessExtRAMPauses;
    break;
  case SFR:
    num = &mcu->numOfAccessSFRPauses;
    breakpoints = &mcu->accessSFRPauses;
    break;
  case IROM:
    num = &mcu->numOfAccessIntROMPauses;
    breakpoints = &mcu->accessIntROMPauses;
    break;
  case XROM:
    num = &mcu->numOfAccessExtROMPauses;
    breakpoints = &mcu->accessExtROMPauses;
    break;
  default:
    return;
  }

  if (memoryType == IDATA && address > 0xFF)
    return;
  if (memoryType == SFR && (address < 0x80 || address > 0xFF))
    return;

  /* Sprawdz, czy już nie istnieje taki. */
  for (int i = 0; i < *num; ++i)
    if (**breakpoints == address)
      return;

  *num += 1;
  *breakpoints = realloc(*breakpoints, *num * sizeof(WORD*));
  *breakpoints[*num - 1] = address;
}

void setCondPause(MCU* mcu, MemoryType memoryType,
                  WORD address, BreakpointType breakType, BYTE value)
{
  int* num = NULL;
  MCUConditionBreakpoint** breakpoints = NULL;

  switch (memoryType) {
  case IDATA:
    num = &mcu->numOfIntRAMPauses;
    breakpoints = &mcu->intRAMPauses;
    break;
  case XDATA:
    num = &mcu->numOfExtRAMPauses;
    breakpoints = &mcu->extRAMPauses;
    break;
  case SFR:
    num = &mcu->numOfSFRPauses;
    breakpoints = &mcu->SFRPauses;
    break;
  default:
    return;
  }

  if (memoryType == IDATA && address > 0xFF)
    return;
  if (memoryType == SFR && (address < 0x80 || address > 0xFF))
    return;

  /* Sprawdz, czy już nie istnieje taki. */
  for (int i = 0; i < *num; ++i)
    if ((*breakpoints)->address == address)
      return;

  *num += 1;
  *breakpoints = realloc(*breakpoints, *num * sizeof(MCUConditionBreakpoint*));
  (*breakpoints)[*num - 1].address = address;
  (*breakpoints)[*num - 1].type = breakType;
  (*breakpoints)[*num - 1].value = value;
}

void clearPCBreakpoint(MCU* mcu, WORD address)
{
  for (int i = 0; i < mcu->numOfPCBreakpoints; ++i) {
    if (mcu->PCBreakpoints[i] == address) {
      if (i == mcu->numOfPCBreakpoints - 1) {
        mcu->numOfPCBreakpoints -= 1;
        mcu->PCBreakpoints = realloc(mcu->PCBreakpoints,
                                     mcu->numOfPCBreakpoints * sizeof(WORD*));
      } else {
        memmove(&mcu->PCBreakpoints[i], &mcu->PCBreakpoints[i + 1],
                (mcu->numOfPCBreakpoints - i - 1) * sizeof(WORD*));
        mcu->numOfPCBreakpoints -= 1;
        mcu->PCBreakpoints = realloc(mcu->PCBreakpoints,
                                     mcu->numOfPCBreakpoints * sizeof(WORD*));
      }
      /* szukaj dalej gdyby breakpoint się powtarzał */
      i -= 1;
      continue;
    }
  }
}

void clearAccessPause(MCU* mcu, MemoryType memoryType, WORD address)
{
  int* num = NULL;
  WORD* breakpoints = NULL;

  switch (memoryType) {
  case IDATA:
    num = &mcu->numOfaccessIntRAMPauses;
    breakpoints = mcu->accessIntRAMPauses;
    break;
  case XDATA:
    num = &mcu->numOfAccessExtRAMPauses;
    breakpoints = mcu->accessExtRAMPauses;
    break;
  case SFR:
    num = &mcu->numOfAccessSFRPauses;
    breakpoints = mcu->accessSFRPauses;
    break;
  case IROM:
    num = &mcu->numOfAccessIntROMPauses;
    breakpoints = mcu->accessIntROMPauses;
    break;
  case XROM:
    num = &mcu->numOfAccessExtROMPauses;
    breakpoints = mcu->accessExtROMPauses;
    break;
  default:
    return;
  }

  for (int i = 0; i < *num; ++i) {
    if (breakpoints[i] == address) {
      if (i == *num - 1) {
        *num -= 1;
        breakpoints = realloc(breakpoints, *num * sizeof(WORD*));
      } else {
        memmove(&breakpoints[i], &breakpoints[i + 1], (*num - i - 1) * sizeof(WORD*));
        *num -= 1;
        breakpoints = realloc(breakpoints, *num * sizeof(WORD*));
      }
      /* szukaj dalej gdyby breakpoint się powtarzał */
      i -= 1;
      continue;
    }
  }
}

void clearCondPause(MCU* mcu, MemoryType memoryType, WORD address)
{
  int* num = NULL;
  MCUConditionBreakpoint* breakpoints = NULL;

  switch (memoryType) {
  case IDATA:
    num = &mcu->numOfIntRAMPauses;
    breakpoints = mcu->intRAMPauses;
    break;
  case XDATA:
    num = &mcu->numOfExtRAMPauses;
    breakpoints = mcu->extRAMPauses;
    break;
  case SFR:
    num = &mcu->numOfSFRPauses;
    breakpoints = mcu->SFRPauses;
    break;
  default:
    return;
  }

  for (int i = 0; i < *num; ++i) {
    if (breakpoints[i].address == address) {
      if (i == *num - 1) {
        *num -= 1;
        breakpoints = realloc(breakpoints, *num * sizeof(WORD*));
      } else {
        memmove(&breakpoints[i], &breakpoints[i + 1], (*num - i - 1) * sizeof(WORD*));
        *num -= 1;
        breakpoints = realloc(breakpoints, *num * sizeof(WORD*));
      }
      /* szukaj dalej gdyby breakpoint się powtarzał */
      i -= 1;
      continue;
    }
  }
}

void clearAllBreakpointsAndPauses(MCU* mcu)
{
  free(mcu->PCBreakpoints);
  free(mcu->accessIntRAMPauses);
  free(mcu->accessExtRAMPauses);
  free(mcu->accessIntROMPauses);
  free(mcu->accessExtROMPauses);
  free(mcu->accessSFRPauses);
  free(mcu->intRAMPauses);
  free(mcu->extRAMPauses);
  free(mcu->SFRPauses);

  mcu->numOfPCBreakpoints = 0;
  mcu->numOfaccessIntRAMPauses = 0;
  mcu->numOfAccessExtRAMPauses = 0;
  mcu->numOfAccessIntROMPauses = 0;
  mcu->numOfAccessExtROMPauses = 0;
  mcu->numOfAccessSFRPauses = 0;
  mcu->numOfIntRAMPauses = 0;
  mcu->numOfExtRAMPauses = 0;
  mcu->numOfSFRPauses = 0;

  mcu->PCBreakpoints = NULL;
  mcu->accessIntRAMPauses = NULL;
  mcu->accessExtRAMPauses = NULL;
  mcu->accessIntROMPauses = NULL;
  mcu->accessExtROMPauses = NULL;
  mcu->accessSFRPauses = NULL;
  mcu->intRAMPauses = NULL;
  mcu->extRAMPauses = NULL;
  mcu->SFRPauses = NULL;
}

double getMCUTime(MCU* mcu)
{
  return (double)mcu->cycles / ((double)mcu->oscillator / 12);
}

void processMCU(MCU* mcu)
{
  // Reset error.
  mcu->errid = E_NOERRORS;

  // Hack!
  mcu->INPUT = *mcu->SBUF;

  /*
   * Update pins.
   */
  if (mcu->EAconnect != NULL)
    mcu->EA = mcuCheckBit(mcu, mcu->EAconnect);

  /*
   * Actual instruction
   */
  mcu->lastInstruction = *mcuROM(mcu, mcu->PC);

  /*
   * Number of executed instructions.
   */
  mcu->instructions += 1;

  /*
   * Cycles.
   */
  mcu->cycles += mcu->cycleCount[mcu->lastInstruction];

  /*
   * Instructions.
   */
  if (mcu->_instructions[mcu->lastInstruction] != NULL)
    mcu->_instructions[mcu->lastInstruction](mcu);
  else {
    mcu->errid = E_UNSUPPORTED;
    mcu->PC += 1;
  }

  mcu->PC %= mcu->iromMemorySize > mcu->xromMemorySize ?
             mcu->iromMemorySize : mcu->xromMemorySize;

  /*
   * Update information about last changed memory
   */
  if (mcu->_beforeAccessedSFR != mcu->sfr[mcu->accessedSFR - 0x80])
    mcu->changedSFR = mcu->accessedSFR;

  if (mcu->_beforeAccessedIntRAM != mcu->idata[mcu->accessedIntRAM])
    mcu->changedIntRAM = mcu->accessedIntRAM;

  if (mcu->_beforeAccessedExtRAM != mcu->xdata[mcu->accessedExtRAM])
    mcu->changedExtRAM = mcu->accessedExtRAM;

  /*
   * Aktualizuje najwyższe i najniższe wartości adresów.
   */
  if (*mcu->SP > mcu->maxValueOfStackPointer)
    mcu->maxValueOfStackPointer = *mcu->SP;

  if (mcu->accessedIntRAM > mcu->maxIntRamAddress)
    mcu->maxIntRamAddress = mcu->accessedIntRAM;

  if (mcu->accessedIntRAM < mcu->minIntRamAddress || mcu->minIntRamAddress == -1)
    mcu->minIntRamAddress = mcu->accessedIntRAM;

  if (mcu->accessedExtRAM > mcu->maxExtRamAddress)
    mcu->maxExtRamAddress = mcu->accessedExtRAM;

  if (mcu->accessedExtRAM < mcu->minExtRamAddress || mcu->minExtRamAddress == -1)
    mcu->minExtRamAddress = mcu->accessedExtRAM;

  if (mcu->accessedIntROM > mcu->maxIntRomAddress)
    mcu->maxIntRomAddress = mcu->accessedIntROM;

  if (mcu->accessedExtROM > mcu->maxExtRomAddress)
    mcu->maxExtRomAddress = mcu->accessedExtROM;

  if (!mcu->EA && mcu->PC > mcu->maxIntRomAddress)
    mcu->maxIntRomAddress = mcu->PC;

  if (mcu->EA && mcu->PC > mcu->maxExtRomAddress)
    mcu->maxExtRomAddress = mcu->PC;

  /*
   * Parity flag.
   */
  if (mcu->accessedSFR == 0xE0) {
    int i = 0;
    int n = 0;
    for (; i < 7; ++i)
      if ((1 << i) & *mcu->ACC)
        n += 1;

    setRegister(mcu, PSW_P, 0 == (n % 2));
  }

  /*
   * Register banks.
   */
  mcu->R = mcuIntRAM(mcu, (checkRegister(mcu, PSW_RS0) << 1 | checkRegister(mcu, PSW_RS1))
                     << 3, true);

  /*
   * Timers.
   */
  if (mcu->_timers != NULL)
    mcu->_timers(mcu);

  /*
   * Interrupts
   */
  if (mcu->_interrupts != NULL)
    mcu->_interrupts(mcu);

  /*
   * Additional microcontroller specific code.
   */
  if (mcu->_additionalCode != NULL)
    mcu->_additionalCode(mcu);
}

bool processMCUEx(MCU* mcu, BYTE* out, BYTE* in, bool* useOut, bool* needIn)
{
  processMCU(mcu);

  if (out != NULL)
    *useOut = false;

  if (in != NULL)
    *needIn = false;

  /*
   * Output.
   */
  if (mcu->autoRead && mcu->OUTPUT != -1 && !checkRegister(mcu, SCON_TI)) {
    if (out != NULL) {
      *out = mcu->OUTPUT;
      *useOut = true;
    }

    setRegister(mcu, SCON_TI, true);
    mcu->OUTPUT = -1;
  }

  /*
   * Input.
   */
  if (mcu->autoWrite && checkRegister(mcu, SCON_REN) && !checkRegister(mcu, SCON_RI)) {
    if (in != NULL) {
      *mcu->SBUF =  *in;
      *needIn = true;
      setRegister(mcu, SCON_RI, true);
    }
    mcu->INPUT = -1;
  }

  /*
   * Disable debuging code. This speed up simulator
   */
  if (mcu->noDebug)
    return true;

  /*
   * Breakpoint.
   */
  if (isBreakpointOrPause(mcu))
    return false;

  /*
   * Error.
   */
  if (mcu->errid != E_NOERRORS)
    return false;

  return true;
}

/*
 * Disassembler.
 */
char* mcuParamParse(MCU* mcu, WORD ip)
{
  const char* params = mcu->mnemonicParams[*ROM(mcu, ip)];
  char* temp = NULL;

  int s = strlen(params);
  int i = 0; // current char in pattern
  int n = 0; // current char in result

  while (i < s) {

    if (params[i] == '%' && i < (s - 1)) {
      // obsługa parametrów

      WORD address;

      // parametr
      if (params[i + 1] == '1')
        address = *ROM(mcu, ip + 1);
      else if (params[i + 1] == '2')
        address = *ROM(mcu, ip + 2);
      else
        continue;

      // Dla parametru O:
      // Powiekszam zaalokowany obszar o 6 bitów, gdyż %.4Xh daje 5 bitów,
      // i \0. Na koniec n zwiekszam tylko o 4 dlatego, że nadpisuje o jeden
      // znak wcześniej przed % usuwając parametr O. W pozostałch przypadkach z
      // podanym parametrem poprzedzającym % analgicznie. Na koniec zwiększam
      // i o 2 aby przeszkoczyć %1 lub %2.

      // offset
      if (i > 0 && params[i - 1] == 'O') {
        address = ip + (signed char) address + mcu->byteCount[*ROM(mcu, ip)];
        temp = realloc(temp, n + 6);
        sprintf(&temp[n - 1], "%.4Xh", address);
        n += 4;
      }
      // bit
      else if (i > 0 && params[i - 1] == '0') {
        temp = realloc(temp, n + strlen(mcu->SFRBits[address]) + 1);
        sprintf(&temp[n - 1], "%s", mcu->SFRBits[address]);
        n += strlen(mcu->SFRBits[address]) - 1;
      }
      // no replace
      else if (i > 0 && params[i - 1] == 'N') {
        temp = realloc(temp, n + 3);
        sprintf(&temp[n - 1], "%.2X", address);
        n += 1;
      }
      // replace to sfr name
      else {
        temp = realloc(temp, n + strlen(mcu->SFRNames[address]) + 1);
        sprintf(&temp[n], "%s", mcu->SFRNames[address]);
        n += strlen(mcu->SFRNames[address]);
      }

      i += 2;
    } else {
      // Po prostu kopiuje znak
      temp = realloc(temp, n + 1);
      temp[n] = params[i];
      n += 1;
      i += 1;
    }
  }

  temp = realloc(temp, n + 1);
  temp[n] = '\0';

  return temp;
}

char* disassembler(MCU* mcu, WORD ip, char* format, WORD* next)
{
  char* temp = NULL;

  int s = strlen(format);
  int i = 0; // current char in pattern
  int n = 0; // current char in result

  while (i < s) {
    if (format[i] == '%' && format[i + 1] == 'a') {
      temp = realloc(temp, n + 5);
      sprintf(&temp[n], "%.4X", ip);
      n += 4;
      i += 2;
    } else if (format[i] == '%' && format[i + 1] == 'm') {
      temp = realloc(temp, n + strlen(mcu->mnemonicTable[*ROM(mcu, ip)]) + 1);
      sprintf(&temp[n], "%s", mcu->mnemonicTable[*ROM(mcu, ip)]);
      n += strlen(mcu->mnemonicTable[*ROM(mcu, ip)]);
      i += 2;
    } else if (format[i] == '%' && format[i + 1] == 'o') {
      int j = 0;
      for (; j < 3; ++j) {
        temp = realloc(temp, n + 4);
        if (j < mcu->byteCount[*ROM(mcu, ip)])
          sprintf(&temp[n], "%.2X ", *ROM(mcu, ip + j));
        else
          sprintf(&temp[n], "   ");
        n += 3;
      }
      n -= 1; // remove last space
      i += 2;
    } else if (format[i] == '%' && format[i + 1] == 'p') {
      char* params = mcuParamParse(mcu, ip);
      int s = strlen(params) < 20 ? 20 : strlen(params);
      temp = realloc(temp, n + s + 1);
      sprintf(&temp[n], "%-20s", params);
      n += s;
      i += 2;
      free(params);
    } else if (format[i] == '\\' && format[i + 1] == 'n') {
      temp = realloc(temp, n + 1);
      temp[n] = '\n';
      n += 1;
      i += 2;
    } else if (format[i] == '\\' && format[i + 1] == 't') {
      temp = realloc(temp, n + 1);
      temp[n] = '\t';
      n += 1;
      i += 2;
    } else if (format[i] == '\\' && format[i + 1] >= '0' && format[i + 1] <= '7') {
      int j = 0, c = 0;
      sscanf(&format[i + 1], "%3o%n", &c, &j);
      temp = realloc(temp, n + 1);
      temp[n] = (char) c;
      n += 1;
      i += 1 + j;
    } else if (format[i] == '\\' && format[i + 1] == 'x') {
      int j = 0, c = 0;
      sscanf(&format[i + 1], "%2x%n", &c, &j);
      temp = realloc(temp, n + 1);
      temp[n] = (char) c;
      n += 1;
      i += 1 + j;
    } else {
      temp = realloc(temp, n + 1);
      temp[n] = format[i];
      i += 1;
      n += 1;
    }
  }
  if (next != NULL)
    *next = ip + mcu->byteCount[*ROM(mcu, ip)];

  temp = realloc(temp, n + 1);
  temp[n] = '\0';

  return temp;
}

/*
vi:ts=4:et:nowrap
*/
