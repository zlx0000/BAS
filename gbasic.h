// Copyright (C) 2026 leo z <zlx20010815@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
	((c) == '+' || (c) == '-' || (c) == '*' || (c) == '/')

#define IS_EOL(c) ((c) == '\0')

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
	IF,
	THEN,
	PRINT,
	PRINT_LIST,
	INPUT,
	INPUT_LIST,
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
	int linenum;
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
	struct ParseTreeNode *children[128];
} ParseTreeNode;

typedef struct Program {
	int lineCount;
	ParseTreeNode **lines;
} Program;

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
ParseTreeNode *parseForStatement(ParserContext *context);
ParseTreeNode *parseForTail(ParserContext *context);
ParseTreeNode *parseGoSubStatement(ParserContext *context);
ParseTreeNode *parseReturnStatement(ParserContext *context);
ParseTreeNode *parseGotoStatement(ParserContext *context);
ParseTreeNode *parsePrintStatement(ParserContext *context);
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


#define STASK_SIZE 8192

typedef struct {
	char *str;
	size_t refcnt;
} String;

typedef struct Value {
    enum ValueType {
        ERR_VAL,
		LINENUM_VAL,
		BOOL_VAL,
        INT_VAL,
        FLOAT_VAL,
        STRING_VAL
    } type;
    union ValueVal {
		bool boolVal;
        int intVal;
        float floatVal;
        String string;
		enum ErrVal {
			ERR_VAL_NULL,
			UNKNOWN_STATEMENT,
			VAR_NOT_FOUND,
			VAR_ALREADY_EXIST,
			STACK_OVERFLOW,
			UNKNOWN_BOOL_VALUE,
			INCOMPATIBLE_TYPES,
			ERR_VAR_END
		} errVal;
    } value;
} Value;

typedef struct Stack {
    size_t size;
    Value st[STASK_SIZE];
} Stack;

typedef struct Variable {
    char name[BFSIZE];
    Value val;
} Variable;

typedef struct VarListNode {
	Variable var;
	struct VarListNode *next;
} VarListNode;

void init_eval();
Value evalLine(ParseTreeNode node);
Value evalExpr(ParseTreeNode node);
Value evalPrint(ParseTreeNode node);
Value evalLet(ParseTreeNode node);
Value evalOrExpr(ParseTreeNode node);
Value evalAndExpr(ParseTreeNode node);
Value evalRelExpr(ParseTreeNode node);
Value evalAddExpr(ParseTreeNode node);
Value evalMulExpr(ParseTreeNode node);
Value evalUnary(ParseTreeNode node);
Value evalPrimary(ParseTreeNode node);