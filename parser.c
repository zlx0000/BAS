// Copyright (C) 2026 leo z <zlx20010815@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gbasic.h"

#define ERR(str) {if (!IN_RANGE) {fprintf(stderr,\
		 "token stream ended prematurely\n");\
		free(node);\
		return NULL;}fprintf(stderr, "%s at %d,%d: `%s`,\n",\
		str,context->tokenPtr->lineNum, context->tokenPtr->colNum,                  \
		context->tokenPtr->lexeme); context->err = true ;return NULL;}
#define ERR_NOFREE(str) {if (!IN_RANGE) {fprintf(stderr,\
		 "token stream ended prematurely\n");\
		return NULL;}fprintf(stderr, "%s at %d,%d: `%s`,\n",\
		str,context->tokenPtr->lineNum, context->tokenPtr->colNum,                  \
		context->tokenPtr->lexeme); context->err = true ;return NULL;}
#define IN_RANGE (context->len < context->tokenLen)
#define LAST_TOKEN (context->len == context->tokenLen-1)
#define ERR_RET(n) {if (!n) {context->err=true;free(node);return NULL;}}
#define ERR_RET_NOFREE(n) {if (!n) {context->err=true;return NULL;}}
#define CONSUME_TOKEN {if (!IN_RANGE) \
		{ERR("Token stream ended prematurely")}; context->tokenPtr++; \
		context->len++;}
#define CONSUME_TOKEN_NOFREE {if (!IN_RANGE) \
		{ERR_NOFREE("Token stream ended prematurely")}; context->tokenPtr++; \
		context->len++;}

/*
void parse(ParserContext *context)
{
	if (context->tokenPtr == NULL || context->prog == NULL) {
		fprintf(stderr, "Parser context is not initialized.\n");
		return;
	}
	int lineNum = 0;
	while (context->tokenPtr != NULL) {
		context->prog->lines[lineNum] = parseLine(context);
		lineNum++;
	}
}
*/

void free_tree(ParseTreeNode *node)
{
	for (int i = 0; i < node->childCount; i++) {
		free_tree(node->children[i]);
	}
	free(node->children);
	free(node);
}

ParseTreeNode *parseLine(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = ROOT;
	int i = 0;
	if (IN_RANGE && context->tokenPtr->type == INT_TOKEN) {
		node->children = malloc(sizeof(uintptr_t));
		node->children[i] = parseLinenum(context);
		ERR_RET(node->children[i]);
		node->childCount++;
		i++;
	}
	node->children = realloc(node->children, (i+1)*sizeof(uintptr_t));
	node->children[i] = parseStatement(context);
	ERR_RET(node->children[i]);
	node->childCount++;
	if (IN_RANGE) {
		fprintf(stderr, "%s at %d,%d: `%s`\n",
				"Unexpected token",
				context->tokenPtr->lineNum, 
				context->tokenPtr->colNum,
				context->tokenPtr->lexeme);
		context->err = true;
	}
	//parseEOL(context);
	return node;
}

ParseTreeNode *parseIdentifier(ParserContext *context)
{
	ParseTreeNode *node =
			(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (IN_RANGE && context->tokenPtr->type == IDENT_TOKEN) {
		node->type = IDENTI;
		node->childCount = 0;
		node->token = context->tokenPtr;
		CONSUME_TOKEN;
		if (IN_RANGE && context->tokenPtr->type == PAREN_TOKEN
			&& strcmp(context->tokenPtr->lexeme, "(") == 0) {
			CONSUME_TOKEN;
			node->childCount = 0;
			do {
				if (IN_RANGE && context->tokenPtr->type == COMMA_TOKEN) {
					if (node->childCount > 0)
						CONSUME_TOKEN;
				}
				node->children = realloc(node->children, (node->childCount+1)*sizeof(uintptr_t));
				node->children[node->childCount] = parseExpr(context);
				node->childCount++;
				ERR_RET(node->children[0]);
			} while (IN_RANGE && context->tokenPtr->type == COMMA_TOKEN);
			if (IN_RANGE && context->tokenPtr->type == PAREN_TOKEN
				&& strcmp(context->tokenPtr->lexeme, ")") == 0) {
				CONSUME_TOKEN;
			} else {
				ERR("Expected closing parenthesis");
			}
		}
	}else {
			ERR("Expected identifiers");
	}
	return node;
}

ParseTreeNode *parseEqual(ParserContext *context)
{
	ParseTreeNode *node =
			(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (IN_RANGE && context->tokenPtr->type == RELOP_TOKEN
		&& strcmp(context->tokenPtr->lexeme, "=") == 0) {
		node->type = EQUALS;
		node->childCount = 0;
		node->token = context->tokenPtr;
		CONSUME_TOKEN;
	}else {
			ERR("Expected `=`");
	}
	return node;
}

ParseTreeNode *parseRelOperator(ParserContext *context)
{
	ParseTreeNode *node =
			(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (IN_RANGE && context->tokenPtr->type == RELOP_TOKEN) {
		if (strcmp(context->tokenPtr->lexeme, ">") == 0)
			node->type = GT;
		else if (strcmp(context->tokenPtr->lexeme, ">=") == 0)
			node->type = GE;
		else if (strcmp(context->tokenPtr->lexeme, "<") == 0)
			node->type = LT;
		else if (strcmp(context->tokenPtr->lexeme, "<=") == 0)
			node->type = LE;
		else if (strcmp(context->tokenPtr->lexeme, "=") == 0)
			node->type = EQ;
		else if (strcmp(context->tokenPtr->lexeme, "==") == 0)
			node->type = EQ;
		else if (strcmp(context->tokenPtr->lexeme, "<>") == 0)
			node->type = NE;
		else if (strcmp(context->tokenPtr->lexeme, "!=") == 0)
			node->type = NE;
		else
			ERR("Expected relops");
		node->childCount = 0;
		node->token = context->tokenPtr;
		CONSUME_TOKEN;
	} else {
			ERR("Expected relops");
	}
	return node;
}

ParseTreeNode *parseAddOperand(ParserContext *context)
{
	ParseTreeNode *node =
			(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (IN_RANGE && context->tokenPtr->type == OP_TOKEN
		&& (strcmp(context->tokenPtr->lexeme, "+") == 0
			|| strcmp(context->tokenPtr->lexeme, "-") == 0)) {
		node->type = ADD_OP;
		node->childCount = 0;
		node->token = context->tokenPtr;
		CONSUME_TOKEN;
	}else {
			ERR("Expected `+` or `-`");
	}
	return node;
}

ParseTreeNode *parseMulOperand(ParserContext *context)
{
	ParseTreeNode *node =
			(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (IN_RANGE && context->tokenPtr->type == OP_TOKEN
		&& (strcmp(context->tokenPtr->lexeme, "*") == 0
			|| strcmp(context->tokenPtr->lexeme, "/") == 0
			|| strcmp(context->tokenPtr->lexeme, "%") == 0)) {
		node->type = MUL_OP;
		node->childCount = 0;
		node->token = context->tokenPtr;
		CONSUME_TOKEN;
	}else {
			ERR("Expected `*`, `/` or `%`");
	}
	return node;
}

ParseTreeNode *parseUnaryOperand(ParserContext *context)
{
	ParseTreeNode *node =
			(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (IN_RANGE && ((context->tokenPtr->type == OP_TOKEN
		&& (strcmp(context->tokenPtr->lexeme, "+") == 0
			|| strcmp(context->tokenPtr->lexeme, "-") == 0))
		|| (context->tokenPtr->type == KEYWORD_TOKEN
			&& strcasecmp(context->tokenPtr->lexeme, "NOT") == 0))) {
		node->type = UNARY_OP;
		node->childCount = 0;
		node->token = context->tokenPtr;
		CONSUME_TOKEN;
	}else {
			ERR("Expected `+`, `-` or NOT");
	}
	return node;
}

ParseTreeNode *parseLinenum(ParserContext *context)
{
	ParseTreeNode *node = parseIntegerLiteral(context);
	ERR_RET_NOFREE(node);
	//node->childCount = 0;
	node->type = LINENUM;
	return node;
}

ParseTreeNode *parseIntegerLiteral(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (!IN_RANGE || context->tokenPtr->type != INT_TOKEN) {
		ERR("Not a integer");
	}
	node->childCount = 0;
	node->type = INTEGER;
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	return node;
}

ParseTreeNode *parseFloatLiteral(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (!IN_RANGE || context->tokenPtr->type != FLOAT_TOKEN)
		ERR("Not a float");
	node->childCount = 0;
	node->type = FLOAT;
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	return node;
}

ParseTreeNode *parseStringLiteral(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (!IN_RANGE || context->tokenPtr->type != STRING_TOKEN) {
		ERR("Not a string");
	}
	node->childCount = 0;
	node->type = STRING;
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	return node;
}

ParseTreeNode *parseStatement(ParserContext *context)
{
	ParseTreeNode *node;
	if (IN_RANGE && (context->tokenPtr->type == KEYWORD_TOKEN
		|| context->tokenPtr->type == IDENT_TOKEN)) {
		if (strcasecmp(context->tokenPtr->lexeme, "LET") == 0
			|| context->tokenPtr->type == IDENT_TOKEN) {
			node = parseLetStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "IF") == 0) {
			node = parseIfStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "ELSE") == 0) {
			node = parseElseStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "FI") == 0) {
			node = parseFiStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "PRINT") == 0) {
			node = parsePrintStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "INPUT") == 0) {
			node = parseInputStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "FOR") == 0) {
			node = parseForStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "NEXT") == 0) {
			node = parseForTail(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "GOSUB") == 0) {
			node = parseGoSubStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "RETURN") == 0) {
			node = parseReturnStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "GOTO") == 0) {
			node = parseGotoStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "DIM") == 0) {
			node = parseDimStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "PUTCHAR") == 0) {
			node = parsePutCharStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "CLEAR") == 0) {
			node = parseClearStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "HOME") == 0) {
			node = parseHomeStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "SLEEP") == 0) {
			node = parseSleepStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "DELETE") == 0) {
			node = parseDelStatement(context);
			ERR_RET_NOFREE(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "FREE") == 0) {
			node = parseFreeStatement(context);
			ERR_RET_NOFREE(node);
		}
		else {
			ERR_NOFREE("Unknown statement type");
		}
	} else
		ERR_NOFREE("Not a statement type");

	return node;
}

ParseTreeNode *parseLetStatement(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (!IN_RANGE || ((context->tokenPtr->type != KEYWORD_TOKEN
		|| strcasecmp(context->tokenPtr->lexeme, "LET") != 0)
		&& context->tokenPtr->type != IDENT_TOKEN))
		ERR("Not a LET statement");
	node->childCount = 0;
	node->type = LET;
	if (context->tokenPtr->type == KEYWORD_TOKEN
		&& strcasecmp(context->tokenPtr->lexeme, "LET") == 0) {
		node->token = context->tokenPtr;
		CONSUME_TOKEN;
	}

	node->children = malloc(3*sizeof(uintptr_t));
	node->children[0] = parseIdentifier(context);
	ERR_RET(node->children[0]);
	node->childCount++;
	node->children[1] = parseEqual(context);
	ERR_RET(node->children[1]);
	node->childCount++;
	node->children[2] = parseExpr(context);
	ERR_RET(node->children[2]);
	node->childCount++;

  	return node;
}

ParseTreeNode *parseDimStatement(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (!IN_RANGE || ((context->tokenPtr->type != KEYWORD_TOKEN
		|| strcasecmp(context->tokenPtr->lexeme, "DIM") != 0)
		&& context->tokenPtr->type != IDENT_TOKEN))
		ERR("Not a DIM statement");
	node->childCount = 0;
	node->type = DIM;
	if (context->tokenPtr->type == KEYWORD_TOKEN
		&& strcasecmp(context->tokenPtr->lexeme, "DIM") == 0) {
		node->token = context->tokenPtr;
		CONSUME_TOKEN;
	}
	node->children = malloc(sizeof(uintptr_t));
	node->children[0] = parseIdentifier(context);
	ERR_RET(node->children[0]);
	node->childCount++;

  	return node;
}

ParseTreeNode *parseIfStatement(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = IF;
	node->token = context->tokenPtr;
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "IF") != 0)
			ERR("Expected IF token.\n");
	CONSUME_TOKEN;
	node->children = malloc(sizeof(uintptr_t));
	node->children[0] = parseExpr(context);
	ERR_RET(node->children[0]);
	node->childCount++;
	if (IN_RANGE && context->tokenPtr->type == KEYWORD_TOKEN &&
		strcasecmp(context->tokenPtr->lexeme, "THEN") == 0) {
		CONSUME_TOKEN;
		node->children = realloc(node->children, 2*sizeof(uintptr_t));
		node->children[1] = parseLinenum(context); //3
		ERR_RET(node->children[1]); //3
		node->childCount++;
	}
	return node;
}

ParseTreeNode *parseElseStatement(ParserContext *context) {
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = ELSE;
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "ELSE") != 0)
			ERR("Expected ELSE token.\n");
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	return node;
}

ParseTreeNode *parseFiStatement(ParserContext *context) {
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = FI;
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "Fi") != 0)
			ERR("Expected Fi token.\n");
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	return node;
}

ParseTreeNode *parsePrintStatement(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = PRINT;
	node->token = context->tokenPtr;
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "PRINT") != 0)
			ERR("Expected PRINT token.\n");
	CONSUME_TOKEN;
	node->children = malloc(sizeof(uintptr_t));
	node->children[0] = parsePrintList(context);
	ERR_RET(node->children[0]);
	node->childCount++;
	return node;
}

ParseTreeNode *parsePutCharStatement(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = PUTCHAR;
	node->token = context->tokenPtr;
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "PUTCHAR") != 0)
			ERR("Expected PRINT token.\n");
	CONSUME_TOKEN;
	node->children = malloc(sizeof(uintptr_t));
	node->children[0] = parsePrintList(context);
	ERR_RET(node->children[0]);
	node->childCount++;
	return node;
}

ParseTreeNode *parseClearStatement(ParserContext *context) {
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = CLEAR;
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "CLEAR") != 0)
			ERR("Expected ELSE token.\n");
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	return node;
}

ParseTreeNode *parseHomeStatement(ParserContext *context) {
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = HOME;
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "HOME") != 0)
			ERR("Expected ELSE token.\n");
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	return node;
}

ParseTreeNode *parseSleepStatement(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "SLEEP") != 0)
			ERR("Expected GOTO token.\n");
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	node->type = SLEEP;
	node->children = malloc(sizeof(uintptr_t));
	node->children[0] = parseExpr(context);
	ERR_RET(node->children[0]);
	node->childCount++;
	return node;
}

ParseTreeNode *parsePrintList(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = PRINT_LIST;
	node->token = NULL;
	int i = 0;
	do {
		if (i >= 128)
			ERR("too many print items");
		if (i > 0) {
			if (IN_RANGE && (context->tokenPtr->type == SEMICOLON_TOKEN ||
				context->tokenPtr->type == COMMA_TOKEN)) {
				CONSUME_TOKEN;
			} else
				ERR("Expected commas or semicolons");
		}
		node->children = realloc(node->children, (i+1)*sizeof(uintptr_t));
		node->children[i] = parseExpr(context);
		ERR_RET(node->children[i]);
		if (node->children[i] == NULL)
			ERR("Expected print items in PRINT statement");
		i++;
	} while (IN_RANGE);
	node->childCount = i;
	return node;
}

ParseTreeNode *parseInputStatement(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = INPUT;
	node->token = context->tokenPtr;
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "INPUT") != 0)
			ERR("Expected INPUT token");
	CONSUME_TOKEN;
	node->children = malloc(sizeof(uintptr_t));
	node->children[0] = parseInputList(context);
	ERR_RET(node->children[0]);
	return node;
}

ParseTreeNode *parseInputList(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = INPUT_LIST;
	node->token = NULL;
	int i = 0;
	do {
		if (i >= 128)
			ERR("too many input items");
		if (i > 0) {
			if (IN_RANGE && (context->tokenPtr->type == SEMICOLON_TOKEN ||
				context->tokenPtr->type == COMMA_TOKEN)) {
				CONSUME_TOKEN;
			} else
				ERR("Expected commas or semicolons");
		}
		if (IN_RANGE && context->tokenPtr->type == IDENT_TOKEN) {
			ParseTreeNode *n =
				(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
			n->type = IDENTI;
			n->childCount = 0;
			n->token = context->tokenPtr;
			node->children = realloc(node->children, (i+1)*sizeof(uintptr_t));
			node->children[i++] = n;
			CONSUME_TOKEN;
		} else {
			ERR("Expected identifiers in PRINT statement");
		}
	} while (IN_RANGE);
	node->childCount = i;
	return node;
}

ParseTreeNode *parseGotoStatement(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "GOTO") != 0)
			ERR("Expected GOTO token.\n");
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	node->type = GOTO;
	node->children = malloc(sizeof(uintptr_t));
	node->children[0] = parseLinenum(context);
	ERR_RET(node->children[0]);
	node->childCount++;
	return node;
}

ParseTreeNode *parseGoSubStatement(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "GOSUB") != 0)
			ERR("Expected GOSUB token.\n");
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	node->type = GOSUB;
	node->children = malloc(sizeof(uintptr_t));
	node->children[0] = parseLinenum(context);
	ERR_RET(node->children[0]);
	node->childCount++;
	return node;
}

ParseTreeNode *parseReturnStatement(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "RETURN") != 0)
			ERR("Expected RETURN token.\n");
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	node->type = RETURN;
	node->childCount = 0;
	return node;
}

ParseTreeNode *parseForStatement(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "FOR") != 0)
			ERR("Expected FOR token.\n");
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	node->type = FOR;
	node->children = malloc(4*sizeof(uintptr_t));
	node->children[0] = parseIdentifier(context);
	ERR_RET(node->children[0]);
	node->childCount++;
	node->children[1] = parseEqual(context);
	ERR_RET(node->children[1]);
	node->childCount++;
	node->children[2] = parseExpr(context);
	ERR_RET(node->children[2]);
	node->childCount++;
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "TO") != 0)
			ERR("Expected TO token.\n");
	CONSUME_TOKEN;
	node->children[3] = parseExpr(context);
	ERR_RET(node->children[3]);
	node->childCount++;
	if (IN_RANGE && context->tokenPtr->type == KEYWORD_TOKEN
		&& strcasecmp(context->tokenPtr->lexeme, "STEP") == 0) {
		CONSUME_TOKEN;
		node->children = realloc(node->children, 5*sizeof(uintptr_t));
		node->children[4] = parseExpr(context);
		ERR_RET(node->children[4]);
		node->childCount++;
	}
	return node;
}

ParseTreeNode *parseForTail(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "NEXT") != 0)
			ERR("Expected NEXT token.\n");
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	node->token = context->tokenPtr;
	node->type = NEXT;
	node->children = malloc(sizeof(uintptr_t));
	node->children[0] = parseIdentifier(context);
	ERR_RET(node->children[0]);
	node->childCount = 1;
	return node;
}

ParseTreeNode *parseDelStatement(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "DELETE") != 0)
			ERR("Expected DELETE token.\n");
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	node->type = DEL;
	node->children = malloc(sizeof(uintptr_t));
	node->children[0] = parseIdentifier(context);
	ERR_RET(node->children[0]);
	node->childCount++;
	return node;
}

ParseTreeNode *parseFreeStatement(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "FREE") != 0)
			ERR("Expected FREE token.\n");
	node->token = context->tokenPtr;
	CONSUME_TOKEN;
	node->type = FREE;
	node->children = malloc(sizeof(uintptr_t));
	node->children[0] = parseIdentifier(context);
	ERR_RET(node->children[0]);
	node->childCount++;
	return node;
}

ParseTreeNode *parseExpr(ParserContext *context)
{
	ParseTreeNode *node;
	node = parseOrExpr(context);
	ERR_RET_NOFREE(node);
	return node;
}

ParseTreeNode *parseOrExpr(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	int cnt = 0;
	node->type = OR_EXPR;
	node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
	node->children[cnt++] = parseAndExpr(context);
	while (IN_RANGE && (context->tokenPtr->type == KEYWORD_TOKEN
				&& strcasecmp(context->tokenPtr->lexeme, "OR") == 0)) {
		struct ParseTreeNode *node2 = parseOrOperand(context);
		ERR_RET(node2);
		node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
		node->children[cnt++] = node2;
		struct ParseTreeNode *node3 = parseAndExpr(context);
		ERR_RET(node3);
		node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
		node->children[cnt++] = node3;
	}
	node->childCount = cnt;
	return node;
}


ParseTreeNode *parseOrOperand(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (IN_RANGE && context->tokenPtr->type == KEYWORD_TOKEN
		&& strcasecmp(context->tokenPtr->lexeme, "OR") == 0) {
		node->type = OR_OP;
		node->token = context->tokenPtr;
		node->childCount = 0;
		CONSUME_TOKEN;
	}
	return node;
}

ParseTreeNode *parseAndExpr(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	int cnt = 0;
	node->type = AND_EXPR;
	node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
	node->children[cnt++] = parseRelExpr(context);
	while (IN_RANGE && strcasecmp(context->tokenPtr->lexeme, "AND") == 0) {
		struct ParseTreeNode *node2 = parseAndOperand(context);
		ERR_RET(node2);
		node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
		node->children[cnt++] = node2;
		struct ParseTreeNode *node3 = parseRelExpr(context);
		ERR_RET(node3);
		node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
		node->children[cnt++] = node3;
	}
	node->childCount = cnt;
	return node;
}

ParseTreeNode *parseAndOperand(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (IN_RANGE && context->tokenPtr->type == KEYWORD_TOKEN
		&& strcasecmp(context->tokenPtr->lexeme, "AND") == 0) {
		node->type = AND_OP;
		node->token = context->tokenPtr;
		node->childCount = 0;
		CONSUME_TOKEN;
	}
	return node;
}

#define IS_RELOP (strcmp(context->tokenPtr->lexeme, ">") == 0     \
				  || strcmp(context->tokenPtr->lexeme, "<") == 0  \
				  || strcmp(context->tokenPtr->lexeme, ">=") == 0 \
				  || strcmp(context->tokenPtr->lexeme, "<=") == 0 \
				  || strcmp(context->tokenPtr->lexeme, "=") == 0  \
				  || strcmp(context->tokenPtr->lexeme, "==") == 0 \
				  || strcmp(context->tokenPtr->lexeme, "!=") == 0 \
				  || strcmp(context->tokenPtr->lexeme, "<>") == 0)
ParseTreeNode *parseRelExpr(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	int cnt = 0;
	node->type = REL_EXPR;
	node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
	node->children[cnt++] = parseAddExpr(context);
	if (IN_RANGE && context->tokenPtr->type == RELOP_TOKEN
			&& IS_RELOP) {
		struct ParseTreeNode *node2 = parseRelOperator(context);
		ERR_RET(node2);
		node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
		node->children[cnt++] = node2;
		struct ParseTreeNode *node3 = parseAddExpr(context);
		ERR_RET(node3);
		node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
		node->children[cnt++] = node3;
	}
	node->childCount = cnt;
	return node;
}
#undef IS_RELOP

ParseTreeNode *parseAddExpr(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	int cnt = 0;
	node->type = ADD_EXPR;
	node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
	node->children[cnt++] = parseMulExpr(context);
	while (IN_RANGE && context->tokenPtr->type == OP_TOKEN
			&& (strcmp(context->tokenPtr->lexeme, "+") == 0
			|| strcmp(context->tokenPtr->lexeme, "-") == 0)) {
		struct ParseTreeNode *node2 = parseAddOperand(context);
		ERR_RET(node2);
		node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
		node->children[cnt++] = node2;
		struct ParseTreeNode *node3 = parseMulExpr(context);
		ERR_RET(node3);
		node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
		node->children[cnt++] = node3;
	}
	node->childCount = cnt;
	return node;
}

ParseTreeNode *parseMulExpr(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	int cnt = 0;
	node->type = MUL_EXPR;
	node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
	node->children[cnt++] = parseUnary(context);
	while (IN_RANGE && context->tokenPtr->type == OP_TOKEN
			&& (strcmp(context->tokenPtr->lexeme, "*") == 0
			|| strcmp(context->tokenPtr->lexeme, "/") == 0
			|| strcmp(context->tokenPtr->lexeme, "%") == 0 )) {
		struct ParseTreeNode *node2 = parseMulOperand(context);
		ERR_RET(node2);
		node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
		node->children[cnt++] = node2;
		struct ParseTreeNode *node3 = parseUnary(context);
		ERR_RET(node3);
		node->children = realloc(node->children, (cnt+1)*sizeof(uintptr_t));
		node->children[cnt++] = node3;
	}
	node->childCount = cnt;
	return node;
}

ParseTreeNode *parseUnary(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = UNARY;
	if (IN_RANGE && (strcmp(context->tokenPtr->lexeme, "+") == 0 ||
		strcmp(context->tokenPtr->lexeme, "-") == 0 ||
		strcasecmp(context->tokenPtr->lexeme, "NOT") == 0)) {
		struct ParseTreeNode *node2 = parseUnaryOperand(context);
		ERR_RET(node2);
		node->children = malloc(2*sizeof(uintptr_t));
		node->children[0] = node2;
		node->childCount++;
		struct ParseTreeNode *node3 = parsePrimary(context);
		ERR_RET(node3);
		node->children[1] = node3;
		node->childCount++;
	} else {
		struct ParseTreeNode *node2 = parsePrimary(context);
		ERR_RET(node2);
		node->children = malloc(sizeof(uintptr_t));
		node->children[0] = node2;
		node->childCount++;
	}
	return node;
}

ParseTreeNode *parsePrimary(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = PRIMARY;
	if (IN_RANGE && context->tokenPtr->type == INT_TOKEN) {
		node->children = malloc(sizeof(uintptr_t));
		node->children[0] = parseIntegerLiteral(context);
		ERR_RET(node->children[0]);
		node->childCount++;
	} else if (IN_RANGE && context->tokenPtr->type == FLOAT_TOKEN) {
		node->children = malloc(sizeof(uintptr_t));
		node->children[0] = parseFloatLiteral(context);
		ERR_RET(node->children[0]);
		node->childCount++;
	} else if (IN_RANGE && context->tokenPtr->type == STRING_TOKEN) {
		node->children = malloc(sizeof(uintptr_t));
		node->children[0] = parseStringLiteral(context);
		ERR_RET(node->children[0]);
		node->childCount++;
	} else if (IN_RANGE && context->tokenPtr->type == IDENT_TOKEN) {
		node->children = malloc(sizeof(uintptr_t));
		node->children[0] = parseIdentifier(context);
		ERR_RET(node->children[0]);
		node->childCount++;
	} else if (IN_RANGE && strcmp(context->tokenPtr->lexeme, "(") == 0){
		CONSUME_TOKEN;
		struct ParseTreeNode *node2 = parseExpr(context);
		ERR_RET(node2);
		node->children = realloc(node->children, (node->childCount+1)*sizeof(uintptr_t));
		node->children[node->childCount] = node2;
		node->childCount++;
		if (IN_RANGE && strcmp(context->tokenPtr->lexeme, ")") == 0) {
			CONSUME_TOKEN;
		} else {
			ERR("Expected closing parenthesis");
		}
	} else {
		ERR("Invalid primary expression");
	}
	return node;
}
