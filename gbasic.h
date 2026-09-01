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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <ctype.h>

#define MAX_TOKEN 25600
#define BFSIZE 256

#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_ALPHA(c)                                                              \
	(((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_')
#define IS_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r')
#define IS_CHAR(c) (IS_ALPHA(c) || IS_DIGIT(c) || (c) == '_')
#define IS_QUOT(c) ((c) == '"' || (c) == '\'')
#define IS_PAREN(c) ((c) == '(' || (c) == ')')
#define IS_SYMBOL(c)                                                             \
	((c) == '+' || (c) == '-' || (c) == '*' || (c) == '/' || (c) == '^' ||       \
	(c) == '=' || (c) == '<' || (c) == '>' || (c) == '&' || (c) == '|' ||        \
	(c) == '$' || (c) == '%' || (c) == '.' || (c) == ',' || (c) == ';' ||        \
	IS_QUOT(c) || IS_PAREN(c))
#define IS_OP(c)                                                                 \
	((c) == '+' || (c) == '-' || (c) == '*' || (c) == '/' || (c) == '%')

#define IS_EOL(c) ((c) == '\0')

# define __unlikely(cond)	__builtin_expect ((cond), 0)
# define __likely(cond)	__builtin_expect ((cond), 1)

static char *keywords[] = {"LET", "PRINT", "INPUT", "IF", "ELSE", "FI", "THEN", "FOR", "TO",
                  	 "STEP", "NEXT", "GOTO", "GOSUB", "RETURN", "RETURN", "END",
                  	 "REM", "AND", "OR", "NOT", "DIM", "PUTCHAR", "CLEAR", "HOME", "SLEEP",
					 "DELETE", "FREE", "FUN", "ENDFUN"};

#define KEYWORDS_SIZE sizeof(keywords) / sizeof(keywords[0])

static char *relops[] = {"=", "==", "<>", "!=", "<=", ">=", "<", ">"};

#define RELOPS_SIZE sizeof(relops) / sizeof(relops[0])

typedef enum TokenType { //the order of which reflects the precedence.
	TOKEN_TYPE_NULL = 0,
    STRING_TOKEN,
    KEYWORD_TOKEN,
    RELOP_TOKEN,
    OP_TOKEN,
    FLOAT_TOKEN,
    INT_TOKEN,
	PAREN_TOKEN,
    IDENT_TOKEN,
	COMMA_TOKEN,
	SEMICOLON_TOKEN,
    SPACE_TOKEN,
    TOKEN_TYPE_END
} TokenType;

typedef enum NodeType {
	NODE_TYPE_NULL = 0,
	ROOT,
	EXPR,
	OR_EXPR,
	AND_EXPR,
	REL_EXPR,
	ADD_EXPR,
	MUL_EXPR,
	UNARY,
	PRIMARY,
	LINENUM,
	INTEGER,
	FLOAT,
	INTEGERIDENT,
	FLOATIDENT,
	STRING,
	IDENTI,
	REM,
	LET,
	DIM,
	IF,
	THEN,
	ELSE,
	FI,
	PRINT,
	PRINT_LIST,
	PUTCHAR,
	CLEAR,
	HOME,
	INPUT,
	INPUT_LIST,
	SLEEP,
	DEL,
	FREE,
	FOR,
	TO,
	STEP,
	NEXT,
	GOTO,
	GOSUB,
	RETURN,
	END,
	PLUS,
	MINUS,
	STAR,
	SLASH,
	CARET,
	EQUALS,
	LT,
	GT,
	LE,
	GE,
	NE,
	EQ,
	AND_OP,
	OR_OP,
	ADD_OP,
	MUL_OP,
	UNARY_OP,
	NOT,
	COMMA,
	SEMICOLON,
	COLON,
	LPAREN,
	RPAREN
} NodeType;

typedef union Literal {
	double floatValue;
	int intValue;
	char string[BFSIZE];
	//int linenum;
} Literal;

typedef struct {
	TokenType type;
	char lexeme[BFSIZE];
	Literal literal;
	int lineNum;
	int colNum;
} Token;

typedef struct ParseTreeNode {
	Token *token;
	NodeType type;
	unsigned int childCount;
	struct ParseTreeNode **children;
} ParseTreeNode;

typedef struct ParserContext {
	Token *tokens;
	int tokenLen;
	int len;
	//Program *prog;
	// char token[64];
	Token *tokenPtr;
	bool err;
} ParserContext;

int lexer(char *bf, Token *tokens, int lineNum);
void parse(ParserContext *context);


void free_tree(ParseTreeNode *node);
// advance the token pointer by one after successfully parsing a token
ParseTreeNode *parseLine(ParserContext *context);
ParseTreeNode *parseLinenum(ParserContext *context);
ParseTreeNode *parseIntegerLiteral(ParserContext *context);
ParseTreeNode *parseFloatLiteral(ParserContext *context);
ParseTreeNode *parseStringLiteral(ParserContext *context);
//ParseTreeNode *parseDigit(ParserContext *context);
ParseTreeNode *parseStatement(ParserContext *context);
ParseTreeNode *parseLetStatement(ParserContext *context);
ParseTreeNode *parseExpr(ParserContext *context);
ParseTreeNode *parseIfStatement(ParserContext *context);
ParseTreeNode *parseElseStatement(ParserContext *context);
ParseTreeNode *parseFiStatement(ParserContext *context);
ParseTreeNode *parseForStatement(ParserContext *context);
ParseTreeNode *parseForTail(ParserContext *context);
ParseTreeNode *parseGoSubStatement(ParserContext *context);
ParseTreeNode *parseReturnStatement(ParserContext *context);
ParseTreeNode *parseGotoStatement(ParserContext *context);
ParseTreeNode *parsePrintStatement(ParserContext *context);
ParseTreeNode *parsePutCharStatement(ParserContext *context);
ParseTreeNode *parseClearStatement(ParserContext *context);
ParseTreeNode *parseHomeStatement(ParserContext *context);
ParseTreeNode *parseSleepStatement(ParserContext *context);
ParseTreeNode *parseDimStatement(ParserContext *context);
ParseTreeNode *parseDelStatement(ParserContext *context);
ParseTreeNode *parseFreeStatement(ParserContext *context);
ParseTreeNode *parsePrintList(ParserContext *context);
ParseTreeNode *parsePrintItem(ParserContext *context);
ParseTreeNode *parseInputStatement(ParserContext *context);
ParseTreeNode *parseInputList(ParserContext *context);
ParseTreeNode *parseIdentifier(ParserContext *context);
ParseTreeNode *parseEqual(ParserContext *context);
ParseTreeNode *parseTerm(ParserContext *context);
void *parseEOL(ParserContext *context);
ParseTreeNode *parseOrExpr(ParserContext *context);
ParseTreeNode *parseOrOperand(ParserContext *context);
ParseTreeNode *parseAndExpr(ParserContext *context);
ParseTreeNode *parseAndOperand(ParserContext *context);
ParseTreeNode *parseRelExpr(ParserContext *context);
ParseTreeNode *parseAddExpr(ParserContext *context);
ParseTreeNode *parseAddOperand(ParserContext *context);
ParseTreeNode *parseMulExpr(ParserContext *context);
ParseTreeNode *parseUnary(ParserContext *context);
ParseTreeNode *parsePrimary(ParserContext *context);
ParseTreeNode *parseMulOperand(ParserContext *context);
ParseTreeNode *parseRelOperator(ParserContext *context);
ParseTreeNode *parseUnaryOperand(ParserContext *context);


#define STASK_SIZE 256

typedef struct {
	char *str;
	size_t refcnt;
} String;

typedef struct Value {
    enum ValueType {
        ERR_VAL,
		LINENUM_VAL,
		IDENTI_VAL,
		SUB_CTX,
		FOR_CTX,
		IF_FRAME,
		BOOL_VAL,
        INT_VAL,
        FLOAT_VAL,
        STRING_VAL,
		ARR_VAL,
		POINTER_VAL
	} type;
    union ValueVal {
		bool boolVal;
        int intVal;
        float floatVal;
        String string;
		struct Array {
			struct Value *ptr;
			bool hasAlias;
			char** alias;
			int size;
			size_t refcnt;
		} arr;
		int lineNum;
		struct Value *pointer;
		char *identi;
		struct SubCtx {
			int lineNum;
		} subctx;
		struct ForCtx {
			char *identi;
			int pc;
			int next;
			union {
				int intStep;
				float floatStep;
			} step;
		} forCtx;
		struct IfFrame {
			int entry;
			enum If_State {
				IF_STATE_NULL,
				IF_BEFORE,
				IF_SKIP_ELSE,
				IF_EXPECTING_ELSE_OR_FI,
				IF_EXPECTING_FI,
				IF_CONT
			} state;
		} ifFrame;
		enum ErrVal {
			ERR_VAL_NULL,
			UNKNOWN_STATEMENT,
			VAR_NOT_FOUND,
			VAR_ALREADY_EXIST,
			STACK_OVERFLOW,
			STACK_UNDERFLOW,
			UNKNOWN_BOOL_VALUE,
			INCOMPATIBLE_TYPES,
			DIVIDE_BY_ZERO,
			LINENUM_NOT_FOUND,
			INDEX_OUT_OF_RANGE,
			OUT_OF_MEMORY,
			UNINIT_VAL,
			ERR_VAR_END
		} errVal;
    } value;
} Value;

typedef struct Stack {
    int size;
    Value st[STASK_SIZE];
} Stack;

typedef struct Variable {
    char *name;
    Value val;
} Variable;

typedef struct VarListNode {
	Variable var;
	struct VarListNode *next;
} VarListNode;

typedef struct Program {
	int lineCount;
	ParseTreeNode **lines;
	Stack *shadow_st;
	struct Function {
		VarListNode *local;
		int local_size;
		int lineCount;
		ParseTreeNode **lines;
		Stack *shadow_st;
	} *functions;
} Program;

typedef struct ArrPtrList {
	Value *ptr;
	size_t size;
	bool isReachable;
	struct ArrPtrList *next;
} ArrPtrList;

extern int pc;
extern int expectedLineNum;
extern Program prog;
extern enum If_State if_state;
extern Stack if_st;
extern Stack shadow_st;

int lineNum_to_pc(int lineNum);
bool is_if_else_or_fi(ParseTreeNode *node);
void copy_stack(Stack *src, Stack* dst);

void init_eval();
Value evalLine(ParseTreeNode *node);
Value evalExpr(ParseTreeNode *node);
Value evalPrint(ParseTreeNode *node);
Value evalPutChar(ParseTreeNode *node);
Value evalDim(ParseTreeNode *node);
Value evalLet(ParseTreeNode *node);
Value evalIf(ParseTreeNode *node);
Value evalElse(ParseTreeNode *node);
Value evalFi(ParseTreeNode *node);
Value evalFor(ParseTreeNode *node);
Value evalNext(ParseTreeNode *node);
Value evalGoto(ParseTreeNode *node);
Value evalSleep(ParseTreeNode *node);
Value evalClear(ParseTreeNode *node);
Value evalHome(ParseTreeNode *node);
Value evalFree(ParseTreeNode *node);
Value evalDel(ParseTreeNode *node);
Value evalOrExpr(ParseTreeNode *node);
Value evalAndExpr(ParseTreeNode *node);
Value evalRelExpr(ParseTreeNode *node);
Value evalAddExpr(ParseTreeNode *node);
Value evalMulExpr(ParseTreeNode *node);
Value evalUnary(ParseTreeNode *node);
Value evalPrimary(ParseTreeNode *node);
