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

#include "DeAsm.h"
#include "DeAsmTables.h"

char* paramParse(BYTE* rom, WORD ip)
{
  const char* params = MNEMONIC_PARAMS[rom[ip]];
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
        address = rom[ip + 1];
      else if (params[i + 1] == '2')
        address = rom[ip + 2];
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
        address = ip + (signed char) address + BYTE_COUNT[rom[ip]];
        temp = realloc(temp, n + 6);
        sprintf(&temp[n - 1], "%.4Xh", address);
        n += 4;
      }
      // bit
      else if (i > 0 && params[i - 1] == '0') {
        temp = realloc(temp, n + strlen(SFR_BITS[address]) + 1);
        sprintf(&temp[n - 1], "%s", SFR_BITS[address]);
        n += strlen(SFR_BITS[address]) - 1;
      }
      // no replace
      else if (i > 0 && params[i - 1] == 'N') {
        temp = realloc(temp, n + 3);
        sprintf(&temp[n - 1], "%.2X", address);
        n += 1;
      }
      // replace to sfr name
      else {
        temp = realloc(temp, n + strlen(SFR_NAMES[address]) + 1);
        sprintf(&temp[n], "%s", SFR_NAMES[address]);
        n += strlen(SFR_NAMES[address]);
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

char* formatParse(BYTE* rom, WORD ip, char* format)
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
      temp = realloc(temp, n + strlen(MNEMONIC_TABLE[rom[ip]]) + 1);
      sprintf(&temp[n], "%s", MNEMONIC_TABLE[rom[ip]]);
      n += strlen(MNEMONIC_TABLE[rom[ip]]);
      i += 2;
    } else if (format[i] == '%' && format[i + 1] == 'o') {
      int j = 0;
      for (; j < 3; ++j) {
        temp = realloc(temp, n + 4);
        if (j < BYTE_COUNT[rom[ip]])
          sprintf(&temp[n], "%.2X ", rom[ip + j]);
        else
          sprintf(&temp[n], "   ");
        n += 3;
      }
      n -= 1; // remove last space
      i += 2;
    } else if (format[i] == '%' && format[i + 1] == 'p') {
      char* params = paramParse(rom, ip);
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

  temp = realloc(temp, n + 1);
  temp[n] = '\0';

  return temp;
}

/*
 * Opis w DeAsm.h
 */
char* deAsm(BYTE* rom, int instructions, WORD from, char* format, bool cutNops, WORD* nextAddr)
{
  char* result = NULL;
  int size = 0;

  unsigned ic = 0;
  unsigned ip = from;
  unsigned end = 0xFFFF;

  if (cutNops) while (!rom[end] && end > 0) end -= 1;

  if (end == 0)
    return NULL;

  while (ip <= end && ic < instructions) {
    char* temp = formatParse(rom, ip, format);

    result = realloc(result, size + strlen(temp));
    memcpy(&result[size], temp, strlen(temp));
    size += strlen(temp);

    ip += BYTE_COUNT[rom[ip]];
    ic += 1;

    free(temp);
  }

  if (nextAddr != NULL)
    *nextAddr = ip;

  result = realloc(result, size + 1);
  result[size] = '\0';
  return result;
}

/*
vi:ts=4:et:nowrap
*/
