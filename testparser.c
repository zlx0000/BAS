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

#include "gbasic.h"

static const char *token_type_to_string(TokenType type)
{
	switch (type) {
		case TOKEN_TYPE_NULL:
			return "TOKEN_TYPE_NULL";
		case STRING_TOKEN:
			return "STRING_TOKEN";
		case KEYWORD_TOKEN:
			return "KEYWORD_TOKEN";
		case RELOP_TOKEN:
			return "RELOP_TOKEN";
		case OP_TOKEN:
			return "OP_TOKEN";
		case FLOAT_TOKEN:
			return "FLOAT_TOKEN";
		case INT_TOKEN:
			return "INT_TOKEN";
		case IDENT_TOKEN:
			return "IDENT_TOKEN";
		case SPACE_TOKEN:
			return "SPACE_TOKEN";
		case TOKEN_TYPE_END:
			return "TOKEN_TYPE_END";
		case PAREN_TOKEN:
			return "PAREN_TOKEN";
		case COMMA_TOKEN:
			return "COMMA_TOKEN";
		case SEMICOLON_TOKEN:
			return "SEMICOLON_TOKEN";
		default:
			return "UNKNOWN_TOKEN_TYPE";
	}
}

static const char *node_type_to_string(NodeType type)
{
	switch (type) {
		case NODE_TYPE_NULL:
			return "NODE_TYPE_NULL";
		case ROOT:
			return "ROOT";
		case EXPR:
			return "EXPR";
		case OR_EXPR:
			return "OR_EXPR";
		case AND_EXPR:
			return "AND_EXPR";
		case REL_EXPR:
			return "REL_EXPR";
		case ADD_EXPR:
			return "ADD_EXPR";
		case MUL_EXPR:
			return "MUL_EXPR";
		case UNARY:
			return "UNARY";
		case PRIMARY:
			return "PRIMARY";
		case LINENUM:
			return "LINENUM";
		case INTEGER:
			return "INTEGER";
		case FLOAT:
			return "FLOAT";
		case INTEGERIDENT:
			return "INTEGERIDENT";
		case FLOATIDENT:
			return "FLOATIDENT";
		case STRING:
			return "STRING";
		case IDENTI:
			return "IDENTI";
		case REM:
			return "REM";
		case LET:
			return "LET";
		case IF:
			return "IF";
		case ELSE:
			return "ELSE";
		case FI:
			return "FI";
		case THEN:
			return "THEN";
		case PRINT:
			return "PRINT";
		case PRINT_LIST:
			return "PRINT_LIST";
		case INPUT:
			return "INPUT";
		case INPUT_LIST:
			return "INPUT_LIST";
		case FOR:
			return "FOR";
		case TO:
			return "TO";
		case STEP:
			return "STEP";
		case NEXT:
			return "NEXT";
		case GOTO:
			return "GOTO";
		case GOSUB:
			return "GOSUB";
		case RETURN:
			return "RETURN";
		case END:
			return "END";
		case PLUS:
			return "PLUS";
		case MINUS:
			return "MINUS";
		case STAR:
			return "STAR";
		case SLASH:
			return "SLASH";
		case CARET:
			return "CARET";
		case EQUALS:
			return "EQUALS";
		case LT:
			return "LT";
		case GT:
			return "GT";
		case LE:
			return "LE";
		case GE:
			return "GE";
		case NE:
			return "NE";
		case EQ:
			return "EQ";
		case AND_OP:
			return "AND_OP";
		case OR_OP:
			return "OR_OP";
		case ADD_OP:
			return "ADD_OP";
		case MUL_OP:
			return "MUL_OP";
		case UNARY_OP:
			return "UNARY_OP";
		case NOT:
			return "NOT";
		case COMMA:
			return "COMMA";
		case SEMICOLON:
			return "SEMICOLON";
		case COLON:
			return "COLON";
		case LPAREN:
			return "LPAREN";
		case RPAREN:
			return "RPAREN";
		default:
			return "UNKNOWN_NODE_TYPE";
	}
}

void print_tree(ParseTreeNode *t, unsigned int depth)
{
	static bool depth_arr[256];
	if (depth >= sizeof(depth_arr))
		return;
	if (t->token)
		printf("%s: %s\n", node_type_to_string(t->type), t->token->lexeme);
	else
		printf("%s\n", node_type_to_string(t->type));
	if (depth >= 1) {
		for (int i = 0; i < t->childCount; i++) {
			if (i != t->childCount-1) {
				for (int j = 1; j < depth; j++) {
					if(depth_arr[j])
						printf("│ ");
					else
						printf("  ");
				}
				printf("├─");
				depth_arr[depth] = true;
			} else {
				for (int j = 1; j < depth; j++) {
					if(depth_arr[j])
						printf("│ ");
					else
						printf("  ");
				}
				printf("└─");
				depth_arr[depth] = false;
			}
			print_tree(t->children[i], depth+1);
		}
	} else {
		for (int i = 0; i < t->childCount; i++) {
			print_tree(t->children[i], depth+1);
		}
	}
}

int main(int argc, char **argv)
{
	FILE *fp = NULL;
	char str[1024];
	int line = 1;
	ParseTreeNode *p = NULL;
	if (argc <= 1) {
repl:
		printf(">");
		if (!fgets(str, sizeof(str), stdin)) {
        	printf("\n");
        	return 0;
    	}
		if (strcasecmp(str, "exit\n") == 0)
			return 0;
		Token *tokens =
		(Token *)calloc(1, sizeof(Token) * MAX_TOKEN);
		Program prog;
		prog.lineCount = 0;
		prog.lines = (ParseTreeNode **)calloc(16384, sizeof(ParseTreeNode *));
		if (prog.lines == NULL) {
			perror("Memory allocation failed");
			exit(EXIT_FAILURE);
		}
		int len = lexer(str, tokens, line);
		if (len > 0) {
			line++;
			for (size_t i = 0; i < len; i++)
				printf("%d,%d: %s (%s)\n", tokens[i].lineNum, tokens[i].colNum,
					tokens[i].lexeme, token_type_to_string(tokens[i].type));
		}
		ParserContext ctx;
    	ctx.tokens = tokens;
		ctx.tokenLen = len;
		ctx.len = 0;
    	ctx.tokenPtr = ctx.tokens;
		ctx.err = false;
		if (len > 0) {
			p = parseLine(&ctx);
		}
		if (p && !ctx.err) {
			print_tree(p, 1);
			prog.lines[prog.lineCount] = p;
			prog.lineCount = line;
		}
		p = NULL;
		goto repl;
    }
	if (*argv[1]) {
		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			perror("Can't open file");
			exit(EXIT_FAILURE);
		}
	}

	fseek(fp, 0, SEEK_END);
	unsigned long size = ftell(fp);
	rewind(fp);

	char *bf = (char *)malloc(size + 1);
	if (bf == NULL) {
		perror("Can't load the file, memory allocation failed");
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	size_t byteRead = fread(bf, 1, size, fp);
	if (byteRead != size) {
		perror("Error reading file");
		free(bf);
		fclose(fp);
		exit(EXIT_FAILURE);
	}
	bf[size] = '\0';
	fclose(fp);
	Program *prog = (Program *)calloc(1, sizeof(Program));
	if (prog == NULL) {
		perror("Memory allocation failed");
		free(bf);
		exit(EXIT_FAILURE);
	}
	prog->lines = (ParseTreeNode **)calloc(
		1, sizeof(ParseTreeNode *) * 16384);
	if (prog->lines == NULL) {
		perror("Memory allocation failed");
		free(prog);
		free(bf);
		exit(EXIT_FAILURE);
	}

	Token *tokens =
		(Token *)calloc(1, sizeof(Token) * MAX_TOKEN);

	char *start = bf;
	char *end = start;
	int slen = 0;
next:
	while (*end != '\n' && *end != '\0') {
		*end++;
		slen++;
	}
	strncpy(str, start, slen);
	str[slen] = '\0';
	slen = 0;
	if (*end != '\0') {
		start = end + 1;
		end = start;
	}
    printf("%s\n", str);
	int len = lexer(str, tokens, line);
	line++;
	if (len < 0)
		return len;
	for (size_t i = 0; i < len; i++)
		printf("%d,%d: %s (%s)\n", tokens[i].lineNum, tokens[i].colNum,
			tokens[i].lexeme, token_type_to_string(tokens[i].type));
	ParserContext ctx;
    ctx.tokens = tokens;
	ctx.tokenLen = len;
	ctx.len = 0;
    ctx.tokenPtr = ctx.tokens;
	ctx.err = false;
    p = parseLine(&ctx);
	if (end == bf + size)
		return 0;
	goto next;
}
