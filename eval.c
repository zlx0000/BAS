#include "gbasic.h"

#define ERR(str) {fprintf(stderr, str);}
#define ERRVAL ((Value) {.type = ERR_VAL, .value.intVal=-1})
#define IS_ERR(x) x.type == ERR_VAL

static Value push(Stack st, Value val)
{
    if (st.size == STASK_SIZE) {
        ERR("stack overflow");
        return ERRVAL;
    }
    st.st[st.size++] = val;
    return val;
}

static Value pop(Stack st)
{
    if (st.size == 0) {
        ERR("stack overflow");
        return ERRVAL;
    }
    return st.st[st.size--];
}

static print_val(Value val)
{
    switch (val.type) {
        case INT_VAL:
            printf("%d ", val.value.intVal);
            break;
        case FLOAT_VAL:
            printf("%f ", val.value.floatVal);
            break;
        case STRING_VAL:
            printf("%s ", val.value.stringVal);
            break;
    }
}

Value evalLine(ParseTreeNode node)
{
    ParseTreeNode *statement = node.children[1];
    switch (statement->type) {
        case LET:
            //evalLet(*statement);
            break;
        case PRINT:
            evalPrint(*statement);
            break;
        default:
            ERR("unknown statement");
    }
}

Value evalPrint(ParseTreeNode node)
{
    Value ret;
    ParseTreeNode *list = node.children[0];
    int cnt = list->childCount;
    for (int i = 0; i < cnt; i++) {
        ret = evalExpr(*list->children[i]);
        if (IS_ERR(ret))
            return ret;
        
    }
}