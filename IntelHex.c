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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Utils.h"
#include "IntelHex.h"
#include "MCS51.h"

void loadIntelHexFile(char* filename, BYTE* rom, WORD start, WORD end, bool* valid, WORD* highestAddress)
{
  memset(rom, 0, MAX_ROM_SIZE);
  *valid = true;

  if (highestAddress != NULL)
    *highestAddress = 0;

  FILE* file = NULL;
  if (*filename != 0)
    file = fopen (filename, "r");

  if (file == NULL) {
    fprintf(AppSettings()->errorOut, "Error while trying to open '%s': %s.\n", filename,
            strerror(errno));

    rom = NULL;
    *valid = false;
    return;
  }

  char line[522];
  while (!feof(file)) {
    unsigned i;
    char chrStartCode[2];
    char chrByteCount[3];
    char chrAddress[5];
    char chrRecordType[3];
    char chrChecksum[3];
    char chrTempByte[3];
    unsigned address = 0;
    unsigned byteCount = 0;
    unsigned recordType = 0;
    unsigned checksumInFile = 0;
    unsigned tempByte = 0;

    fscanf(file, "%521s", line);

    if (strlen(line) < 11 && strlen(line) % 2 == 1) {
      *valid = false;
      continue;
    }

    sscanf(line, "%1c", chrStartCode);
    sscanf(line + 1, "%2c", chrByteCount);
    sscanf(line + 3, "%4c", chrAddress);
    sscanf(line + 7, "%2c", chrRecordType);
    sscanf(chrAddress, "%4x", &address);
    sscanf(chrByteCount, "%X", &byteCount);
    sscanf(chrRecordType, "%X", &recordType);
    BYTE checksum = (BYTE)byteCount + (BYTE)(address && 0x00FF)
                    + (BYTE)((address && 0xFF00) >> 2) + (BYTE)recordType;

    for (i = 0; i < byteCount; ++i) {
      char* from = line + 9 + i * 2;

      if (from - line - strlen(line) < 2) {
        *valid = false;
        break;
      }

      sscanf(from, "%2c", chrTempByte);
      sscanf(chrTempByte, "%2x", &tempByte);
      checksum += (BYTE)tempByte;

      // obsługuje jedynie typ 0, pozostałe nie mają zastosowania dla tej
      // architekruty.
      if (recordType == 0x00) {
        if (address >= start && address <= end)
          memcpy(&rom[address], &tempByte, 1);

        if (highestAddress != NULL) {
          if (address > *highestAddress)
            *highestAddress = address;
        }
      }

      address += 1;
    }

    // koniec pliku, przerwij
    if (recordType == 0x01)
      break;

    sscanf(line + strlen(line) - 2, "%2c", chrChecksum);
    sscanf(chrChecksum, "%2x", &checksumInFile);

    // sprawdzić checksumy
  }

  fclose(file);
}

/*
vi:ts=4:et:nowrap
*/
