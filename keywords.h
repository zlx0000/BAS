// Copyright (C) 2026 leo z <zlx20010815@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/* This file is part of BAS.
 *
 * BAS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * BAS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * BAS. If not, see <https://www.gnu.org/licenses/>.
 */

static char *keywords[] = {"LET", "PRINT", "INPUT", "IF", "ELSE", "FI", "THEN", "FOR", "TO",
                  	 "STEP", "NEXT", "GOTO", "GOSUB", "RETURN", "RETURN", "END",
                  	 "REM", "AND", "OR", "NOT", "DIM", "PUTCHAR", "CLEAR", "HOME", "SLEEP",
					 "DELETE", "FREE", "FUN"};

#define KEYWORDS_SIZE sizeof(keywords) / sizeof(keywords[0])

static char *relops[] = {"=", "==", "<>", "!=", "<=", ">=", "<", ">"};

#define RELOPS_SIZE sizeof(relops) / sizeof(relops[0])
