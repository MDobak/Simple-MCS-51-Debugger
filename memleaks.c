/*
 * Memory Leaks Finder
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


#include <stdlib.h>
#include <stdio.h>
#include <time.h>

typedef struct {
  void* memory;
  void* original;
  char* allocInFile;
  int allocInLine;
  double allocOnTime;
  char** reallocsInFile;
  int* reallocsInLine;
  size_t* reallocsSize;
  double* reallocsOnTime;
  void** realloc;
  int numOfReallocs;
  int memorySize;
  int released;
} _allocations;

_allocations* _memoryAllocs;
int _numOfMemAllocs;

size_t _totalAalloc = 0;
size_t _maxAalloc = 0;
size_t _totalUunreleased = 0;
size_t _totalReleased = 0;


void MEMORY_STATUS()
{
  FILE* out = fopen("memory.txt", "w");
  if (out == NULL)
    out = stdout;

  fprintf(out, "-------------------------------------------------\n");
  fprintf(out, "Total allocated memory: %i bytes\n", _totalAalloc);
  fprintf(out, "Total released memory: %i bytes\n", _totalReleased);
  fprintf(out, "Unreleased memory: %i bytes\n", _totalAalloc - _totalReleased);
  fprintf(out, "Maximum allocated memory: %i bytes\n", _maxAalloc);

  fprintf(out, "-------------------------------------------------\n");
  fprintf(out, "Unreleased memory\n");
  fprintf(out, "-------------------------------------------------\n");
  for (int i = 0; i < _numOfMemAllocs; ++i) {
    if (_memoryAllocs[i].released == 0) {
      fprintf(out, "  * %s:%i (allocated %i bytes, run time: %.2f)\n",
              _memoryAllocs[i].allocInFile,
              _memoryAllocs[i].allocInLine,
              _memoryAllocs[i].memorySize,
              _memoryAllocs[i].allocOnTime);

      char* READ_TEST = _memoryAllocs[i].memory;
      READ_TEST += 1;

      if (_memoryAllocs[i].numOfReallocs > 0)
        fprintf(out, "    * Reallocations\n");

      for (int j = 0; j < _memoryAllocs[i].numOfReallocs; ++j)
        fprintf(out, "      %s:%i (resize to %u bytes, run time: %.2f)\n",
                _memoryAllocs[i].reallocsInFile[j],
                _memoryAllocs[i].reallocsInLine[j],
                _memoryAllocs[i].reallocsSize[j],
                _memoryAllocs[i].reallocsOnTime[j]);
    }
  }
  fclose(out);
}

void* __wrapper_malloc(size_t size, int line, char* file)
{
  _memoryAllocs = (_allocations*)realloc(_memoryAllocs, sizeof(_allocations) * (_numOfMemAllocs + 1));

  _memoryAllocs[_numOfMemAllocs].memory = malloc(size);
  _memoryAllocs[_numOfMemAllocs].original = _memoryAllocs[_numOfMemAllocs].memory;
  _memoryAllocs[_numOfMemAllocs].allocInFile = file;
  _memoryAllocs[_numOfMemAllocs].allocInLine = line;
  _memoryAllocs[_numOfMemAllocs].allocOnTime = (double)clock() / CLOCKS_PER_SEC;
  _memoryAllocs[_numOfMemAllocs].reallocsInFile = NULL;
  _memoryAllocs[_numOfMemAllocs].reallocsInLine = NULL;
  _memoryAllocs[_numOfMemAllocs].reallocsSize = NULL;
  _memoryAllocs[_numOfMemAllocs].reallocsOnTime = NULL;
  _memoryAllocs[_numOfMemAllocs].realloc = NULL;
  _memoryAllocs[_numOfMemAllocs].numOfReallocs = 0;
  _memoryAllocs[_numOfMemAllocs].memorySize = size;
  _memoryAllocs[_numOfMemAllocs].released = 0;

  _numOfMemAllocs += 1;
  _totalAalloc += size;

  return _memoryAllocs[_numOfMemAllocs - 1].memory;
}

void  __wrapper_free(void *ptr, int line, char* file)
{
  if (ptr == NULL)
    return;

  for (int i = 0; i < _numOfMemAllocs; ++i) {
    if (_memoryAllocs[i].memory == ptr && !_memoryAllocs[i].released) {
      _memoryAllocs[i].released = 1;
      _totalReleased += _memoryAllocs[i].memorySize;

      size_t totalUunreleased = 0;
      for (int i = 0; i < _numOfMemAllocs; ++i)
        if (_memoryAllocs[i].released == 0)
          totalUunreleased += _memoryAllocs[i].memorySize;

      if (totalUunreleased > _maxAalloc)
        _maxAalloc = totalUunreleased;

      free(ptr);
      return;
    }
  }

  printf("\nWARNING! FREE USING UNDEF PTR\n%s:%i\n", file, line);
  free(ptr);
}

void* __wrapper_realloc(void *ptr, size_t size, int line, char* file)
{
  if (ptr == NULL)
    return __wrapper_malloc(size, line, file);

  for (int i = 0; i < _numOfMemAllocs; ++i) {
    if (_memoryAllocs[i].memory == ptr && !_memoryAllocs[i].released) {
      _memoryAllocs[i].memory = realloc(ptr, size);

      _memoryAllocs[i].reallocsInFile =
        (char**)realloc(_memoryAllocs[i].reallocsInFile,
                        (_memoryAllocs[i].numOfReallocs + 1) * sizeof(char*));

      _memoryAllocs[i].reallocsInLine =
        (int*)realloc(_memoryAllocs[i].reallocsInLine,
                      (_memoryAllocs[i].numOfReallocs + 1) * sizeof(int));

      _memoryAllocs[i].reallocsSize =
        (size_t*)realloc(_memoryAllocs[i].reallocsSize,
                         (_memoryAllocs[i].numOfReallocs + 1) * sizeof(size_t));

      _memoryAllocs[i].reallocsOnTime =
        (double*)realloc(_memoryAllocs[i].reallocsOnTime,
                         (_memoryAllocs[i].numOfReallocs + 1) * sizeof(double));

      _memoryAllocs[i].realloc =
        (void**)realloc(_memoryAllocs[i].realloc,
                        (_memoryAllocs[i].numOfReallocs + 1) * sizeof(void**));

      _memoryAllocs[i].reallocsInFile[_memoryAllocs[i].numOfReallocs] = file;
      _memoryAllocs[i].reallocsInLine[_memoryAllocs[i].numOfReallocs] = line;
      _memoryAllocs[i].reallocsSize[_memoryAllocs[i].numOfReallocs] = size;
      _memoryAllocs[i].reallocsOnTime[_memoryAllocs[i].numOfReallocs] = (double)clock() / CLOCKS_PER_SEC;
      _memoryAllocs[i].realloc[_memoryAllocs[i].numOfReallocs] = _memoryAllocs[i].memory;

      _memoryAllocs[i].numOfReallocs += 1;

      _totalAalloc += size - _memoryAllocs[i].memorySize;
      _memoryAllocs[i].memorySize = size;

      return _memoryAllocs[i].memory;
    }
  }

  printf("\nWARNING! REALLOC USING UNDEF PTR\n%s:%i\n", file, line);
  return NULL;
}
