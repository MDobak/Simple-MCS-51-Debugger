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

#ifndef MEMLEAKS_H
#define MEMLEAKS_H

#include <stdlib.h>

#define malloc(size) __wrapper_malloc(size, __LINE__, __FILE__)
#define calloc(nmeb, size) __wrapper_malloc((nmeb) * (size), __LINE__, __FILE__)
#define free(ptr) __wrapper_free(ptr, __LINE__, __FILE__)
#define realloc(ptr, size) __wrapper_realloc(ptr, size, __LINE__, __FILE__);

void MEMORY_STATUS();

void* __wrapper_malloc(size_t size, int line, char* file);
void  __wrapper_free(void *ptr, int line, char* file);
void* __wrapper_realloc(void *ptr, size_t size, int line, char* file);

#endif // MEMLEAKS_H
