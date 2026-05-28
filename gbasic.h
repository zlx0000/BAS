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

typedef enum { //the order of which reflects the precedence.
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

typedef enum {
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

typedef union {
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

typedef struct {
	int lineCount;
	ParseTreeNode **lines;
} Program;

typedef struct {
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


#define STASK_SIZE 1024

typedef struct {
    enum {
        ERR_VAL,
		LINENUM_VAL,
		BOOL_VAL,
        INT_VAL,
        FLOAT_VAL,
        STRING_VAL
    } type;
    union {
		bool boolVal;
        int intVal;
        float floatVal;
        char stringVal[BFSIZE];
    } value;
	unsigned int refcnt;
} Value;

typedef struct {
    size_t size;
    Value st[STASK_SIZE];
} Stack;

typedef struct {
    char name[BFSIZE];
    Value val;
} Variable;

Value evalLine(ParseTreeNode node);
Value evalExpr(ParseTreeNode node);
Value evalPrint(ParseTreeNode node);
Value evalOrExpr(ParseTreeNode node);
Value evalAndExpr(ParseTreeNode node);
Value evalRelExpr(ParseTreeNode node);
Value evalAddExpr(ParseTreeNode node);