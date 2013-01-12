#ifndef VT100WINDOWS_H
#define VT100WINDOWS_H

#include <windows.h>
#include <stdio.h>

BOOL WINAPI MyWriteConsoleW(
  HANDLE hConsoleOutput,
  const VOID* lpBuffer,
  DWORD nNumberOfCharsToWrite,
  LPDWORD lpNumberOfCharsWritten,
  LPVOID lpReserved);

#endif // VT100WINDOWS_H
