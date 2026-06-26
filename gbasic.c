#include "gbasic.h"

Program prog;
int pc;
int expectedLineNum;

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
	prog.lines = (ParseTreeNode **)calloc(16384, sizeof(ParseTreeNode *));
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
	printf(">");
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
		if (find_lineNum(prog, p->children[0]->token->literal.intValue)) {
			fprintf(stderr, "duplicate lineNum\n");
			free_tree(p);
			free(tokens);
			ret.type = INT_VAL;
			goto repl;
		}
		if (p && !ctx.err) {
			prog.lines[prog.lineCount++] = p;
			if (pc < 0) {
				if (p->children[0]->token->literal.intValue == expectedLineNum) {
					pc = lineNum_to_pc(expectedLineNum);
				}
			}
			while (pc >= 0 && pc < prog.lineCount) {
				if (__glibc_unlikely(ret.type == ERR_VAL)) {
					prog.lineCount--;
					free_tree(p);
					free(tokens);
					ret.type = INT_VAL;
					goto repl;
				}
				ret = evalLine(*prog.lines[pc]);
			}
		}
		p = NULL;
		goto repl;
	} else {
		free(tokens);
	}
}