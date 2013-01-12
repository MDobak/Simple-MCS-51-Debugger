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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include "Utils.h"
#include "MCS51.h"
#include "Debugger.h"

void version(void)
{
  print(C_PROLOG "%s %i.%i.%i (build at %s %s)\nCopyright (C) 2011 %s\n" C_RESET,
        APP_NAME_LONG, APP_MAJOR_VERSION, APP_MINOR_VERSION,
        APP_PATH_VERSION, __DATE__, __TIME__, APP_AUTHOR);
}

void help(void)
{
  version();
  puts("Usage: s51d [-hv] [-n] [-e file] [-p prompt] [-f format]\n");
  puts("Options:");
  puts("  -h --help                      Print out this help.");
  puts("  -v --version                   Print out version number.");
  puts("  --no-prolog                    Do not display prolog.");
  puts("  --no-colors                    Do not use colors.");
  puts("  -e <file> --exec <file>        Execute command from 'file'.");
  puts("  -p <prompt> --prompt <prompt>  Specify string for prompt.");
  puts("  -f <format> --format <format>  Specify string for disassemble listing.");
  puts("  -E <file> --stderr <file>      Specify file to write errors.");
  puts("  -o <file> --stdout <file>      Specify file to write output.");
  puts("  --dont-remove-escapes          Don't remove escape codes when use redirect output.\n");
  puts("  --use-stdout                   Send errors to stdout instead of stderr.\n");
  puts("  -m --mcu-type                  Type of MCU. 8031 8032 8051 8052 89S51 89S52\n");
  puts("  --irom-size                    Send errors to stdout instead of stderr.\n");
  puts("  --xrom-size                    Send errors to stdout instead of stderr.\n");
  puts("  --iram-size                    Send errors to stdout instead of stderr.\n");
  puts("  --xram-size                    Send errors to stdout instead of stderr.\n");
  puts("To display available command type help in program console.\n");
}

void cleanUp(void)
{
  removeMCU(AppSettings()->mcu);
  free(AppSettings()->mcu);

#ifndef NDEBUG
  MEMORY_STATUS();
#endif
}

int main(int argc, char* argv[])
{
  /*
   * Initialize vt100 emulator for Windows.
   */
#ifdef _WIN32
  vtInitVT100();
#endif // _WIN32

  /*
   * Set default values.
   */
  srand(time(NULL));
  bool showProlog = true;

  AppSettings()->mcu = malloc(sizeof(MCU));
  AppSettings()->mcu->noDebug = false;
  AppSettings()->format =
    C_BOLD C_FWHITE "%a: " C_NBOLD "%o " C_FGREEN "%m " C_BOLD" %p " C_RESET "\n";
  AppSettings()->commandPrompt = "cmd:>";
  AppSettings()->exitkey = 0x04; /* Ctrl + D */
  AppSettings()->lastKey = 0;
  AppSettings()->realtime = true;
  AppSettings()->keyAvailable = false;
  AppSettings()->stopThread = false;
  AppSettings()->noColors = false;
  AppSettings()->dontRemoveEscapeCodes = false;
  AppSettings()->pauseOnError = true;
  AppSettings()->simTimeBeforeStop = 0;
  AppSettings()->simSyncTimeBeforeStop = 0;
  AppSettings()->mcuSec = 0;
  AppSettings()->simSec = 0;
  AppSettings()->in = stdin;
  AppSettings()->out = stdout;
  AppSettings()->defaultIn = stdin;
  AppSettings()->defaultOut = stdout;
  AppSettings()->errorOut = stderr;
  init8052MCU(AppSettings()->mcu);
  atexit(cleanUp);

#ifndef USE_COLORS
  AppSettings()->noColors = true;
#endif

  /*
   * Read options.
   */
  int c;
  while (1) {
    static struct option long_options[] = {
      {"help",                no_argument,       0, 'h'},
      {"version",             no_argument,       0, 'v'},
      {"no-prolog",           no_argument,       0, 1000},
#ifdef USE_COLORS
      {"no-colors",           no_argument,       0, 1001},
#endif
      {"exec",                required_argument, 0, 'e'},
      {"prompt",              required_argument, 0, 'p'},
      {"format",              required_argument, 0, 'f'},
      {"stderr",              required_argument, 0, 'E'},
      {"stdout",              required_argument, 0, 'o'},
      {"dont-remove-escape",  required_argument, 0, 1002},
      {"use-stdout",          required_argument, 0, 1003},
      {"mcu-type",            required_argument, 0, 'm'},
      {"irom-size",           required_argument, 0, 1004},
      {"xrom-size",           required_argument, 0, 1005},
      {"iram-size",           required_argument, 0, 1006},
      {"xram-size",           required_argument, 0, 1007},
      {0, 0, 0, 0}
    };

    int option_index = 0;
    c = getopt_long(argc, argv, "hve:p:f:E:o:m:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {

    case 'h':
      help();
      exit(EXIT_SUCCESS);
      break;

    case 'v':
      version();
      exit(EXIT_SUCCESS);
      break;

    case 1000:
      showProlog = false;
      break;

    case 'e':
      AppSettings()->in = fopen(optarg, "r");
      if (AppSettings()->in == NULL) {
        fprintf(stderr, "Error while trying to open '%s': %s.\n", optarg,
                strerror(errno));
        AppSettings()->in = stdin;
      }
      break;

    case 'p':
      AppSettings()->commandPrompt = optarg;
      break;

#ifdef USE_COLORS
    case 1001:
      AppSettings()->noColors = true;
      break;
#endif

    case 'f':
      AppSettings()->format = optarg;
      break;

    case 'E':
      AppSettings()->errorOut = fopen(optarg, "r");
      if (AppSettings()->errorOut == NULL) {
        fprintf(stderr, "Error while trying to open '%s': %s.\n", optarg,
                strerror(errno));
        AppSettings()->errorOut = stderr;
      }
      break;

    case 'o':
      AppSettings()->defaultOut = fopen(optarg, "r");
      if (AppSettings()->defaultOut == NULL) {
        fprintf(stderr, "Error while trying to open '%s': %s.\n", optarg,
                strerror(errno));
        AppSettings()->defaultOut = stdout;
      }
      break;

    case 1002:
      AppSettings()->dontRemoveEscapeCodes = true;
      break;

    case 'm':
      if (0 == strcmp(optarg, "8031"))
        init8031MCU(AppSettings()->mcu);
      if (0 == strcmp(optarg, "8032"))
        init8032MCU(AppSettings()->mcu);
      else if (0 == strcmp(optarg, "8051"))
        init8051MCU(AppSettings()->mcu);
      else if (0 == strcmp(optarg, "8052"))
        init8052MCU(AppSettings()->mcu);
      else if (0 == strcmp(optarg, "89S51"))
        init89S51MCU(AppSettings()->mcu);
      else if (0 == strcmp(optarg, "89S52"))
        init89S52MCU(AppSettings()->mcu);
      else
        fprintf(stderr, "Invalid MCU type '%s'.\n", optarg);
      break;

    case 1003:
      AppSettings()->errorOut = stdout;
      break;

    case 1004:
      AppSettings()->mcu->iromMemorySize = hextoi(optarg, 0x0, 0x10000, 0x10000, NULL);
      break;

    case 1005:
      AppSettings()->mcu->xromMemorySize = hextoi(optarg, 0x0, 0x10000, 0x10000, NULL);
      break;

    case 1006:
      AppSettings()->mcu->idataMemorySize = hextoi(optarg, 0x10, 0x100, 0x100, NULL);
      break;

    case 1007:
      AppSettings()->mcu->xdataMemorySize = hextoi(optarg, 0x0, 0x10000, 0x10000, NULL);
      break;

    case '?':
      break;

    default:
      abort();
    }
  }

  /*
   * Prolog.
   */
  if (showProlog)
    version();

  /*
   * Debugger.
   */
  runDebugger();

  /* Go to cleanUp() before exit !!! */
  return EXIT_SUCCESS;

  /* La fin de mon monde... */
}

/*
vi:ts=4:et:nowrap
*/

