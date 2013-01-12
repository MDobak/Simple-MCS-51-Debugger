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

#ifndef DEASMTABLES_H_
#define DEASMTABLES_H_

extern const int BYTE_COUNT[];
extern const char* MNEMONIC_TABLE[];
/*
 * [0ON]%[12]
 * 0 - bit
 * O - offset
 * N - dont replace address to sfr name
 * 1 - first bit
 * 2 - second bit
 */
extern const char* MNEMONIC_PARAMS[];
extern const char* SFR_NAMES[];
extern const char* SFR_BITS[];

#endif /* DEASMTABLES_H_ */

/*
vi:ts=4:et:nowrap
*/
