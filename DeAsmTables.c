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

/*
 * WYGENEROWANY KOD
 */

const int BYTE_COUNT[] = {
  1, 2, 3, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 3, 1, 1, 2, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 3, 2, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 1,
  2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 2, 2, 2, 3, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 2, 2, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 1, 1, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 1, 2, 2, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  2, 2, 2, 1, 1, 3, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 2, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

const char* MNEMONIC_TABLE[] = {
  "nop  ", "ajmp ", "ljmp ", "rr   ", "inc  ", "inc  ", "inc  ", "inc  ",
  "inc  ", "inc  ", "inc  ", "inc  ", "inc  ", "inc  ", "inc  ", "inc  ",
  "jbc  ", "acall", "lcall", "rrc  ", "dec  ", "dec  ", "dec  ", "dec  ",
  "dec  ", "dec  ", "dec  ", "dec  ", "dec  ", "dec  ", "dec  ", "dec  ",
  "jb   ", "ajmp ", "ret  ", "rl   ", "add  ", "add  ", "add  ", "add  ",
  "add  ", "add  ", "add  ", "add  ", "add  ", "add  ", "add  ", "add  ",
  "jnb  ", "acall", "reti ", "rlc  ", "addc ", "addc ", "addc ", "addc ",
  "addc ", "addc ", "addc ", "addc ", "addc ", "addc ", "addc ", "addc ",
  "jc   ", "ajmp ", "orl  ", "orl  ", "orl  ", "orl  ", "orl  ", "orl  ",
  "orl  ", "orl  ", "orl  ", "orl  ", "orl  ", "orl  ", "orl  ", "orl  ",
  "jnc  ", "acall", "anl  ", "anl  ", "anl  ", "anl  ", "anl  ", "anl  ",
  "anl  ", "anl  ", "anl  ", "anl  ", "anl  ", "anl  ", "anl  ", "anl  ",
  "jz   ", "ajmp ", "xrl  ", "xrl  ", "xrl  ", "xrl  ", "xrl  ", "xrl  ",
  "xrl  ", "xrl  ", "xrl  ", "xrl  ", "xrl  ", "xrl  ", "xrl  ", "xrl  ",
  "jnz  ", "acall", "orl  ", "jmp  ", "mov  ", "mov  ", "mov  ", "mov  ",
  "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ",
  "sjmp ", "ajmp ", "anl  ", "movc ", "div  ", "mov  ", "mov  ", "mov  ",
  "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ",
  "mov  ", "acall", "mov  ", "movc ", "subb ", "subb ", "subb ", "subb ",
  "subb ", "subb ", "subb ", "subb ", "subb ", "subb ", "subb ", "subb ",
  "orl  ", "ajmp ", "mov  ", "inc  ", "mul  ", "RESVD", "mov  ", "mov  ",
  "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ",
  "anl  ", "acall", "cpl  ", "cpl  ", "cjne ", "cjne ", "cjne ", "cjne ",
  "cjne ", "cjne ", "cjne ", "cjne ", "cjne ", "cjne ", "cjne ", "cjne ",
  "push ", "ajmp ", "clr  ", "clr  ", "swap ", "xch  ", "xch  ", "xch  ",
  "xch  ", "xch  ", "xch  ", "xch  ", "xch  ", "xch  ", "xch  ", "xch  ",
  "pop  ", "acall", "setb ", "setb ", "da   ", "djnz ", "xchd ", "xchd ",
  "djnz ", "djnz ", "djnz ", "djnz ", "djnz ", "djnz ", "djnz ", "djnz ",
  "movx ", "ajmp ", "movx ", "movx ", "clr  ", "mov  ", "mov  ", "mov  ",
  "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ",
  "movx ", "acall", "movx ", "movx ", "cpl  ", "mov  ", "mov  ", "mov  ",
  "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  ", "mov  "
};


const char* MNEMONIC_PARAMS[] = {
  "", "00N%1", "N%1N%2", "A", "A", "%1", "@R0", "@R1", "R0", "R1", "R2", "R3",
  "R4", "R5", "R6", "R7", "0%1, O%2", "00N%1", "N%1N%2", "A", "A", "%1", "@R0",
  "@R1", "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "0%1, O%2", "01N%1",
  "", "A", "A, #%1", "A, %1", "A, @R0", "A, @R1", "A, R0", "A, R1", "A, R2",
  "A, R3", "A, R4", "A, R5", "A, R6", "A, R7", "0%1, O%2", "01N%1", "", "A",
  "A, #%1", "A, %1", "A, @R0", "A, @R1", "A, R0", "A, R1", "A, R2", "A, R3",
  "A, R4", "A, R5", "A, R6", "A, R7", "O%1", "02N%1", "%1, A", "%1, #%2",
  "A, #%1", "A, %1", "A, @R0", "A, @R1", "A, R0", "A, R1", "A, R2", "A, R3",
  "A, R4", "A, R5", "A, R6", "A, R7", "O%1", "02N%1", "%1, A", "%1, #%2",
  "A, #%1", "A, %1", "A, @R0", "A, @R1", "A, R0", "A, R1", "A, R2", "A, R3",
  "A, R4", "A, R5", "A, R6", "A, R7", "O%1", "03N%1", "%1, A", "%1, #%2",
  "A, #%1", "A, %1", "A, @R0", "A, @R1", "A, R0", "A, R1", "A, R2", "A, R3",
  "A, R4", "A, R5", "A, R6", "A, R7", "O%1", "03N%1", "C, 0%1", "@A+DPTR",
  "A, #%1", "%1, #%2", "@R0, #%1", "@R1, #%1", "R0, #%1", "R1, #%1", "R2, #%1",
  "R3, #%1", "R4, #%1", "R5, #%1", "R6, #%1", "R7, #%1", "O%1", "04N%1",
  "C, 0%1", "A, @A+PC", "AB", "%2, %1", "%1, @R0", "%1, @R1", "%1, R0",
  "%1, R1", "%1, R2", "%1, R3", "%1, R4", "%1, R5", "%1, R6", "%1, R7",
  "DPTR, #N%1N%2", "04N%1", "0%1, C", "A, @A+DPTR", "A, #%1", "A, %1",
  "A, @R0", "A, @R1", "A, R0", "A, R1", "A, R2", "A, R3", "A, R4", "A, R5",
  "A, R6", "A, R7", "C, /0%1", "05N%1", "C, 0%1", "DPTR", "AB", "", "@R0, %1",
  "@R1, %1", "R0, %1", "R1, %1", "R2, %1", "R3, %1", "R4, %1", "R5, %1",
  "R6, %1", "R7, %1", "C, /0%1", "05N%1", "0%1", "C", "A, #%1, O%2",
  "A, %1, O%2", "@R0, #%1, O%2", "@R1, #%1, O%2", "R0, #%1, O%2",
  "R1, #%1, O%2", "R2, #%1, O%2", "R3, #%1, O%2", "R4, #%1, O%2",
  "R5, #%1, O%2", "R6, #%1, O%2", "R7, #%1, O%2", "%1", "06N%1", "0%1", "C",
  "A", "A, %1", "A, @R0", "A, @R1", "A, R0", "A, R1", "A, R2", "A, R3",
  "A, R4", "A, R5", "A, R6", "A, R7", "%1", "06N%1", "0%1", "C", "A",
  "%1, O%2", "A, @R0", "A, @R1", "R0, O%1", "R1, O%1", "R2, O%1", "R3, O%1",
  "R4, O%1", "R5, O%1", "R6, O%1", "R7, O%1", "A, @DPTR", "07N%1", "A, @R0",
  "A, @R1", "A", "A, %1", "A, @R0", "A, @R1", "A, R0", "A, R1", "A, R2",
  "A, R3", "A, R4", "A, R5", "A, R6", "A, R7", "@DPTR, A", "07N%1", "@R0, A",
  "@R1, A", "A", "%1, A", "@R0, A", "@R1, A", "R0, A", "R1, A", "R2, A",
  "R3, A", "R4, A", "R5, A", "R6, A", "R7, A"
};

const char* SFR_NAMES[] = {
  "00h", "01h", "02h", "03h", "04h", "05h", "06h", "07h", "08h", "09h", "0Ah",
  "0Bh", "0Ch", "0Dh", "0Eh", "0Fh", "10h", "11h", "12h", "13h", "14h", "15h",
  "16h", "17h", "18h", "19h", "1Ah", "1Bh", "1Ch", "1Dh", "1Eh", "1Fh", "20h",
  "21h", "22h", "23h", "24h", "25h", "26h", "27h", "28h", "29h", "2Ah", "2Bh",
  "2Ch", "2Dh", "2Eh", "2Fh", "30h", "31h", "32h", "33h", "34h", "35h", "36h",
  "37h", "38h", "39h", "3Ah", "3Bh", "3Ch", "3Dh", "3Eh", "3Fh", "40h", "41h",
  "42h", "43h", "44h", "45h", "46h", "47h", "48h", "49h", "4Ah", "4Bh", "4Ch",
  "4Dh", "4Eh", "4Fh", "50h", "51h", "52h", "53h", "54h", "55h", "56h", "57h",
  "58h", "59h", "5Ah", "5Bh", "5Ch", "5Dh", "5Eh", "5Fh", "60h", "61h", "62h",
  "63h", "64h", "65h", "66h", "67h", "68h", "69h", "6Ah", "6Bh", "6Ch", "6Dh",
  "6Eh", "6Fh", "70h", "71h", "72h", "73h", "74h", "75h", "76h", "77h", "78h",
  "79h", "7Ah", "7Bh", "7Ch", "7Dh", "7Eh", "7Fh", "P0", "SP", "DPL", "DPh",
  "84h", "85h", "86h", "PCON", "TCON", "TMOD", "TL0", "TL1", "Th0", "Th1",
  "8Eh", "8Fh", "P1", "91h", "92h", "93h", "94h", "95h", "96h", "97h", "SCON",
  "SBUF", "9Ah", "9Bh", "9Ch", "9Dh", "9Eh", "9Fh", "P2", "A1h", "A2h", "A3h",
  "A4h", "A5h", "A6h", "A7h", "IE", "A9h", "AAh", "ABh", "ACh", "ADh", "AEh",
  "AFh", "P3", "B1h", "B2h", "B3h", "B4h", "B5h", "B6h", "B7h", "IP", "B9h",
  "BAh", "BBh", "BCh", "BDh", "BEh", "BFh", "C0h", "C1h", "C2h", "C3h", "C4h",
  "C5h", "C6h", "C7h", "C8h", "C9h", "CAh", "CBh", "CCh", "CDh", "CEh", "CFh",
  "PSW", "D1h", "D2h", "D3h", "D4h", "D5h", "D6h", "D7h", "D8h", "D9h", "DAh",
  "DBh", "DCh", "DDh", "DEh", "DFh", "ACC", "E1h", "E2h", "E3h", "E4h", "E5h",
  "E6h", "E7h", "E8h", "E9h", "EAh", "EBh", "ECh", "EDh", "EEh", "EFh", "B",
  "F1h", "F2h", "F3h", "F4h", "F5h", "F6h", "F7h", "F8h", "F9h", "FAh", "FBh",
  "FCh", "FDh", "FEh", "FFh"
};

const char* SFR_BITS[] = {
  "20h.0", "20h.1", "20h.2", "20h.3", "20h.4", "20h.5", "20h.6", "20h.7",
  "21h.0", "21h.1", "21h.2", "21h.3", "21h.4", "21h.5", "21h.6", "21h.7",
  "22h.0", "22h.1", "22h.2", "22h.3", "22h.4", "22h.5", "22h.6", "22h.7",
  "23h.0", "23h.1", "23h.2", "23h.3", "23h.4", "23h.5", "23h.6", "23h.7",
  "24h.0", "24h.1", "24h.2", "24h.3", "24h.4", "24h.5", "24h.6", "24h.7",
  "25h.0", "25h.1", "25h.2", "25h.3", "25h.4", "25h.5", "25h.6", "25h.7",
  "26h.0", "26h.1", "26h.2", "26h.3", "26h.4", "26h.5", "26h.6", "26h.7",
  "27h.0", "27h.1", "27h.2", "27h.3", "27h.4", "27h.5", "27h.6", "27h.7",
  "28h.0", "28h.1", "28h.2", "28h.3", "28h.4", "28h.5", "28h.6", "28h.7",
  "29h.0", "29h.1", "29h.2", "29h.3", "29h.4", "29h.5", "29h.6", "29h.7",
  "2Ah.0", "2Ah.1", "2Ah.2", "2Ah.3", "2Ah.4", "2Ah.5", "2Ah.6", "2Ah.7",
  "2Bh.0", "2Bh.1", "2Bh.2", "2Bh.3", "2Bh.4", "2Bh.5", "2Bh.6", "2Bh.7",
  "2Ch.0", "2Ch.1", "2Ch.2", "2Ch.3", "2Ch.4", "2Ch.5", "2Ch.6", "2Ch.7",
  "2Dh.0", "2Dh.1", "2Dh.2", "2Dh.3", "2Dh.4", "2Dh.5", "2Dh.6", "2Dh.7",
  "2Eh.0", "2Eh.1", "2Eh.2", "2Eh.3", "2Eh.4", "2Eh.5", "2Eh.6", "2Eh.7",
  "2Fh.0", "2Fh.1", "2Fh.2", "2Fh.3", "2Fh.4", "2Fh.5", "2Fh.6", "2Fh.7",
  "P0.0", "P0.1", "P0.2", "P0.3", "P0.4", "P0.5", "P0.6", "P0.7", "IT0", "IE0",
  "IT1", "IE1", "TR0", "TF0", "TR1", "TF1", "P1.0", "P1.1", "P1.2", "P1.3",
  "P1.4", "P1.5", "P1.6", "P1.7", "RI", "TI", "RB8", "TB8", "REN", "SM2",
  "SM1", "SM0", "P2.0", "P2.1", "P2.2", "P2.3", "P2.4", "P2.5", "P2.6", "P2.7",
  "EX0", "ET0", "EX1", "ET1", "ES", "IE.5", "IE.6", "EA", "RXD", "TXD", "INT0",
  "INT1", "T0", "T1", "WR", "RD", "PX0", "PT0", "PX1", "PT1", "PS", "IP.5",
  "IP.6", "IP.7", "C0h.0", "C0h.1", "C0h.2", "C0h.3", "C0h.4", "C0h.5",
  "C0h.6", "C0h.7", "C8h.0", "C8h.1", "C8h.2", "C8h.3", "C8h.4", "C8h.5",
  "C8h.6", "C8h.7", "P", "PSW.1", "OV", "RS0", "RS1", "F0", "AC", "CY",
  "D8h.0", "D8h.1", "D8h.2", "D8h.3", "D8h.4", "D8h.5", "D8h.6", "D8h.7",
  "ACC.0", "ACC.1", "ACC.2", "ACC.3", "ACC.4", "ACC.5", "ACC.6", "ACC.7",
  "E8h.0", "E8h.1", "E8h.2", "E8h.3", "E8h.4", "E8h.5", "E8h.6", "E8h.7",
  "B.0", "B.1", "B.2", "B.3", "B.4", "B.5", "B.6", "B.7", "F8h.0", "F8h.1",
  "F8h.2", "F8h.3", "F8h.4", "F8h.5", "F8h.6", "F8h.7"
};

/*
vi:ts=4:et:nowrap
*/
