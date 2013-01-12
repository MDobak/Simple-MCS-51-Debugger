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

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "MCS51.h"

#define APP_NAME "S51D"
#define APP_NAME_LONG "Simple MCS51 Debugger"
#define APP_AUTHOR "Michal Dobaczewski"
#define APP_MAJOR_VERSION 0
#define APP_MINOR_VERSION 1
#define APP_PATH_VERSION 0

//#define NDEBUG
#define USE_COLORS

#ifndef NDEBUG
#include "memleaks.h"
#endif

#ifdef USE_COLORS
#define C_PROLOG "\033[0;33m"
#define C_PROMPT "\033[m"
#define C_RESET "\033[m"
#define C_FBLACK "\033[30m"
#define C_FRED "\033[31m"
#define C_FGREEN "\033[32m"
#define C_FYELLOW "\033[33m"
#define C_FBLUE "\033[34m"
#define C_FMAGNETA "\033[35m"
#define C_FCYAN "\033[36m"
#define C_FWHITE "\033[37m"
#define C_BBLACK "\033[40m"
#define C_BRED "\033[41m"
#define C_BGREEN "\033[42m"
#define C_BYELLOW "\033[43m"
#define C_BBLUE "\033[44m"
#define C_BMAGNETA "\033[45m"
#define C_BCYAN "\033[46m"
#define C_BWHITE "\033[47m"
#define C_NBOLD "\033[0m"
#define C_BOLD "\033[1m"
#else
#define C_PROLOG ""
#define C_PROMPT ""
#define C_RESET ""
#define C_FBLACK ""
#define C_FRED ""
#define C_FGREEN ""
#define C_FYELLOW ""
#define C_FBLUE ""
#define C_FMAGNETA ""
#define C_FCYAN ""
#define C_FWHITE ""
#define C_BBLACK ""
#define C_BRED ""
#define C_BGREEN ""
#define C_BYELLOW ""
#define C_BBLUE ""
#define C_BMAGNETA ""
#define C_BCYAN ""
#define C_BWHITE ""
#define C_NBOLD ""
#define C_BOLD ""
#endif

struct _settings {
  MCU* mcu;
  int lastKey;
  int exitkey;
  bool realtime;
  bool keyAvailable;
  bool stopThread;
  bool noColors;
  bool dontRemoveEscapeCodes;
  bool pauseOnError;
  double simTimeBeforeStop;
  double simSyncTimeBeforeStop;
  double mcuSec;
  double simSec;
  FILE* in;
  FILE* out;
  FILE* defaultIn;
  FILE* defaultOut;
  FILE* errorOut;
  char* commandPrompt;
  char* format;
  char* lastLoadedFile;
};

/*
 * Return pointer to structure with application settings.
 */
struct _settings* AppSettings();

#endif /* GLOBAL_H_ */

/*
vi:ts=4:et:nowrap
*/
