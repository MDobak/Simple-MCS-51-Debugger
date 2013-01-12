/* http://www.gnu.org/copyleft/gpl.html */
// Code by gynvael.coldwind//vx
// !!!Use at your own risk!!!
#include <windows.h>
#include <stdio.h>

BOOL WINAPI MyWriteConsoleW(
  HANDLE hConsoleOutput,
  const VOID* lpBuffer,
  DWORD nNumberOfCharsToWrite,
  LPDWORD lpNumberOfCharsWritten,
  LPVOID lpReserved)
{
  DWORD i;
  const WORD *p = (const WORD*)lpBuffer;
  DWORD from = 0;
  BOOL SuspendOutput = FALSE;

  CONSOLE_SCREEN_BUFFER_INFO info;
  GetConsoleScreenBufferInfo(hConsoleOutput, &info);

  for(i = 0; i < nNumberOfCharsToWrite; i++) {
    if(p[i] == 27 && i + 2 < nNumberOfCharsToWrite && p[i+1] == '[') {
      // Some Ansi Escape Code was detected...
      // First, let's check if this is a valid opcode...
      // It it is, then there should be an ansi opcode char after 0-7 chars
      //
      // Supported ansi opcodes:
      // m - color
      // H - gotoyx
      // f - gotoyx
      // A - up # lines
      // B - down
      // C - right
      // D - left
      // s - save for recall
      // u - return to saved
      // J - cls (2J)
      // K - clear to end of line
      //
      // Gynvael's ansi extensions:
      // X - push current color on stack
      // x - pop color from stack
      // Y - save current color at index
      // y - retrive color from index
      // t - start of title
      // T - end of title
      DWORD Start_i = i;
      DWORD j;
      BOOL SupportedOpcodeFound = FALSE;
      WORD OpcodeFound = 0;

      for(j = i + 2; j < nNumberOfCharsToWrite && j < i + 2 + 8; j++) {
        switch(p[j]) {
        case 'm':
        case 'H':
        case 'f':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 's':
        case 'u':
        case 'J':
        case 'K':
        case 'x':
        case 'X':
        case 'y':
        case 'Y':
        case 't':
        case 'T':
          SupportedOpcodeFound = TRUE;
          OpcodeFound = p[j];
          break;
        }

        if(SupportedOpcodeFound)
          break;
      }

      if(!SupportedOpcodeFound)
        continue;

      WORD ArgsStart = i + 2;

      // Output stuff from buffer
      if(!SuspendOutput)
        WriteConsoleW(hConsoleOutput, &p[from], Start_i - from, lpNumberOfCharsWritten, lpReserved);

      // Set iterators
      from = j + 1;
      i = j;

      // Handle ansi codes
      switch(OpcodeFound) {
      case 't': // ESC[t TITLE ESC[T  set console TITLE
      case 'T': {
        static DWORD Pos;
        if(OpcodeFound == 't') {
          Pos = from;
          SuspendOutput = TRUE;
          break;
        }

        SuspendOutput = FALSE;

        DWORD Length = Start_i - Pos;
        static WORD Buffer[1024];

        if(Length > sizeof(Buffer) / sizeof(*Buffer))
          Length = sizeof(Buffer) / sizeof(*Buffer) - 1;

        Buffer[Length] = 0;
        memcpy(Buffer, &p[Pos], Length * 2);
        SetConsoleTitleW(Buffer);
        break;
      }

      case 'Y': // ESC[Y   save current color at index
      case 'y': { // ESC[y   retrive color from index
        static WORD ColorArray[1024];

        int Ret, Pos;
        Ret = swscanf(&p[ArgsStart], L"%i", &Pos);
        if(Ret != 1)
          break;

        if(Pos < 0 || Pos >= 1024)
          break;

        if(OpcodeFound == 'Y') {
          // save
          CONSOLE_SCREEN_BUFFER_INFO info;
          GetConsoleScreenBufferInfo(hConsoleOutput, &info);
          ColorArray[Pos] = info.wAttributes;
        } else {
          // get
          SetConsoleTextAttribute(hConsoleOutput, ColorArray[Pos]);
        }
      }
      break;

      case 'X': // ESC[X   push current color on stack
      case 'x': { // ESC[x   pop color from stack
        static WORD ColorStack[1024];
        static WORD ColorPtr = 0;

        // Archive original setting
        if(ColorPtr == 0) {
          CONSOLE_SCREEN_BUFFER_INFO info;
          GetConsoleScreenBufferInfo(hConsoleOutput, &info);
          ColorStack[ColorPtr++] = info.wAttributes;
        }

        // Handle stack
        if(OpcodeFound == 'x') {
          // Pop
          if(ColorPtr > 1)
            SetConsoleTextAttribute(hConsoleOutput, ColorStack[--ColorPtr]);
          else
            SetConsoleTextAttribute(hConsoleOutput, ColorStack[0]);
        } else if(ColorPtr < 1024) {
          // Push
          CONSOLE_SCREEN_BUFFER_INFO info;
          GetConsoleScreenBufferInfo(hConsoleOutput, &info);
          ColorStack[ColorPtr++] = info.wAttributes;
        }
      }
      break;

      case 'm': { // ESC[#(;#)m   color and text formatting
        static WORD FirstColor;
        static BOOL FirstColorSet;

        int Ret, arg[3];
        Ret = swscanf(&p[ArgsStart], L"%i;%i;%i", &arg[0], &arg[1], &arg[2]);

        // Archive original setting
        if(!FirstColorSet) {
          CONSOLE_SCREEN_BUFFER_INFO info;
          GetConsoleScreenBufferInfo(hConsoleOutput, &info);
          FirstColor = info.wAttributes;
          FirstColorSet = TRUE;
        }

        // Setup colors
        if(Ret == 0) {
          SetConsoleTextAttribute(hConsoleOutput, FirstColor);
        } else {
          CONSOLE_SCREEN_BUFFER_INFO info;
          GetConsoleScreenBufferInfo(hConsoleOutput, &info);

          // Set new attributes
          WORD NewAttr = info.wAttributes;
          int k;
          for(k = 0; k < Ret; k++) {
            if(arg[k] >= 30 && arg[k] <= 37) {
              NewAttr &= ~(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
              BYTE ByteArg = arg[k] - 30;
              ByteArg = (ByteArg >> 2) | (ByteArg & 2) | ((ByteArg << 2) & 4); // Swap
              NewAttr |= ByteArg;
            } else if(arg[k] >= 40 && arg[k] <= 47) {
              NewAttr &= ~(BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);
              BYTE ByteArg = arg[k] - 40;
              ByteArg = (ByteArg >> 2) | (ByteArg & 2) | ((ByteArg << 2) & 4); // Swap
              NewAttr |= ByteArg << 4;
            } else if(arg[k] >= 0 && arg[k] <= 8) {
              NewAttr &= ~(FOREGROUND_INTENSITY);
              if(arg[k] == 1)
                NewAttr |= FOREGROUND_INTENSITY;
            }
          }

          SetConsoleTextAttribute(hConsoleOutput, NewAttr);
        }
      }
      break;

      case 'H': // ESC[#;#H or ESC[#;#f   moves cursor to line #, column #
      case 'f': {
        int Ret, x, y;

        Ret = swscanf(&p[ArgsStart], L"%i;%i", &y, &x);
        if(Ret == 0) {
          COORD c = { 0, 0 };
          SetConsoleCursorPosition(hConsoleOutput, c);
          break;
        }

        if(Ret != 2) break;

        x--, y--;

        COORD c = { x, y };
        SetConsoleCursorPosition(hConsoleOutput, c);
      }
      break;

      case 'A': // ESC[#A   moves cursor up # lines
      case 'B': // ESC[#B   moves cursor down # lines
      case 'C': // ESC[#C   moves cursor right # spaces
      case 'D': { // ESC[#D   moves cursor left # spaces
        int Ret, chg;

        Ret = swscanf(&p[ArgsStart], L"%i", &chg);
        if(Ret != 1) break;

        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(hConsoleOutput, &info);

        switch(OpcodeFound) {
        case 'A':
          info.dwCursorPosition.Y -= chg;
          break;
        case 'B':
          info.dwCursorPosition.Y += chg;
          break;
        case 'C':
          info.dwCursorPosition.X += chg;
          break;
        case 'D':
          info.dwCursorPosition.X -= chg;
          break;
        }

        SetConsoleCursorPosition(hConsoleOutput, info.dwCursorPosition);
      }
      break;

      case 's': // ESC[s   save cursor position for recall later
      case 'u': { // ESC[u   Return to saved cursor position
        static COORD CoordStack[1024];
        static int CoordPtr = 0;

        if(OpcodeFound == 's') {
          // Push
          if(CoordPtr < 1024) {
            CONSOLE_SCREEN_BUFFER_INFO info;
            GetConsoleScreenBufferInfo(hConsoleOutput, &info);

            CoordStack[CoordPtr].X = info.dwCursorPosition.X;
            CoordStack[CoordPtr].Y = info.dwCursorPosition.Y;
            CoordPtr++;
          }
        } else {
          // Pop
          if(CoordPtr > 0) {
            CoordPtr--;
            SetConsoleCursorPosition(hConsoleOutput, CoordStack[CoordPtr]);
          }
        }
      }
      break;

      case 'J': { // ESC[2J   clear screen and home cursor
        if(p[ArgsStart] != '2')
          break;

        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(hConsoleOutput, &info);

        DWORD Written;
        DWORD Size = info.dwSize.X * info.dwSize.Y;
        COORD c = { 0, 0 };

        FillConsoleOutputAttribute(hConsoleOutput, info.wAttributes, Size, c, &Written);
        FillConsoleOutputCharacter(hConsoleOutput, ' ', Size, c, &Written);
        SetConsoleCursorPosition(hConsoleOutput, c);
      }
      break;

      case 'K': { // ESC[K   clear to end of line
        COORD c;
        DWORD Written;
        CONSOLE_SCREEN_BUFFER_INFO info;

        GetConsoleScreenBufferInfo(hConsoleOutput, &info);
        c.X = info.dwCursorPosition.X;
        c.Y = info.dwCursorPosition.Y;
        FillConsoleOutputCharacter(hConsoleOutput, ' ', info.dwSize.X - c.X, c, &Written);
        SetConsoleCursorPosition(hConsoleOutput, c);
      }
      break;
      }
    }
  }

  // Flush rest of the text
  if(from < i)
    WriteConsoleW(hConsoleOutput, &p[from], i - from, lpNumberOfCharsWritten, lpReserved);

  // Spoof version string ;>
  static WORD HackMeStringXP[]    = L"Microsoft Windows XP [";
  static WORD HackMeStringVista[] = L"Microsoft Windows [";
  if(memcmp(lpBuffer, HackMeStringXP,    22*2) == 0 ||
      memcmp(lpBuffer, HackMeStringVista, 19*2) == 0) {
    DWORD temp;
    WriteConsoleW(hConsoleOutput,
                  L"\nAnsi hack ver 0.004b by gynvael.coldwind//vx",
                  45, &temp, NULL);
  }

  // Everything was OK...
  *lpNumberOfCharsWritten = nNumberOfCharsToWrite;
  return TRUE;
}
