// Copyright (C) 2026 leo z <zlx20010815@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gbasic.h"

#define ERR(str, err) {fprintf(stderr, "%s\n", str); return ERRVAL(err);}
#define ERRVAL(err) ((Value) {.type = ERR_VAL, .value.errVal = err})
#define IS_ERR(x) (x.type == ERR_VAL)
#define ERR_RETURN_EVAL(err) {if (__glibc_unlikely(IS_ERR(err))) return ERRVAL(err.value.errVal);}

static VarListNode varList;
static Stack st;

int lineNum_to_pc(int lineNum)
{
    for (int i = 0; i < prog.lineCount; i++) {
        if (prog.lines[i]->children[0]->token->literal.intValue == lineNum)
            return i;
    }
    return -1;
}

void init_eval()
{
    varList.next = NULL;
    varList.var.name = NULL;
    st.size = 0;
}

static Value retriveVar(char *name) {
    VarListNode *cur = &varList;
    Value v;
    while (cur->next != NULL) {
        cur = cur->next;
        if (strcasecmp(cur->var.name, name) == 0) {
            v.type = cur->var.val.type;
            v.value = cur->var.val.value;
            return v;
        }
    }
    ERR("variable not found", VAR_NOT_FOUND);
}

static Value modVar(char *name, Value val) {
    VarListNode *cur = &varList;
    while (cur->next != NULL) {
        cur = cur->next;
        if (strcasecmp(cur->var.name, name) == 0) {
            cur->var.val.type = val.type;
            cur->var.val.value = val.value;
            return val;
        }
    }
    ERR("variable not found", VAR_NOT_FOUND);
}

static Value insertVar(char *name, Value val)
{
    VarListNode *cur = &varList;
    VarListNode *v = malloc(sizeof(VarListNode));
    v->var.name = name;
    v->var.val = val;
    while (cur->next != NULL) {
        cur = cur->next;
        if (strcasecmp(cur->var.name, name) == 0) {
            return modVar(name, val);
        }
    }
    v->next = varList.next;
    varList.next = v;
    return val;
}

static Value delVar(char *name)
{
    VarListNode *cur = &varList;
    VarListNode *prev;
    Value v;
    while (cur->next != NULL) {
        prev = cur;
        cur = cur->next;
        if (strcasecmp(cur->var.name, name) == 0) {
            v.type = cur->var.val.type;
            v.value = cur->var.val.value;
            prev->next = cur->next;
            free(cur);
        }
    }
    ERR("no variables of this name", VAR_NOT_FOUND);
}

static Value push(Stack *st, Value val)
{
    if (__glibc_unlikely(st->size == STASK_SIZE)) {
        ERR("stack overflow", STACK_OVERFLOW);
    }
    st->st[st->size++] = val;
    return val;
}

static Value pop(Stack *st)
{
    if (__glibc_unlikely(st->size == 0)) {
        ERR("stack underflow", STACK_UNDERFLOW);
    }
    return st->st[(st->size--) - 1];
}

static Value peek(Stack *st)
{
    if (__glibc_unlikely(st->size == 0)) {
        return ERRVAL(STACK_UNDERFLOW);
    }
    return st->st[st->size - 1];
}

static void printVal(Value val)
{
    switch (val.type) {
        case BOOL_VAL:
            if (val.value.boolVal == true)
                printf("TRUE\n");
            else if (val.value.boolVal == false)
                printf("FALSE\n");
            break;
        case INT_VAL:
            printf("%d\n", val.value.intVal);
            break;
        case FLOAT_VAL:
            printf("%f\n", val.value.floatVal);
            break;
        case STRING_VAL:
            printf("%s\n", val.value.string);
            break;
    }
}

Value evalLine(ParseTreeNode node)
{
    ParseTreeNode *statement = node.children[1];
    switch (statement->type) {
        case LET:
            return evalLet(*statement);
        case PRINT:
            return evalPrint(*statement);
        case IF:
            return evalIf(*statement);
        case FOR:
            return evalFor(*statement);
        case NEXT:
            return evalNext(*statement);
        case GOTO:
            return evalGoto(*statement);
        default:
            ERR("unknown statement", UNKNOWN_STATEMENT);
    }
}

Value evalLet(ParseTreeNode node)
{
    char *name = node.children[0]->token->lexeme;
    Value v = evalExpr(*node.children[2]);
    if (node.token == NULL) {
        pc++;
        return modVar(name, v);
    } else {
        pc++;
        return insertVar(name, v);
    }
    pc++;
    return v;
}

Value evalPrint(ParseTreeNode node)
{
    Value ret;
    ParseTreeNode *list = node.children[0];
    int cnt = list->childCount;
    for (int i = 0; i < cnt; i++) {
        ret = evalExpr(*list->children[i]);
        if (IS_ERR(ret))
            ERR("eval err", ret.value.errVal);
        printVal(ret);
    }
    pc++;
    return ret;
}

Value evalIf(ParseTreeNode node)
{
    ParseTreeNode *expr = node.children[0];
    ParseTreeNode *line = node.children[1];
    Value v = evalExpr(*expr);
    bool flag = false;
    enum ValueType t = v.type;
    switch (t) {
        case BOOL_VAL:
            flag = v.value.boolVal;
            break;
        case INT_VAL:
            flag = v.value.intVal;
            break;
        case FLOAT_VAL:
            flag = v.value.floatVal;
            break;
        default:
            ERR("incompatible types", INCOMPATIBLE_TYPES);
    }
    if (flag) {
        pc = lineNum_to_pc(line->token->literal.intValue);
        if (pc == -1)
            expectedLineNum = line->token->literal.intValue;
    }
    else
        pc++;
    return v;
}

Value evalFor(ParseTreeNode node)
{
    char *id = node.children[0]->token->lexeme;
    Value from = evalExpr(*node.children[2]);
    ERR_RETURN_EVAL(from);
    Value to = evalExpr(*node.children[3]);
    Value step;
    if (node.children[4] != NULL) {
        step = evalExpr(*node.children[4]);
        ERR_RETURN_EVAL(step);
    } else {
        step = (Value) {
            .type = INT_VAL,
            .value.intVal = 1,
        };
    }
    Value ctx = {
        .type = FOR_CTX,
        .value.forCtx = {
            .identi = id,
            .pc = pc,
            .next = -1,
            .step.intStep = step.value.intVal,
        },
    };
    Value top = peek(&st);
    if (top.type == ERR_VAL
        && top.value.errVal == STACK_UNDERFLOW) {
        push(&st, ctx);
        insertVar(id, from);
        pc++;
    }
    else if (__glibc_likely(top.type == FOR_CTX)) {
        if (__glibc_likely(top.value.forCtx.pc == pc)) {
            if (__glibc_likely(strcasecmp(top.value.forCtx.identi, id) == 0)) {
                if (__glibc_unlikely(retriveVar(top.value.forCtx.identi).value.intVal > to.value.intVal
                    || retriveVar(top.value.forCtx.identi).value.intVal < from.value.intVal)) {
                    pc = top.value.forCtx.next;
                    pop(&st);
                } else {
                    pc++;
                }
            } else {
                ERR("stack error", ERR_VAL_NULL);
            }
        } else {
            push(&st, ctx);
            insertVar(id, from);
            pc++;
        }
    } else {
        push(&st,ctx);
        insertVar(id, from);
        pc++;
    }
}

Value evalNext(ParseTreeNode node)
{
    Value top = peek(&st);
    if (top.type != FOR_CTX)
        ERR("NOT_IN_FOR", ERR_VAL_NULL);
    char *id = node.children[0]->token->lexeme;
    Value v = retriveVar(id);
    ERR_RETURN_EVAL(v);
    Value newVar = {
        .type = INT_VAL,
        .value.intVal = v.value.intVal + top.value.forCtx.step.intStep,
    };
    modVar(id, newVar);
    if (top.value.forCtx.next == -1) {
        ERR_RETURN_EVAL(pop(&st));
        top.value.forCtx.next = pc + 1;
        ERR_RETURN_EVAL(push(&st, top));
    }
    pc = top.value.forCtx.pc;
    return newVar;
}

Value evalGoto(ParseTreeNode node)
{
    pc = lineNum_to_pc(node.children[0]->token->literal.intValue);
    if (pc == -1)
        expectedLineNum = node.children[0]->token->literal.intValue;
}

Value evalExpr(ParseTreeNode node)
{
    return evalOrExpr(node);
}

Value evalOrExpr(ParseTreeNode node)
{
    unsigned int cnt = node.childCount;
    if (cnt == 1) {
        return evalAndExpr(*node.children[0]);
    } else {
        Value v = evalAndExpr(*node.children[0]);
        ERR_RETURN_EVAL(v);
        for (int i = 1; i < cnt; i += 2) {
            if (node.children[i]->type == OR_OP) {
                Value tmp = evalAndExpr(*node.children[i+1]);
                ERR_RETURN_EVAL(tmp);
                v.type = BOOL_VAL;
                if (v.type == BOOL_VAL) {
                    if (tmp.type == BOOL_VAL)
                        v.value.boolVal = (v.value.boolVal || tmp.value.boolVal);
                    else if (tmp.type == INT_VAL)
                        v.value.boolVal = (v.value.boolVal || tmp.value.intVal);
                    else if (tmp.type == FLOAT_VAL)
                            v.value.boolVal = (v.value.boolVal || tmp.value.floatVal);
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (v.type == INT_VAL) {
                    if (tmp.type == BOOL_VAL)
                        v.value.boolVal = (v.value.intVal || tmp.value.boolVal);
                    else if (tmp.type == INT_VAL)
                        v.value.boolVal = (v.value.intVal || tmp.value.intVal);
                    else if (tmp.type == FLOAT_VAL)
                            v.value.boolVal = (v.value.intVal || tmp.value.floatVal);
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (v.type == FLOAT_VAL) {
                    if (tmp.type == BOOL_VAL)
                        v.value.boolVal = (v.value.floatVal || tmp.value.boolVal);
                    else if (tmp.type == INT_VAL)
                        v.value.boolVal = (v.value.floatVal || tmp.value.intVal);
                    else if (tmp.type == FLOAT_VAL)
                            v.value.boolVal = (v.value.floatVal || tmp.value.floatVal);
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
            } else {
                ERR("not an or operation", INCOMPATIBLE_TYPES);
            }
        }
        return v;
    }
}

Value evalAndExpr(ParseTreeNode node)
{
    unsigned int cnt = node.childCount;
    if (cnt == 1) {
        return evalRelExpr(*node.children[0]);
    } else {
        Value v = evalRelExpr(*node.children[0]);
        ERR_RETURN_EVAL(v);
        for (int i = 1; i < cnt; i += 2) {
            if (node.children[i]->type == AND_OP) {
                Value tmp = evalRelExpr(*node.children[i+1]);
                ERR_RETURN_EVAL(tmp);
                v.type = BOOL_VAL;
                if (v.type == BOOL_VAL) {
                    if (tmp.type == BOOL_VAL)
                        v.value.boolVal = (v.value.boolVal && tmp.value.boolVal);
                    else if (tmp.type == INT_VAL)
                        v.value.boolVal = (v.value.boolVal && tmp.value.intVal);
                    else if (tmp.type == FLOAT_VAL)
                            v.value.boolVal = (v.value.boolVal && tmp.value.floatVal);
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (v.type == INT_VAL) {
                    if (tmp.type == BOOL_VAL)
                        v.value.boolVal = (v.value.intVal && tmp.value.boolVal);
                    else if (tmp.type == INT_VAL)
                        v.value.boolVal = (v.value.intVal && tmp.value.intVal);
                    else if (tmp.type == FLOAT_VAL)
                            v.value.boolVal = (v.value.intVal && tmp.value.floatVal);
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (v.type == FLOAT_VAL) {
                    if (tmp.type == BOOL_VAL)
                        v.value.boolVal = (v.value.floatVal && tmp.value.boolVal);
                    else if (tmp.type == INT_VAL)
                        v.value.boolVal = (v.value.floatVal && tmp.value.intVal);
                    else if (tmp.type == FLOAT_VAL)
                            v.value.boolVal = (v.value.floatVal && tmp.value.floatVal);
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
            } else {
                ERR("not an add operation", INCOMPATIBLE_TYPES);
            }
        }
        return v;
    }
}

Value evalRelExpr(ParseTreeNode node)
{
    unsigned int cnt = node.childCount;
    if (cnt == 1) {
        return evalAddExpr(*node.children[0]);
    } else {
        Value v = evalAddExpr(*node.children[0]);
        ERR_RETURN_EVAL(v);
        for (int i = 1; i < cnt; i += 2) {
            if (node.children[i]->type == EQ || node.children[i]->type == LT
                || node.children[i]->type == GT || node.children[i]->type == LE
                || node.children[i]->type == GE) {
                TokenType t = node.children[i]->type;
                Value tmp = evalAddExpr(*node.children[i+1]);
                ERR_RETURN_EVAL(tmp);
                switch (t) {
                    case EQ:
                        if (v.type == BOOL_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.boolVal == tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.boolVal == tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.boolVal == tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                        else if (v.type == INT_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.intVal == tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.intVal == tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.intVal == tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                        break;
                    case LT:
                        if (v.type == BOOL_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.boolVal < tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.boolVal < tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.boolVal < tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                        else if (v.type == INT_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.intVal < tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.intVal < tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.intVal < tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                        break;
                    case GT:
                        if (v.type == BOOL_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.boolVal > tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.boolVal > tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.boolVal > tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                        else if (v.type == INT_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.intVal > tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.intVal > tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.intVal > tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                        break;
                    case LE:
                        if (v.type == BOOL_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.boolVal <= tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.boolVal <= tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.boolVal <= tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                        else if (v.type == INT_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.intVal <= tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.intVal <= tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.intVal <= tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                        break;
                    case GE:
                        if (v.type == BOOL_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.boolVal >= tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.boolVal >= tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.boolVal >= tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                        else if (v.type == INT_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.intVal >= tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.intVal >= tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.intVal >= tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                }
            }
        }
        return v;
    }
}

Value evalAddExpr(ParseTreeNode node)
{
    unsigned int cnt = node.childCount;
    if (cnt == 1) {
        return evalMulExpr(*node.children[0]);
    } else {
        Value v = evalMulExpr(*node.children[0]);
        ERR_RETURN_EVAL(v);
        for (int i = 1; i < cnt; i += 2) {
            enum ValueType t = v.type;
            if (node.children[i]->type == ADD_OP) {
                Value tmp = evalMulExpr(*node.children[i+1]);
                ERR_RETURN_EVAL(tmp);
                v.type = INT_VAL;
                if (t == BOOL_VAL) {
                    if (tmp.type == BOOL_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "+") == 0)
                            v.value.intVal = (v.value.boolVal + tmp.value.boolVal);
                        else if (strcmp(node.children[i]->token->lexeme, "-") == 0)
                            v.value.intVal = (v.value.boolVal - tmp.value.boolVal);
                    }
                    else if (tmp.type == INT_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "+") == 0)
                            v.value.intVal = (v.value.boolVal + tmp.value.intVal);
                        else if (strcmp(node.children[i]->token->lexeme, "-") == 0)
                            v.value.intVal = (v.value.boolVal - tmp.value.intVal);
                    }
                    else if (tmp.type == FLOAT_VAL) {
                        v.type = FLOAT_VAL;
                        if (strcmp(node.children[i]->token->lexeme, "+") == 0)
                            v.value.floatVal = (v.value.boolVal + tmp.value.floatVal);
                        else if (strcmp(node.children[i]->token->lexeme, "-") == 0)
                            v.value.floatVal = (v.value.boolVal - tmp.value.floatVal);
                    }
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (t == INT_VAL) {
                    if (tmp.type == BOOL_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "+") == 0)
                            v.value.intVal = (v.value.intVal + tmp.value.boolVal);
                        else if (strcmp(node.children[i]->token->lexeme, "-") == 0)
                            v.value.intVal = (v.value.intVal - tmp.value.boolVal);
                    }
                    else if (tmp.type == INT_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "+") == 0)
                            v.value.intVal = (v.value.intVal + tmp.value.intVal);
                        else if (strcmp(node.children[i]->token->lexeme, "-") == 0)
                            v.value.intVal = (v.value.intVal - tmp.value.intVal);
                    }
                    else if (tmp.type == FLOAT_VAL) {
                        v.type = FLOAT_VAL;
                        if (strcmp(node.children[i]->token->lexeme, "+") == 0)
                            v.value.floatVal = (v.value.intVal + tmp.value.floatVal);
                        else if (strcmp(node.children[i]->token->lexeme, "-") == 0)
                            v.value.floatVal = (v.value.intVal - tmp.value.floatVal);
                    }
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (t == FLOAT_VAL) {
                    v.type = FLOAT_VAL;
                    if (tmp.type == BOOL_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "+") == 0)
                            v.value.floatVal = (v.value.floatVal + tmp.value.boolVal);
                        else if (strcmp(node.children[i]->token->lexeme, "-") == 0)
                            v.value.floatVal = (v.value.floatVal - tmp.value.boolVal);
                    }
                    else if (tmp.type == INT_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "+") == 0)
                            v.value.floatVal = (v.value.floatVal + tmp.value.intVal);
                        else if (strcmp(node.children[i]->token->lexeme, "-") == 0)
                            v.value.floatVal = (v.value.floatVal - tmp.value.intVal);
                    }
                    else if (tmp.type == FLOAT_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "+") == 0)
                            v.value.floatVal = (v.value.floatVal + tmp.value.floatVal);
                        else if (strcmp(node.children[i]->token->lexeme, "-") == 0)
                            v.value.floatVal = (v.value.floatVal - tmp.value.floatVal);
                    }
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
            } else {
                ERR("not an add operation", INCOMPATIBLE_TYPES);
            }
        }
        return v;
    }
}

Value evalMulExpr(ParseTreeNode node)
{
    unsigned int cnt = node.childCount;
    if (cnt == 1) {
        return evalUnary(*node.children[0]);
    } else {
        Value v = evalUnary(*node.children[0]);
        ERR_RETURN_EVAL(v);
        for (int i = 1; i < cnt; i += 2) {
            if (node.children[i]->type == MUL_OP) {
                enum ValueType t = v.type;
                Value tmp = evalUnary(*node.children[i+1]);
                ERR_RETURN_EVAL(tmp);
                v.type = INT_VAL;
                if (t == BOOL_VAL) {
                    if (tmp.type == BOOL_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "*") == 0)
                            v.value.intVal = (v.value.boolVal * tmp.value.boolVal);
                        else if (strcmp(node.children[i]->token->lexeme, "/") == 0)
                            v.value.intVal = (v.value.boolVal / tmp.value.boolVal);
                    }
                    else if (tmp.type == INT_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "*") == 0)
                            v.value.intVal = (v.value.boolVal * tmp.value.intVal);
                        else if (strcmp(node.children[i]->token->lexeme, "/") == 0)
                            v.value.intVal = (v.value.boolVal / tmp.value.intVal);
                    }
                    else if (tmp.type == FLOAT_VAL) {
                        v.type = FLOAT_VAL;
                        if (strcmp(node.children[i]->token->lexeme, "*") == 0)
                            v.value.floatVal = (v.value.boolVal * tmp.value.floatVal);
                        else if (strcmp(node.children[i]->token->lexeme, "/") == 0)
                            v.value.floatVal = (v.value.boolVal / tmp.value.floatVal);
                    }
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (t == INT_VAL) {
                    if (tmp.type == BOOL_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "*") == 0)
                            v.value.intVal = (v.value.intVal * tmp.value.boolVal);
                        else if (strcmp(node.children[i]->token->lexeme, "/") == 0)
                            v.value.intVal = (v.value.intVal / tmp.value.boolVal);
                    }
                    else if (tmp.type == INT_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "*") == 0)
                            v.value.intVal = (v.value.intVal * tmp.value.intVal);
                        else if (strcmp(node.children[i]->token->lexeme, "/") == 0)
                            v.value.intVal = (v.value.intVal / tmp.value.intVal);
                    }
                    else if (tmp.type == FLOAT_VAL) {
                        v.type = FLOAT_VAL;
                        if (strcmp(node.children[i]->token->lexeme, "*") == 0)
                            v.value.floatVal = (v.value.intVal * tmp.value.floatVal);
                        else if (strcmp(node.children[i]->token->lexeme, "/") == 0)
                            v.value.floatVal = (v.value.intVal / tmp.value.floatVal);
                    }
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (t == FLOAT_VAL) {
                    v.type = FLOAT_VAL;
                    if (tmp.type == BOOL_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "*") == 0)
                            v.value.floatVal = (v.value.floatVal * tmp.value.boolVal);
                        else if (strcmp(node.children[i]->token->lexeme, "/") == 0)
                            v.value.floatVal = (v.value.floatVal / tmp.value.boolVal);
                    }
                    else if (tmp.type == INT_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "*") == 0)
                            v.value.floatVal = (v.value.floatVal * tmp.value.intVal);
                        else if (strcmp(node.children[i]->token->lexeme, "/") == 0)
                            v.value.floatVal = (v.value.floatVal / tmp.value.intVal);
                    }
                    else if (tmp.type == FLOAT_VAL) {
                        if (strcmp(node.children[i]->token->lexeme, "*") == 0)
                            v.value.floatVal = (v.value.floatVal * tmp.value.floatVal);
                        else if (strcmp(node.children[i]->token->lexeme, "/") == 0)
                            v.value.floatVal = (v.value.floatVal / tmp.value.floatVal);
                    }
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
            }
        }
        return v;
    }
}

Value evalUnary(ParseTreeNode node)
{
    unsigned int cnt = node.childCount;
    if (cnt == 1) {
        return evalPrimary(*node.children[0]);
    } else {
        Value v = evalPrimary(*node.children[1]);
        ERR_RETURN_EVAL(v);
        enum ValueType t = v.type;
        if (node.children[0]->type == UNARY_OP) {
            if (strcmp(node.children[0]->token->lexeme, "+") == 0) {
                v.type = INT_VAL;
                if (t == BOOL_VAL)
                    v.value.intVal = +v.value.boolVal;
                else if (t == INT_VAL)
                    v.value.intVal = +v.value.intVal;
                else if (t == FLOAT_VAL) {
                    v.type = FLOAT_VAL;
                    v.value.floatVal = +v.value.floatVal;
                }
                else
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
            }
            else if (strcmp(node.children[0]->token->lexeme, "-") == 0) {
                v.type = INT_VAL;
                if (t == BOOL_VAL)
                    v.value.intVal = -v.value.boolVal;
                else if (t == INT_VAL)
                    v.value.intVal = -v.value.intVal;
                else if (t == FLOAT_VAL) {
                    v.type = FLOAT_VAL;
                    v.value.floatVal = -v.value.floatVal;
                }
                else
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
            }
            else if (strcasecmp(node.children[0]->token->lexeme, "NOT") == 0) {
                v.type = BOOL_VAL;
                if (t == BOOL_VAL)
                    v.value.boolVal = !v.value.boolVal;
                else if (t == INT_VAL)
                    v.value.boolVal = !v.value.intVal;
                else if (t == FLOAT_VAL) {
                    v.value.boolVal = !v.value.floatVal;
                }
                else
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
            }
            else
                ERR("unknown unary operations", INCOMPATIBLE_TYPES);
        }
        return v;
    }
}

Value evalPrimary(ParseTreeNode node)
{
    Value v;
    if (node.children[0]->type == KEYWORD_TOKEN) {
        v.type = BOOL_VAL;
        if (strcasecmp(node.token->lexeme, "FALSE") == 0)
            v.value.boolVal = false;
        else if (strcasecmp(node.token->lexeme, "TRUE") == 0)
            v.value.boolVal = true;
        else
            ERR("unknown bool value", UNKNOWN_BOOL_VALUE);
    }
    else if (node.children[0]->type == INTEGER) {
        v.type = INT_VAL;
        v.value.intVal = node.children[0]->token->literal.intValue;
    }
    else if (node.children[0]->type == FLOAT) {
        v.type = FLOAT_VAL;
        v.value.floatVal = node.children[0]->token->literal.floatValue;
    }
    else if (node.children[0]->type == STRING) {
        v.type = STRING_VAL;
        v.value.string.str = node.children[0]->token->literal.string;
        v.value.string.refcnt = 1;
    }
    else if (node.children[0]->type == OR_EXPR) {
        Value tmp = evalOrExpr(*node.children[0]);
        v.type = tmp.type;
        v.value = tmp.value;
    }
    else if (node.children[0]->type == IDENTI) {
        Value tmp = retriveVar(node.children[0]->token->lexeme);
        v.type = tmp.type;
        v.value = tmp.value;
    }
    return v;
}
