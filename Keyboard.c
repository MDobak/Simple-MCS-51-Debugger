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
#include "Keyboard.h"

#if defined(__linux__)
#include <termios.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <conio.h>
#else
#include <stdio.h>
#endif

#if defined(__linux__)
static struct termios initial_settings, new_settings;
static int peek_character = -1;
#endif

void initKeyboard()
{
#if defined(__linux__)
  tcgetattr(0,&initial_settings);
  new_settings = initial_settings;
  new_settings.c_lflag &= ~ICANON;
  new_settings.c_lflag &= ~ECHO;
  new_settings.c_lflag &= ~ISIG;
  new_settings.c_cc[VMIN] = 1;
  new_settings.c_cc[VTIME] = 0;
  tcsetattr(0, TCSANOW, &new_settings);
#endif // __linux__
}

void closeKeyboard()
{
#if defined(__linux__)
  tcsetattr(0, TCSANOW, &initial_settings);
#endif // __linux__
}

int keyboardHit()
{
#if defined(__linux__)
  unsigned char ch;
  int nread;

  if (peek_character != -1) return 1;
  new_settings.c_cc[VMIN]=0;
  tcsetattr(0, TCSANOW, &new_settings);
  nread = read(0, &ch, 1);
  new_settings.c_cc[VMIN] = 1;
  tcsetattr(0, TCSANOW, &new_settings);
  if(nread == 1) {
    peek_character = ch;
    return 1;
  }
  return 0;
#elif defined(_WIN32)
  return kbhit();
#else
  return 0;
#endif
}

/*
 * Opis w Keyboard.h
 */
int getKey()
{
#if defined(__linux__)
  char ch;

  if(peek_character != -1) {
    ch = peek_character;
    peek_character = -1;
    return ch;
  }

  read(0, &ch, 1);
  return ch;
#elif defined(__WIN32)
  return getch();
#else
  int in;
  scanf("%c", &in);
  return in;
#endif
}

/*
vi:ts=4:et:nowrap
*/
