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

int main(int argc, char **argv)
{
	FILE *fp = NULL;
	char str[1024];
	int line = 1;
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
		Program *prog = (Program *)calloc(1, sizeof(Program));
		if (prog == NULL) {
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
	    ctx.prog = prog;
		ctx.prog->lineCount = 1;
		ctx.err = false;
		ParseTreeNode *p = parseLine(&ctx);
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
    ctx.prog = prog;
	ctx.prog->lineCount = 1;
	ctx.err = false;
    ParseTreeNode *p = parseLine(&ctx);
	if (end == bf + size)
		return 0;
	goto next;
}
