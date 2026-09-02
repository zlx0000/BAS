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

#include "bas.h"
#include <unistd.h>
#ifndef WIN32
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define STR_SIZE 8192

Program prog;
int pc;
int expectedLineNum;
extern Stack shadow_st;
bool in_fun_def = false;
BasFunction *def_fun = NULL;
Stack def_shadow_st;

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

bool find_lineNum_fun(BasFunction *f, int n)
{
	for (int i = 0; i < f->lineCount; i++) {
		if (f->lines[i]->childCount > 1 && f->lines[i]->children[0]->token->literal.intValue == n)
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
	//prog.lines = (ParseTreeNode **)calloc(16384, sizeof (ParseTreeNode *));
	//prog.shadow_st = (Stack *)calloc(16384, sizeof (Stack));
#ifndef WIN32
	rl_variable_bind("enable-bracketed-paste", "off");
	rl_attempted_completion_function = gbasic_completion;
	using_history();
    read_history(".gbasic_history");
#endif
	/*
	for (int i = 0; i < 16384; i++) {
		prog.shadow_st->size = -1;
	}
	if (prog.lines == NULL) {
		perror("Memory allocation failed");
		exit(EXIT_FAILURE);
	}
	*/
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
			free(str);
			if (isatty(STDIN_FILENO))
    	  		printf("\n");
			free(str);
    	   	return 0;
    	}
		add_history(str);
#endif
	}
	else {
		str = calloc(STR_SIZE, sizeof(char));
		if (!fgets(str, STR_SIZE, stdin)) {
			free(str);
    	   	return 0;
    	}
	}
	if (strcasecmp(str, "exit\n") == 0
		|| strcasecmp(str, "exit") == 0) {
		free(str);
		return 0;
	}
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
		if (ctx.err) {
			free(t);
			if (p)
				free_tree(p);
		}
		if (p && !ctx.err) {
			if (in_fun_def) {
				if (p->childCount > 1 && find_lineNum_fun(def_fun, p->children[0]->token->literal.intValue)) {
					fprintf(stderr, "duplicate lineNum\n");
					free_tree(p);
					free(tokens);
					ret.type = INT_VAL;
					goto repl;
				}
				if (p->children[p->childCount-1]->type == FUN) {
					fprintf(stderr, "can not define functions in functions\n");
					free_tree(p);
					free(tokens);
					ret.type = INT_VAL;
					goto repl;
				}
				else if (p->children[p->childCount-1]->type == ENDFUN) {
					evalLine(p);
					free_tree(p);
					free(tokens);
					goto repl;
				}
				def_fun->shadow_st = realloc(def_fun->shadow_st,
					(def_fun->lineCount + 1) * sizeof (Stack));
				def_fun->shadow_st[def_fun->lineCount].size = 0;
				def_fun->lines = realloc(def_fun->lines,
					(def_fun->lineCount + 1) * sizeof (ParseTreeNode *));
				def_fun->lines[def_fun->lineCount] = p;
				if (p->children[0]->type == LINENUM) {
					copy_stack(&def_shadow_st, &def_fun->shadow_st[def_fun->lineCount]);
				}
				if (p->children[p->childCount-1]->type == IF) {
					Value v;
					v.type = IF_FRAME;
					v.value.ifFrame.entry = def_fun->lineCount;
					v.value.ifFrame.state = IF_SKIP_ELSE;
				 	push(&def_shadow_st, v);
				}
				else if (p->children[p->childCount-1]->type == ELSE) {
					Value top = peek(&def_shadow_st);
        			if (top.type != IF_FRAME) {
            			fprintf(stderr, "not in a if statement\n");
						free_tree(p);
						free(tokens);
						ret.type = INT_VAL;
						goto repl;
					}
					Value v;
					v.type = IF_FRAME;
					v.value.ifFrame.entry = def_fun->lineCount;
					v.value.ifFrame.state = IF_CONT;
					pop(&def_shadow_st);
				 	push(&def_shadow_st, v);
				}
				else if (p->children[p->childCount-1]->type == FI) {
					Value top = peek(&def_shadow_st);
        			if (top.type != IF_FRAME) {
            			fprintf(stderr, "not in a if statement\n");
						free_tree(p);
						free(tokens);
						ret.type = INT_VAL;
						goto repl;
					}
					pop(&def_shadow_st);
				}
				def_fun->lineCount++;
				goto repl;
			}
			if (p->childCount > 1 && find_lineNum(prog, p->children[0]->token->literal.intValue)) {
				fprintf(stderr, "duplicate lineNum\n");
				free_tree(p);
				free(tokens);
				ret.type = INT_VAL;
				goto repl;
			}
			prog.lines = realloc(prog.lines,
				(prog.lineCount + 1) * sizeof (ParseTreeNode *));
			if (prog.lines == NULL) {
				perror("Memory allocation failed");
				exit(EXIT_FAILURE);
			}
			prog.shadow_st = realloc(prog.shadow_st,
				(prog.lineCount + 1) * sizeof (Stack));
			if (prog.shadow_st == NULL) {
				perror("Memory allocation failed");
				exit(EXIT_FAILURE);
			}
			prog.shadow_st[prog.lineCount].size = -1;
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
						if (if_st.size > 0)
                        	if_state = if_st.st[if_st.size-1].value.ifFrame.state;
                    	else
                        	if_state = IF_BEFORE;
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
