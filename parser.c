#include "gbasic.h"

#define ERR(str) {if (!IN_RANGE) {fprintf(stderr,\
		 "token stream ended prematurely\n");\
		return NULL;}fprintf(stderr, "%s at %d,%d: `%s`,\n",\
		str,context->tokenPtr->lineNum, context->tokenPtr->colNum,                  \
		context->tokenPtr->lexeme); context->err = true ;return NULL;}
#define IN_RANGE (context->len < context->tokenLen)
#define LAST_TOKEN (context->len == context->tokenLen-1)
#define ERR_RET(node) {if (!node) {context->err=true; return NULL;}}
#define CONSUME_TOKEN {if (!IN_RANGE) \
		{ERR("Token stream ended prematurely")}; context->tokenPtr++; \
		context->len++;}

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

ParseTreeNode *parseLine(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = ROOT;
	node->children[0] = parseLinenum(context);
	ERR_RET(node->children[0]);
	node->childCount++;
	node->children[1] = parseStatement(context);
	ERR_RET(node->children[1]);
	node->childCount++;
	if (IN_RANGE) {
		ERR("Token stream did not end");
		return NULL;
	}
	//parseEOL(context);
	return node;
}

ParseTreeNode *parseIdentifier(ParserContext *context)
{
	ParseTreeNode *node =
			(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	if (IN_RANGE && context->tokenPtr->type == IDENT_TOKEN) {
		node->type = IDENT_TOKEN;
		node->childCount = 0;
		node->token = context->tokenPtr;
		CONSUME_TOKEN;
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
			|| strcmp(context->tokenPtr->lexeme, "/") == 0)) {
		node->type = ADD_OP;
		node->childCount = 0;
		node->token = context->tokenPtr;
		CONSUME_TOKEN;
	}else {
			ERR("Expected `*` or `/`");
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
	ERR_RET(node);
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
	if (IN_RANGE && context->tokenPtr->type == KEYWORD_TOKEN) {
		if (strcasecmp(context->tokenPtr->lexeme, "LET") == 0
			|| context->tokenPtr->type == IDENT_TOKEN) {
			node = parseLetStatement(context);
			ERR_RET(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "IF") == 0) {
			node = parseIfStatement(context);
			ERR_RET(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "PRINT") == 0) {
			node = parsePrintStatement(context);
			ERR_RET(node);
		}
		else if (strcasecmp(context->tokenPtr->lexeme, "INPUT") == 0) {
			node = parseInputStatement(context);
			ERR_RET(node);
		}
		else {
			ERR("Unknown statement type");
		}
	} else
		ERR("Not a statement type");

	return node;
}

ParseTreeNode *parseLetStatement(ParserContext *context)
{
	if (!IN_RANGE || ((context->tokenPtr->type != KEYWORD_TOKEN
		|| strcasecmp(context->tokenPtr->lexeme, "LET") != 0)
		&& context->tokenPtr->type != IDENT_TOKEN))
		ERR("Not a LET statement");
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	node->childCount = 0;
	node->type = LET;
	node->token = context->tokenPtr;
	if (context->tokenPtr->type == KEYWORD_TOKEN
		&& strcasecmp(context->tokenPtr->lexeme, "LET") == 0)
		CONSUME_TOKEN;

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
	node->children[0] = parseExpr(context);
	ERR_RET(node->children[0]);
	node->childCount++;
	node->children[1] = parseRelOperator(context);
	ERR_RET(node->children[1]);
	node->childCount++;
	node->children[2] = parseExpr(context);
	ERR_RET(node->children[2]);
	node->childCount++;
	if (!IN_RANGE || context->tokenPtr->type != KEYWORD_TOKEN ||
		strcasecmp(context->tokenPtr->lexeme, "THEN") != 0)
			ERR("Expected THEN in IF statement.\n");
	CONSUME_TOKEN;
	node->children[3] = parseLinenum(context);
	ERR_RET(node->children[3]);
	node->childCount++;
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
	node->children[0] = parsePrintList(context);
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
		if (i > 128)
			ERR("too many print items");
		if (i > 0) {
			if (IN_RANGE && (context->tokenPtr->type == SEMICOLON_TOKEN ||
				context->tokenPtr->type == COMMA_TOKEN)) {
				CONSUME_TOKEN;
			} else
				ERR("Expected commas or semicolons");
		}
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
		if (i > 128)
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
	CONSUME_TOKEN;
	node->children[0] = parseLinenum(context);
	ERR_RET(node->children[0]);
	node->childCount++;
	return node;
}

ParseTreeNode *parseExpr(ParserContext *context)
{
	ParseTreeNode *node;
	node = parseOrExpr(context);
	ERR_RET(node);
	return node;
}

ParseTreeNode *parseOrExpr(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	int cnt = 0;
	node->type = OR_EXPR;
	node->children[cnt++] = parseAndExpr(context);
	while (IN_RANGE && (context->tokenPtr->type == KEYWORD_TOKEN
				&& strcasecmp(context->tokenPtr->lexeme, "OR") == 0)) {
		struct ParseTreeNode *node2 = parseOrOperand(context);
		ERR_RET(node2);
		node->children[cnt++] = node2;
		struct ParseTreeNode *node3 = parseAndExpr(context);
		ERR_RET(node3);
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
	node->children[cnt++] = parseAddExpr(context);
	while (IN_RANGE && strcasecmp(context->tokenPtr->lexeme, "AND") == 0) {
		struct ParseTreeNode *node2 = parseAndOperand(context);
		ERR_RET(node2);
		node->children[cnt++] = node2;
		cnt++;
		struct ParseTreeNode *node3 = parseAddExpr(context);
		ERR_RET(node3);
		node->children[cnt++] = node3;
		cnt++;
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

ParseTreeNode *parseAddExpr(ParserContext *context)
{
	ParseTreeNode *node =
		(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
	int cnt = 0;
	node->type = ADD_EXPR;
	node->children[cnt++] = parseMulExpr(context);
	while (IN_RANGE && context->tokenPtr->type == OP_TOKEN
			&& (strcmp(context->tokenPtr->lexeme, "+") == 0
			|| strcmp(context->tokenPtr->lexeme, "-") == 0)) {
		struct ParseTreeNode *node2 = parseAddOperand(context);
		ERR_RET(node2);
		node->children[cnt++] = node2;
		struct ParseTreeNode *node3 = parseMulExpr(context);
		ERR_RET(node3);
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
	node->children[cnt++] = parseUnary(context);
	while (IN_RANGE && context->tokenPtr->type == OP_TOKEN
			&& (strcmp(context->tokenPtr->lexeme, "*") == 0
			|| strcmp(context->tokenPtr->lexeme, "/") == 0)) {
		struct ParseTreeNode *node2 = parseMulOperand(context);
		ERR_RET(node2);
		node->children[cnt++] = node2;
		struct ParseTreeNode *node3 = parseUnary(context);
		ERR_RET(node3);
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
		strcmp(context->tokenPtr->lexeme, "NOT") == 0)) {
		struct ParseTreeNode *node2 = parseUnaryOperand(context);
		ERR_RET(node2);
		node->children[0] = node2;
		node->childCount++;
		struct ParseTreeNode *node3 = parsePrimary(context);
		ERR_RET(node3);
		node->children[1] = node3;
		node->childCount++;
	} else {
		node->children[0] = NULL; // todo: add a placeholder for no unary operator
		node->childCount++;
		struct ParseTreeNode *node2 = parsePrimary(context);
		ERR_RET(node2);
		node->children[1] = node2;
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
		node->children[0] = parseIntegerLiteral(context);
		ERR_RET(node->children[0]);
		node->childCount++;
	} else if (IN_RANGE && context->tokenPtr->type == FLOAT_TOKEN) {
		node->children[0] = parseFloatLiteral(context);
		ERR_RET(node->children[0]);
		node->childCount++;
	} else if (IN_RANGE && context->tokenPtr->type == STRING_TOKEN) {
		node->children[0] = parseStringLiteral(context);
		ERR_RET(node->children[0]);
		node->childCount++;
	} else if (IN_RANGE && context->tokenPtr->type == IDENT_TOKEN) {
		ParseTreeNode *n =
			(ParseTreeNode *)calloc(1, sizeof(ParseTreeNode));
		n->type = IDENTI;
		n->childCount = 0;
		n->token = context->tokenPtr;
		node->children[0] = n;
		CONSUME_TOKEN;
	} else if (IN_RANGE && strcmp(context->tokenPtr->lexeme, "(") == 0){
		CONSUME_TOKEN;
		struct ParseTreeNode *node2 = parseExpr(context);
		ERR_RET(node2);
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
