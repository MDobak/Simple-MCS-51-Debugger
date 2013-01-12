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

#ifdef _linux_
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include "Utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#include "VT100.h"

int stricmp(const char* str1, const char* str2)
{
  int n = 0;
  while (1) {
    if (tolower(str1[n]) != tolower(str2[n]))
      return n + 1;
    if (!str1[n] && !str2[n])
      return 0;
    n += 1;
  }
  return 0;
}

char* bytetobin(char byte)
{
  char* result = malloc(9);
  int bit = 1;

  for (int i = 0; i < 8; ++i)
    if (byte & (bit << i))
      result[7 - i] = '1';
    else
      result[7 - i] = '0';
  result[8] = '\0';

  return result;
}

int hextoi(char* hex, int min, int max, int def, bool* valid)
{
  if (valid != NULL)
    *valid = true;

  int tmp;
  if (0 == sscanf(hex, "%x[h]", &tmp)) {
    if (valid != NULL)
      *valid = false;
    return def;
  }

  if (tmp > max)  {
    if (valid != NULL)
      *valid = false;
    tmp = max;
  }

  if (tmp < min) {
    if (valid != NULL)
      *valid = false;
    tmp = min;
  }

  return tmp;
}

bool boolQuestion(char* input, char* trueChars, char* falseChars, bool* valid)
{
  if (valid != NULL)
    *valid = true;

  if (strlen(input) != 1) {
    if (valid != NULL)
      *valid = false;
    return false;
  }

  int s = strlen(trueChars);
  for (int i = 0; i < s; ++i)
    if (input[0] == trueChars[i])
      return true;

  s = strlen(falseChars);
  for (int i = 0; i < s; ++i)
    if (input[0] == falseChars[i])
      return false;

  if (valid != NULL)
    *valid = false;
  return false;
}

#if defined(_WIN32)
void msSleep(unsigned int waitTime)
{
  SleepEx(waitTime, FALSE);
}
#elif defined(__linux__)
void msSleep(unsigned int waitTime)
{
  usleep(waitTime * 1000);
}
#else
void msSleep(unsigned int waitTime)
{
  int start = clock();
  int waitTime *= CLOCKS_PER_SEC / 1000;
  while (clock() - start < waitTime);
}
#endif

void print(char *format, ...)
{
#ifndef USE_COLORS
  va_list args;
  va_start(args, format);
  vfprintf(AppSettings()->out, format, args);
  va_end(args);
#else
  char cbuf[1024];
  va_list va;

  va_start(va, format);
  vsnprintf(cbuf, sizeof(cbuf), format, va);
  va_end(va);

  /* Remove escape codes. */
  if ((AppSettings()->out != stdout && !AppSettings()->dontRemoveEscapeCodes) ||
      AppSettings()->noColors) {

    int n = 0;
    int size = strlen(cbuf);
    bool escape = false;

    for (int i = 0; i < size; ++i) {
      if (cbuf[i] == '\033') {
        escape = true;
      } else if (((cbuf[i] >= 'a' && cbuf[i] <= 'z') ||
                  (cbuf[i] >= 'A' && cbuf[i] <= 'Z') ||
                  cbuf[i] == '(' || cbuf[i] == ')') &&
                 escape) {
        escape = false;
      } else if (!escape) {
        cbuf[n] = cbuf[i];
        n += 1;
      }
    }
    cbuf[n] = '\0';
  }

#ifdef _WIN32
  if (AppSettings()->out == stdout)
    vtProcessedTextOut(cbuf, strlen(cbuf));
  else
    fprintf(AppSettings()->out, cbuf);
#else
  fprintf(AppSettings()->out, cbuf);
#endif

#endif // NO_COLORS
}

void setConsoleTitle(const char* title)
{
#if defined(_WIN32)
  SetConsoleTitleA(title);
#elif defined(_linux_)
  printf("%c]0;%s%c", '\033', title, '\007');
#endif
}

/*
vi:ts=4:et:nowrap
*/
