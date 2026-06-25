#include "gbasic.h"

Program prog;
int pc;

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

static bool depth_arr[256];
void print_tree(ParseTreeNode *t, unsigned int depth)
{
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
	ParseTreeNode *p = NULL;
	ParserContext ctx;
	prog.lineCount = 0;
	prog.lines = (ParseTreeNode **)calloc(16384, sizeof(ParseTreeNode *));
	if (prog.lines == NULL) {
		perror("Memory allocation failed");
		exit(EXIT_FAILURE);
	}
	pc = 0;
    init_eval();
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
	int len = lexer(str, tokens, prog.lineCount);
	if (len > 0) {
	    ctx.tokens = tokens;
		ctx.tokenLen = len;
		ctx.len = 0;
	    ctx.tokenPtr = ctx.tokens;
		ctx.err = false;
		p = parseLine(&ctx);
		if (p && !ctx.err) {
			prog.lines[prog.lineCount++] = p;
			while (pc < prog.lineCount) {
				evalLine(*prog.lines[pc]);
			}
		}
		p = NULL;
		goto repl;
	}
}