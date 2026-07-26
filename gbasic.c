// Copyright (C) 2026 leo z <zlx20010815@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gbasic.h"
#include <unistd.h>

Program prog;
int pc;
int expectedLineNum;
Stack shadow_st;

bool find_lineNum(Program p, int n)
{
	for (int i = 0; i < p.lineCount; i++) {
		if (p.lines[i]->children[0]->token->literal.intValue == n)
			return true;
	}
	return false;
}

int main(int argc, char **argv)
{
	char str[1024];
	ParseTreeNode *p = NULL;
	ParserContext ctx;
	prog.lineCount = 0;
	prog.lines = (ParseTreeNode **)calloc(16384, sizeof (ParseTreeNode *));
	prog.shadow_st = (Stack *)calloc(16384, sizeof (Stack));
	for (int i = 0; i < 16384; i++) {
		prog.shadow_st->size = -1;
	}
	if (prog.lines == NULL) {
		perror("Memory allocation failed");
		exit(EXIT_FAILURE);
	}
	pc = 0;
	expectedLineNum = -1;
    init_eval();
repl:
	Value ret = {
		.type = INT_VAL,
	};
	if (isatty(STDIN_FILENO)) {
    	printf(">");
	}
	if (!fgets(str, sizeof(str), stdin)) {
      	printf("\n");
       	return 0;
    }
	if (strcasecmp(str, "exit\n") == 0)
		return 0;
	Token *tokens =
	(Token *)calloc(1, sizeof(Token) * MAX_TOKEN);
	int len = lexer(str, tokens, prog.lineCount);
	if (len > 0) {
		Token *t = realloc(tokens, (len + 1) * sizeof(Token));
		tokens = t ? t : tokens;
	    ctx.tokens = tokens;
		ctx.tokenLen = len;
		ctx.len = 0;
	    ctx.tokenPtr = ctx.tokens;
		ctx.err = false;
		p = parseLine(&ctx);
		if (p && !ctx.err) {
			if (p->childCount > 1 && find_lineNum(prog, p->children[0]->token->literal.intValue)) {
				fprintf(stderr, "duplicate lineNum\n");
				free_tree(p);
				free(tokens);
				ret.type = INT_VAL;
				goto repl;
			}
			prog.lines[prog.lineCount] = p;
			if (p->children[0]->type == LINENUM) {
				copy_stack(&shadow_st, &prog.shadow_st[prog.lineCount]);
			}
			prog.lineCount++;
			if (pc == -1) {
				if (is_if_else_or_fi(p)) {
					ret = evalLine(p);
					pc = -1;
				}
				if (p->children[0]->token->literal.intValue == expectedLineNum) {
					pc = lineNum_to_pc(expectedLineNum);
					if (prog.shadow_st[pc].size >= 0) {
            			copy_stack(&prog.shadow_st[pc], &if_st);
						copy_stack(&if_st, &shadow_st);
						if_state = if_st.st[if_st.size-1].value.ifFrame.state;
					}
				}
			}
			while (pc >= 0 && pc < prog.lineCount) {
				if (__unlikely(ret.type == ERR_VAL)) {
					prog.lineCount--;
					free_tree(p);
					free(tokens);
					ret.type = INT_VAL;
					goto repl;
				}
				if (__unlikely(if_state == IF_EXPECTING_ELSE_OR_FI
					|| if_state == IF_EXPECTING_FI)) {
					if (is_if_else_or_fi(prog.lines[pc]))
						ret = evalLine(prog.lines[pc]);
					else
						pc++;
				} else
					ret = evalLine(prog.lines[pc]);
			}
		}
		p = NULL;
		goto repl;
	} else {
		free(tokens);
		goto repl;
	}
}
