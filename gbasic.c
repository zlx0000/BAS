// Copyright (C) 2026 leo z <zlx20010815@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gbasic.h"
#include <unistd.h>
#ifndef WIN32
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define STR_SIZE 8192

Program prog;
int pc;
int expectedLineNum;
Stack shadow_st;

#ifndef WIN32
static char *keyword_generator(const char *text, int state)
{
    static int index;
    static size_t len;
    if (state == 0) {
        index = 0;
        len = strlen(text);
    }
    while (index < KEYWORDS_SIZE) {
        const char *word = keywords[index++];
        if (strncasecmp(word, text, len) == 0) {
            char *result = strdup(word);

            if (result == NULL)
                return NULL;

            if (len > 0 && islower((unsigned char)text[0])) {
                for (char *p = result; *p; p++)
                    *p = (char)tolower((unsigned char)*p);
            }
            return result;
        }
    }
    return NULL;
}

static char **gbasic_completion(
    const char *text,
    int start,
    int end)
{
    (void)start;
    (void)end;
    return rl_completion_matches(text, keyword_generator);
}
#endif

bool find_lineNum(Program p, int n)
{
	for (int i = 0; i < p.lineCount; i++) {
		if (p.lines[i]->childCount > 1 && p.lines[i]->children[0]->token->literal.intValue == n)
			return true;
	}
	return false;
}

int main(int argc, char **argv)
{
	char *str;
	ParseTreeNode *p = NULL;
	ParserContext ctx;
	prog.lineCount = 0;
	prog.lines = (ParseTreeNode **)calloc(16384, sizeof (ParseTreeNode *));
	prog.shadow_st = (Stack *)calloc(16384, sizeof (Stack));
#ifndef WIN32
	rl_variable_bind("enable-bracketed-paste", "off");
	rl_attempted_completion_function = gbasic_completion;
	using_history();
    read_history(".gbasic_history");
#endif
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
	;
	Value ret = {
		.type = INT_VAL,
	};
	if (isatty(STDIN_FILENO)) {
#ifdef WIN32
		printf(">");
		str = calloc(STR_SIZE, sizeof(char));
		if (!fgets(str, STR_SIZE, stdin)) {
    	  	printf("\n");
    	   	return 0;
    	}
#else
		if (!(str = readline(">"))) {
    	   	return 0;
    	}
		add_history(str);
#endif
	}
	else {
		str = calloc(STR_SIZE, sizeof(char));
		if (!fgets(str, STR_SIZE, stdin)) {
    	  	printf("\n");
    	   	return 0;
    	}
	}
	if (strcasecmp(str, "exit\n") == 0)
		return 0;
	Token *tokens =
	(Token *)calloc(1, sizeof(Token) * MAX_TOKEN);
	int len = lexer(str, tokens, prog.lineCount);
	free(str);
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
				if (p->childCount > 1 && p->children[0]->type == LINENUM
					&& p->children[0]->token->literal.intValue == expectedLineNum) {
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
