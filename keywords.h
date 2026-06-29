// Copyright (C) 2026 leo z <zlx20010815@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

char *keywords[] = {"LET", "PRINT", "INPUT", "IF", "THEN", "FOR", "TO",
                  	 "STEP", "NEXT", "GOTO", "GOSUB", "RETURN", "RETURN", "END",
                  	 "REM", "AND", "OR", "NOT", "DIM"};

#define KEYWORDS_SIZE sizeof(keywords) / sizeof(keywords[0])

char *relops[] = {"=", "<>", "<=", ">=", "<", ">"};

#define RELOPS_SIZE sizeof(relops) / sizeof(relops[0])