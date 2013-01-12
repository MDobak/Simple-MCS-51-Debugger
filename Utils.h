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

#ifndef UTILS_H
#define UTILS_H


int stricmp(const char* str1, const char* str2);
char* bytetobin(char byte);
int hextoi(char* hex, int min, int max, int def, bool* valid);
bool boolQuestion(char* input, char* trueChars, char* falseChars, bool* valid);
void msSleep(unsigned int ms);
void print(char *format, ...);
void setConsoleTitle(const char* title);

#endif // UTILS_H

/*
vi:ts=4:et:nowrap
*/
