// Copyright (C) 2026 leo z <zlx20010815@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gbasic.h"
#include <math.h>
#include <unistd.h>

#define ERR(str, err) {fprintf(stderr, "%s\n", str); return ERRVAL(err);}
#define ERRVAL(err) ((Value) {.type = ERR_VAL, .value.errVal = err})
#define IS_ERR(x) (x.type == ERR_VAL)
#define ERR_RETURN_EVAL(err) {if (__unlikely(IS_ERR(err))) return ERRVAL(err.value.errVal);}
#define DEF_VAL ((Value){.type = INT_VAL, .value.intVal = 0})

static VarListNode varList;
static ArrPtrList arrPtrList;
static ArrPtrList accessdArr;
static Stack for_st;
Stack if_st;
enum If_State if_state = IF_BEFORE;

int lineNum_to_pc(int lineNum)
{
    for (int i = 0; i < prog.lineCount; i++) {
        if (prog.lines[i]->childCount > 1 && prog.lines[i]->children[0]->token->literal.intValue == lineNum)
            return i;
    }
    return -1;
}

bool is_if_else_or_fi(ParseTreeNode *node)
{
    unsigned int i = node->childCount - 1;
    return ((node->children[i]->type == IF
             && node->children[i]->childCount == 1)
        ||node->children[i]->type == ELSE
        || node->children[i]->type == FI);
}

void init_eval()
{
    varList.next = NULL;
    varList.var.name = NULL;
    arrPtrList.ptr = NULL;
    arrPtrList.next = NULL;
    accessdArr.ptr = NULL;
    accessdArr.next = NULL;
    for_st.size = 0;
    if_st.size = 0;
    shadow_st.size = 0;
}

bool findArrPtr(Value *ptr, ArrPtrList *list) {
    ArrPtrList *cur = list;
    while (cur->next != NULL) {
        cur = cur->next;
        if (cur->ptr == ptr)
            return true;
    }
    return false;
}

Value insertArrPtr(Value *ptr, ArrPtrList *list)
{
    ArrPtrList *cur = list;
    ArrPtrList *v = calloc(1, sizeof(ArrPtrList));
    v->ptr = ptr;
    while (cur->next != NULL) {
        cur = cur->next;
        if (cur->ptr == ptr) {
            return ERRVAL(VAR_ALREADY_EXIST);
        }
    }
    v->next = list->next;
    list->next = v;
    return DEF_VAL;
}

Value delArrPtr(Value *ptr, ArrPtrList *list)
{
    ArrPtrList *cur = list;
    ArrPtrList *prev;
    Value v;
    while (cur->next != NULL) {
        prev = cur;
        cur = cur->next;
        if (cur->ptr == ptr) {
            prev->next = cur->next;
            free(cur);
            return DEF_VAL;
        }
    }
    ERR("no such pointer", VAR_NOT_FOUND);
}

void free_arr_list(ArrPtrList *ptr)
{
    ArrPtrList *cur = ptr;
    ArrPtrList *next = cur->next;
    while (next != NULL) {
        cur = next;
        next = cur->next;
        free(cur);
    }
    ptr->next = NULL;
}

Value retriveVar(char *name) {
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

bool findVar(char *name) {
    VarListNode *cur = &varList;
    while (cur->next != NULL) {
        cur = cur->next;
        if (strcasecmp(cur->var.name, name) == 0)
            return true;
    }
    return false;
}

Value modVar(char *name, Value val) {
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

Value insertVar(char *name, Value val)
{
    VarListNode *cur = &varList;
    while (cur->next != NULL) {
        cur = cur->next;
        if (strcasecmp(cur->var.name, name) == 0) {
            return modVar(name, val);
        }
    }
    VarListNode *v = calloc(1, sizeof(VarListNode));
    v->var.name = name;
    v->var.val = val;
    v->next = varList.next;
    varList.next = v;
    return val;
}

Value insertVar_strict(char *name, Value val)
{
    VarListNode *cur = &varList;
    VarListNode *v = malloc(sizeof(VarListNode));
    v->var.name = name;
    v->var.val = val;
    while (cur->next != NULL) {
        cur = cur->next;
        if (strcasecmp(cur->var.name, name) == 0) {
            return ERRVAL(VAR_ALREADY_EXIST);
        }
    }
    v->next = varList.next;
    varList.next = v;
    return val;
}

Value delVar(char *name)
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
            return DEF_VAL;
        }
    }
    ERR("no variables of this name", VAR_NOT_FOUND);
}

Value push(Stack *st, Value val)
{
    if (__unlikely(st->size == STASK_SIZE)) {
        ERR("stack overflow", STACK_OVERFLOW);
    }
    st->st[st->size++] = val;
    return val;
}

Value pop(Stack *st)
{
    if (__unlikely(st->size == 0)) {
        ERR("stack underflow", STACK_UNDERFLOW);
    }
    return st->st[(st->size--) - 1];
}

Value peek(Stack *st)
{
    if (__unlikely(st->size == 0)) {
        return ERRVAL(STACK_UNDERFLOW);
    }
    return st->st[st->size - 1];
}

void copy_stack(Stack *src, Stack* dst) {
    dst->size = src->size;
    for (int i = 0; i < src->size; i++) {
        dst->st[i] = src->st[i];
    }
}

void printArr(Value *val)
{
    Value ret = insertArrPtr(val, &accessdArr);
    printf("[");
    for (int i = 0; i < val->value.arr.size; i++)
    {
        Value *ptr = val->value.arr.ptr + i;
        switch (ptr->type) {
            case BOOL_VAL:
                if (ptr->value.boolVal == true)
                    printf("TRUE");
                else if (ptr->value.boolVal == false)
                    printf("FALSE");
                break;
            case INT_VAL:
                printf("%d", ptr->value.intVal);
                break;
            case FLOAT_VAL:
                printf("%f", ptr->value.floatVal);
                break;
            case STRING_VAL:
                printf("\"%s\"", ptr->value.string.str);
                break;
            case ARR_VAL:
                if ((ret.type == ERR_VAL
                    && ret.value.errVal == VAR_ALREADY_EXIST)
                    || findArrPtr(ptr, &accessdArr))
                    printf("...");
                else
                    printArr(ptr);
                break;
        }
        if (i != val->value.arr.size - 1)
            printf(", ");
        else
            printf("]");
    }
}

void printVal(Value val)
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
            printf("%s", val.value.string.str);
            break;
        case ARR_VAL:
            printArr(&val);
            free_arr_list(&accessdArr);
            printf("\n");
    }
}

void putChar(Value val)
{
    switch (val.type) {
        case BOOL_VAL:
            putchar(val.value.boolVal);
            break;
        case INT_VAL:
            putchar(val.value.intVal);
            break;
        case FLOAT_VAL:
            putchar(val.value.floatVal);
            break;
        case STRING_VAL:
            putchar(val.value.string.str[0]);
            break;
        case ARR_VAL:
            for (int i = 0; i < val.value.arr.size; i++)
            {
                Value *ptr = val.value.arr.ptr + i;
                switch (ptr->type) {
                    case BOOL_VAL:
                        putchar(val.value.boolVal);
                        break;
                   case INT_VAL:
                        putchar(val.value.intVal);
                        break;
                    case FLOAT_VAL:
                        putchar(val.value.floatVal);
                        break;
                    case STRING_VAL:
                        putchar(val.value.string.str[0]);
                        break;
                }
            }
    }
}

Value evalLine(ParseTreeNode *node)
{
    ParseTreeNode *statement = node->children[node->childCount-1];
    switch (statement->type) {
        case LET:
            return evalLet(statement);
        case DIM:
            return evalDim(statement);
        case PRINT:
            return evalPrint(statement);
        case IF:
            return evalIf(statement);
        case ELSE:
            return evalElse(statement);
        case FI:
            return evalFi(statement);
        case FOR:
            return evalFor(statement);
        case NEXT:
            return evalNext(statement);
        case GOTO:
            return evalGoto(statement);
        case PUTCHAR:
            return evalPutChar(statement);
        case SLEEP:
            return evalSleep(statement);
        case CLEAR:
            return evalClear(statement);
        case HOME:
            return evalHome(statement);
        case FREE:
            return evalFree(statement);
        case DEL:
            return evalDel(statement);
        default:
            ERR("unknown statement", UNKNOWN_STATEMENT);
    }
}

Value evalLet(ParseTreeNode *node)
{
    char *name = node->children[0]->token->lexeme;
    if (strcasecmp(name, "COS") == 0
        || strcasecmp(name, "SIN") == 0
        || strcasecmp(name, "COSF") == 0
        || strcasecmp(name, "SINF") == 0
        || strcasecmp(name, "TAN") == 0
        || strcasecmp(name, "TANF") == 0
        || strcasecmp(name, "EXP") == 0
        || strcasecmp(name, "INT") == 0
        || strcasecmp(name, "FLOAT") == 0
        || strcasecmp(name, "NEW") == 0) {
        ERR("cannot use built-in names", ERR_VAL_NULL);
    }
    if (node->children[0]->childCount == 0
        || node->children[0]->children[0] == NULL) {
        char *name = node->children[0]->token->lexeme;
        Value v = evalExpr(node->children[2]);
        ERR_RETURN_EVAL(v);
        if (node->token == NULL) {
            pc++;
            return modVar(name, v);
        } else {
            pc++;
            return insertVar(name, v);
        }
        pc++;
        return v;
    } else {
        Value id = retriveVar(name);
        ERR_RETURN_EVAL(id);
        if (id.type != ARR_VAL)
            ERR("not an array", INCOMPATIBLE_TYPES);
        int i = 0;
        int cnt = node->children[0]->childCount;
        Value index_val = evalExpr(node->children[0]->children[i]);
        ERR_RETURN_EVAL(index_val);
        if (index_val.type != INT_VAL)
            ERR("index has to be INT type", INCOMPATIBLE_TYPES);
        int index = index_val.value.intVal;
        if (index >= id.value.arr.size)
            ERR("index out of range", INDEX_OUT_OF_RANGE);
        Value v = evalExpr(node->children[2]);
        ERR_RETURN_EVAL(v);
        Value *base = id.value.arr.ptr;
        Value *ptr = base + index;
        i++;
        while (i < cnt) {
            if (ptr->type != ARR_VAL)
                ERR("not an array", INCOMPATIBLE_TYPES);
            Value index_val = evalExpr(node->children[0]->children[i]);
            ERR_RETURN_EVAL(index_val);
            if (index_val.type != INT_VAL)
                ERR("index has to be INT type", INCOMPATIBLE_TYPES);
            int index = index_val.value.intVal;
            if (index >= ptr->value.arr.size)
                ERR("index out of range", INDEX_OUT_OF_RANGE);
            base = ptr->value.arr.ptr;
            ptr = base + index;
            i++;
        }
        ptr->type = v.type;
        ptr->value = v.value;
        pc++;
        return v;
    }
}

Value evalDim(ParseTreeNode *node)
{
    Value v;
    Value oldV;
    char *name = node->children[0]->token->lexeme;
    if (strcasecmp(name, "COS") == 0
        || strcasecmp(name, "SIN") == 0
        || strcasecmp(name, "COSF") == 0
        || strcasecmp(name, "SINF") == 0
        || strcasecmp(name, "TAN") == 0
        || strcasecmp(name, "TANF") == 0
        || strcasecmp(name, "EXP") == 0
        || strcasecmp(name, "INT") == 0
        || strcasecmp(name, "FLOAT") == 0
        || strcasecmp(name, "NEW") == 0) {
        ERR("cannot use built-in names", ERR_VAL_NULL);
    }
    v.type = ARR_VAL;
    if (node->children[0]->childCount == 0
        || node->children[0]->children[0] == NULL) {
        ERR("not an array", ERR_VAL_NULL);
    }
    Value size = evalExpr(node->children[0]->children[0]);
    if (size.type != INT_VAL)
        ERR("size has to be INT type", INCOMPATIBLE_TYPES);
    if (size.value.intVal <= 0) {
        ERR("array size can't be zero", ERR_VAL_NULL);
    }
    if (findVar(name)
        && !IS_ERR((oldV = retriveVar(name))) && oldV.type == ARR_VAL) {
        Value *tmp = (Value *)realloc(oldV.value.arr.ptr,
                    size.value.intVal * sizeof(Value));
        if (tmp == NULL)
            ERR("out of memory", OUT_OF_MEMORY);
        v.value.arr.ptr = tmp;
        if (oldV.value.arr.size < size.value.intVal) {
            memset(v.value.arr.ptr + oldV.value.arr.size, 0,
                    (size.value.intVal - oldV.value.arr.size) * sizeof(Value));
            for (int i = 0; i < (size.value.intVal - oldV.value.arr.size); i++) {
                //printf("f\n");
                (v.value.arr.ptr + oldV.value.arr.size + i)->type = INT_VAL;
            }
        }
        v.value.arr.size = size.value.intVal;
        pc++;
        return modVar(name, v);
    }
    v.value.arr.refcnt = 1;
    v.value.arr.ptr = (Value *)calloc(size.value.intVal, sizeof(Value));
    if (v.value.arr.ptr == NULL)
        ERR("out of memory", OUT_OF_MEMORY);
    v.value.arr.size = size.value.intVal;
    insertArrPtr(v.value.arr.ptr, &arrPtrList);
    for (int i = 0; i < size.value.intVal; i++) {
        v.value.arr.ptr[i].type = INT_VAL;
    }
    pc++;
    return insertVar(name, v);
}

Value evalPrint(ParseTreeNode *node)
{
    Value ret;
    ParseTreeNode *list = node->children[0];
    int cnt = list->childCount;
    for (int i = 0; i < cnt; i++) {
        ret = evalExpr(list->children[i]);
        if (IS_ERR(ret))
            ERR("eval err", ret.value.errVal);
        printVal(ret);
    }
    pc++;
    return ret;
}

Value evalPutChar(ParseTreeNode *node)
{
    Value ret;
    ParseTreeNode *list = node->children[0];
    int cnt = list->childCount;
    for (int i = 0; i < cnt; i++) {
        ret = evalExpr(list->children[i]);
        if (IS_ERR(ret))
            ERR("eval err", ret.value.errVal);
        putChar(ret);
    }
    pc++;
    return ret;
}

Value evalIf(ParseTreeNode *node)
{
    if (node->childCount == 1) {
        ParseTreeNode *expr = node->children[0];
        {
            Value frame = {
                .type = IF_FRAME,
                .value.ifFrame = {
                    .entry = pc,
                    .state = IF_SKIP_ELSE,
                },
            };
            push(&shadow_st, frame);
        }
        if (if_state == IF_EXPECTING_ELSE_OR_FI
            || if_state == IF_EXPECTING_FI) {
            Value frame = {
                .type = IF_FRAME,
                .value.ifFrame = {
                    .entry = pc,
                    .state = IF_EXPECTING_FI,
                },
            };
            if_state = IF_EXPECTING_FI;
            push(&if_st, frame);
            pc++;
            return DEF_VAL;
        }
        if (pc < 0) {
            return DEF_VAL;
        }
        Value v = evalExpr(expr);
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
            Value frame = {
                .type = IF_FRAME,
                .value.ifFrame = {
                    .entry = pc,
                    .state = IF_SKIP_ELSE,
                },
            };
            if_state = IF_SKIP_ELSE;
            push(&if_st, frame);
        } else {
            Value frame = {
                .type = IF_FRAME,
                .value.ifFrame = {
                    .entry = pc,
                    .state = IF_EXPECTING_ELSE_OR_FI
                },
            };
            if_state = IF_EXPECTING_ELSE_OR_FI;
            push(&if_st, frame);
        }
        pc++;
        return v;
    }
    else if (node->childCount == 2) {
        ParseTreeNode *expr = node->children[0];
        ParseTreeNode *line = node->children[1];
        Value v = evalExpr(expr);
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
            else {
                if (prog.shadow_st[pc].size >= 0) {
                    copy_stack(&prog.shadow_st[pc], &if_st);
                    copy_stack(&if_st, &shadow_st);
                    if_state = if_st.st[if_st.size-1].value.ifFrame.state;
                }
            }
        }
        else
            pc++;
        return v;
    }
}

Value evalElse(ParseTreeNode *node)
{
    {
        Value top = peek(&shadow_st);
        if (top.type != IF_FRAME)
            ERR("not in a if statement", INCOMPATIBLE_TYPES);
        Value frame = {
            .type = IF_FRAME,
            .value.ifFrame = {
                .entry = pc,
                .state = IF_CONT,
            },
        };
        pop(&shadow_st);
        push(&shadow_st, frame);
    }
    if (pc < 0) {
        return DEF_VAL;
    }
    Value top = peek(&if_st);
    if (top.type != IF_FRAME)
        ERR("not in a if statement", INCOMPATIBLE_TYPES);
    switch (top.value.ifFrame.state) {
        case IF_BEFORE:
        case IF_CONT:
            ERR("not in a IF statement", ERR_VAL_NULL);
        case IF_SKIP_ELSE: {
                Value frame = {
                    .type = IF_FRAME,
                    .value.ifFrame = {
                        .entry = top.value.ifFrame.entry,
                        .state = IF_EXPECTING_FI
                    },
                };
                if_state = IF_EXPECTING_FI;
                pop(&if_st);
                push(&if_st, frame);
                break;
        }
        case IF_EXPECTING_ELSE_OR_FI: {
            Value frame = {
                .type = IF_FRAME,
                .value.ifFrame = {
                    .entry = top.value.ifFrame.entry,
                    .state = IF_CONT
                },
            };
            if_state = IF_CONT;
            pop(&if_st);
            push(&if_st, frame);
            break;
        }
        case IF_EXPECTING_FI:
    }
    pc++;
    return DEF_VAL;
}

Value evalFi(ParseTreeNode *node)
{
    {
        Value top = peek(&shadow_st);
        if (top.type != IF_FRAME)
            ERR("not in a if statement", INCOMPATIBLE_TYPES);
        pop(&shadow_st);
    }
    if (pc < 0) {
        return DEF_VAL;
    }
    Value top = peek(&if_st);
    if (top.type != IF_FRAME)
        ERR("not in a if statement", INCOMPATIBLE_TYPES);
    Value ret = pop(&if_st);
    if (IS_ERR(ret)) {
        ERR("not in a if statement", ret.value.errVal);
    }
    ret = peek(&if_st);
    if (ret.type != IF_FRAME)
        if_state = IF_BEFORE;
    else
        if_state = ret.value.ifFrame.state;
    pc++;
    return DEF_VAL;
}

Value evalFor(ParseTreeNode *node)
{
    char *id = node->children[0]->token->lexeme;
    Value from = evalExpr(node->children[2]);
    ERR_RETURN_EVAL(from);
    Value to = evalExpr(node->children[3]);
    if (from.type != INT_VAL || to.type != INT_VAL)
        ERR("not a integer", INCOMPATIBLE_TYPES);
    Value step;
    if (node->children[4] != NULL) {
        step = evalExpr(node->children[4]);
        ERR_RETURN_EVAL(step);
        if (step.type != INT_VAL)
            ERR("not a integer", INCOMPATIBLE_TYPES);
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
    Value top = peek(&for_st);
    if (top.type == ERR_VAL
        && top.value.errVal == STACK_UNDERFLOW) {
        push(&for_st, ctx);
        insertVar(id, from);
        pc++;
    }
    else if (__likely(top.type == FOR_CTX)) {
        if (__likely(top.value.forCtx.pc == pc)) {
            if (__likely(strcasecmp(top.value.forCtx.identi, id) == 0)) {
                Value curVar = retriveVar(top.value.forCtx.identi);
                if (__unlikely((curVar.value.intVal > from.value.intVal
                                     && curVar.value.intVal > to.value.intVal)
                                     || (curVar.value.intVal < from.value.intVal
                                     && curVar.value.intVal < to.value.intVal))) {
                    pc = top.value.forCtx.next;
                    pop(&for_st);
                } else {
                    pc++;
                }
            } else {
                ERR("stack error", ERR_VAL_NULL);
            }
        } else {
            push(&for_st, ctx);
            insertVar(id, from);
            pc++;
        }
    } else {
        push(&for_st,ctx);
        insertVar(id, from);
        pc++;
    }
    return step;
}

Value evalNext(ParseTreeNode *node)
{
    Value top = peek(&for_st);
    if (top.type != FOR_CTX)
        ERR("not in a for loop", ERR_VAL_NULL);
    char *id = node->children[0]->token->lexeme;
    Value v = retriveVar(id);
    ERR_RETURN_EVAL(v);
    Value newVar = {
        .type = INT_VAL,
        .value.intVal = v.value.intVal + top.value.forCtx.step.intStep,
    };
    modVar(id, newVar);
    if (top.value.forCtx.next == -1) {
        ERR_RETURN_EVAL(pop(&for_st));
        top.value.forCtx.next = pc + 1;
        ERR_RETURN_EVAL(push(&for_st, top));
    }
    pc = top.value.forCtx.pc;
    return newVar;
}

Value evalGoto(ParseTreeNode *node)
{
    pc = lineNum_to_pc(node->children[0]->token->literal.intValue);
    if (pc == -1)
        expectedLineNum = node->children[0]->token->literal.intValue;
    else {
        if (prog.shadow_st[pc].size >= 0) {
            copy_stack(&prog.shadow_st[pc], &if_st);
            copy_stack(&if_st, &shadow_st);
            if_state = if_st.st[if_st.size-1].value.ifFrame.state;
        }
    }
    return DEF_VAL;
}

Value evalSleep(ParseTreeNode *node)
{
    Value t = evalExpr(node->children[0]);
    if (t.type == INT_VAL) {
        usleep(t.value.intVal);
    }
    else if (t.type == FLOAT_VAL) {
        usleep((int)t.value.floatVal);
    }
    else {
        ERR("incompatible types", INCOMPATIBLE_TYPES);
    }
    pc++;
    return t;
}

Value evalClear(ParseTreeNode *node)
{
    printf("\33[H\33[2J");
    pc++;
    return DEF_VAL;
}

Value evalHome(ParseTreeNode *node)
{
    printf("\033[H");
    pc++;
    return DEF_VAL;
}

static void free_arr(Value *arr)
{
    Value *base = arr->value.arr.ptr;
    for (int i = 0; i < arr->value.arr.size; i++) {
        if ((base + i)->type == ARR_VAL)
            if (findArrPtr(base + i, &arrPtrList)) {
                free_arr(base + i);
                delArrPtr(base + i, &arrPtrList);
            }
    }
    free(base);
}

Value evalFree(ParseTreeNode *node)
{
    char *name = node->children[0]->token->lexeme;
    if (strcasecmp(name, "COS") == 0
        || strcasecmp(name, "SIN") == 0
        || strcasecmp(name, "COSF") == 0
        || strcasecmp(name, "SINF") == 0
        || strcasecmp(name, "TAN") == 0
        || strcasecmp(name, "TANF") == 0
        || strcasecmp(name, "EXP") == 0
        || strcasecmp(name, "INT") == 0
        || strcasecmp(name, "FLOAT") == 0
        || strcasecmp(name, "NEW") == 0) {
        ERR("cannot use built-in names", ERR_VAL_NULL);
    }
    if (node->children[0]->childCount == 0
        || node->children[0]->children[0] == NULL) {
        char *name = node->children[0]->token->lexeme;
        Value v = retriveVar(name);
        if (v.type == ARR_VAL) {
            free_arr(&v);
        } else {
            ERR("not an array", INCOMPATIBLE_TYPES);
        }
        modVar(name, DEF_VAL);
    } else {
        Value id = retriveVar(name);
        ERR_RETURN_EVAL(id);
        if (id.type != ARR_VAL)
            ERR("not an array", INCOMPATIBLE_TYPES);
        int i = 0;
        int cnt = node->children[0]->childCount;
        Value index_val = evalExpr(node->children[0]->children[i]);
        ERR_RETURN_EVAL(index_val);
        if (index_val.type != INT_VAL)
            ERR("index has to be INT type", INCOMPATIBLE_TYPES);
        int index = index_val.value.intVal;
        if (index >= id.value.arr.size)
            ERR("index out of range", INDEX_OUT_OF_RANGE);
        Value *base = id.value.arr.ptr;
        Value *ptr = base + index;
        i++;
        while (i < cnt) {
            if (ptr->type != ARR_VAL)
                ERR("not an array", INCOMPATIBLE_TYPES);
            Value index_val = evalExpr(node->children[0]->children[i]);
            ERR_RETURN_EVAL(index_val);
            if (index_val.type != INT_VAL)
                ERR("index has to be INT type", INCOMPATIBLE_TYPES);
            int index = index_val.value.intVal;
            if (index >= ptr->value.arr.size)
                ERR("index out of range", INDEX_OUT_OF_RANGE);
            base = ptr->value.arr.ptr;
            ptr = base + index;
            i++;
        }
        if (ptr->type != ARR_VAL)
            ERR("not an array", INCOMPATIBLE_TYPES);    
        free_arr(ptr);
        ptr->type = INT_VAL;
        ptr->value.intVal = 0;
    }
    pc++;
    return DEF_VAL;
}

Value evalDel(ParseTreeNode *node)
{
    char *name = node->children[0]->token->lexeme;
    if (strcasecmp(name, "COS") == 0
        || strcasecmp(name, "SIN") == 0
        || strcasecmp(name, "COSF") == 0
        || strcasecmp(name, "SINF") == 0
        || strcasecmp(name, "TAN") == 0
        || strcasecmp(name, "TANF") == 0
        || strcasecmp(name, "EXP") == 0
        || strcasecmp(name, "INT") == 0
        || strcasecmp(name, "FLOAT") == 0
        || strcasecmp(name, "NEW") == 0) {
        ERR("cannot use built-in names", ERR_VAL_NULL);
    }
    if (node->children[0]->childCount == 0
        || node->children[0]->children[0] == NULL) {
        char *name = node->children[0]->token->lexeme;
        Value v = retriveVar(name);
        if (v.type == ARR_VAL) {
            free_arr(&v);
        }
        delVar(name);
    } else {
        Value id = retriveVar(name);
        ERR_RETURN_EVAL(id);
        if (id.type != ARR_VAL)
            ERR("not an array", INCOMPATIBLE_TYPES);
        int i = 0;
        int cnt = node->children[0]->childCount;
        Value index_val = evalExpr(node->children[0]->children[i]);
        ERR_RETURN_EVAL(index_val);
        if (index_val.type != INT_VAL)
            ERR("index has to be INT type", INCOMPATIBLE_TYPES);
        int index = index_val.value.intVal;
        if (index >= id.value.arr.size)
            ERR("index out of range", INDEX_OUT_OF_RANGE);
        Value *base = id.value.arr.ptr;
        Value *ptr = base + index;
        i++;
        while (i < cnt) {
            if (ptr->type != ARR_VAL)
                ERR("not an array", INCOMPATIBLE_TYPES);
            Value index_val = evalExpr(node->children[0]->children[i]);
            ERR_RETURN_EVAL(index_val);
            if (index_val.type != INT_VAL)
                ERR("index has to be INT type", INCOMPATIBLE_TYPES);
            int index = index_val.value.intVal;
            if (index >= ptr->value.arr.size)
                ERR("index out of range", INDEX_OUT_OF_RANGE);
            base = ptr->value.arr.ptr;
            ptr = base + index;
            i++;
        }
        if (ptr->type != ARR_VAL)
            ERR("not an array", INCOMPATIBLE_TYPES);    
        free_arr(ptr);
        ptr->type = INT_VAL;
        ptr->value.intVal = 0;
    }
    pc++;
    return DEF_VAL;
}

Value evalExpr(ParseTreeNode *node)
{
    return evalOrExpr(node);
}

Value evalOrExpr(ParseTreeNode *node)
{
    unsigned int cnt = node->childCount;
    if (cnt == 1) {
        return evalAndExpr(node->children[0]);
    } else {
        Value v = evalAndExpr(node->children[0]);
        ERR_RETURN_EVAL(v);
        for (int i = 1; i < cnt; i += 2) {
            if (node->children[i]->type == OR_OP) {
                Value tmp = evalAndExpr(node->children[i+1]);
                ERR_RETURN_EVAL(tmp);
                enum ValueType t = v.type;
                v.type = BOOL_VAL;
                if (t == BOOL_VAL) {
                    if (tmp.type == BOOL_VAL)
                        v.value.boolVal = (v.value.boolVal || tmp.value.boolVal);
                    else if (tmp.type == INT_VAL)
                        v.value.boolVal = (v.value.boolVal || tmp.value.intVal);
                    else if (tmp.type == FLOAT_VAL)
                            v.value.boolVal = (v.value.boolVal || tmp.value.floatVal);
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (t == INT_VAL) {
                    if (tmp.type == BOOL_VAL)
                        v.value.boolVal = (v.value.intVal || tmp.value.boolVal);
                    else if (tmp.type == INT_VAL)
                        v.value.boolVal = (v.value.intVal || tmp.value.intVal);
                    else if (tmp.type == FLOAT_VAL)
                            v.value.boolVal = (v.value.intVal || tmp.value.floatVal);
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (t == FLOAT_VAL) {
                    if (tmp.type == BOOL_VAL)
                        v.value.boolVal = (v.value.floatVal || tmp.value.boolVal);
                    else if (tmp.type == INT_VAL)
                        v.value.boolVal = (v.value.floatVal || tmp.value.intVal);
                    else if (tmp.type == FLOAT_VAL)
                            v.value.boolVal = (v.value.floatVal || tmp.value.floatVal);
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                } else {
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
            } else {
                ERR("not an or operation", INCOMPATIBLE_TYPES);
            }
        }
        return v;
    }
}

Value evalAndExpr(ParseTreeNode *node)
{
    unsigned int cnt = node->childCount;
    if (cnt == 1) {
        return evalRelExpr(node->children[0]);
    } else {
        Value v = evalRelExpr(node->children[0]);
        ERR_RETURN_EVAL(v);
        for (int i = 1; i < cnt; i += 2) {
            if (node->children[i]->type == AND_OP) {
                Value tmp = evalRelExpr(node->children[i+1]);
                ERR_RETURN_EVAL(tmp);
                enum ValueType t = v.type;
                v.type = BOOL_VAL;
                if (t == BOOL_VAL) {
                    if (tmp.type == BOOL_VAL)
                        v.value.boolVal = (v.value.boolVal && tmp.value.boolVal);
                    else if (tmp.type == INT_VAL)
                        v.value.boolVal = (v.value.boolVal && tmp.value.intVal);
                    else if (tmp.type == FLOAT_VAL)
                            v.value.boolVal = (v.value.boolVal && tmp.value.floatVal);
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (t == INT_VAL) {
                    if (tmp.type == BOOL_VAL)
                        v.value.boolVal = (v.value.intVal && tmp.value.boolVal);
                    else if (tmp.type == INT_VAL)
                        v.value.boolVal = (v.value.intVal && tmp.value.intVal);
                    else if (tmp.type == FLOAT_VAL)
                            v.value.boolVal = (v.value.intVal && tmp.value.floatVal);
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (t == FLOAT_VAL) {
                    if (tmp.type == BOOL_VAL)
                        v.value.boolVal = (v.value.floatVal && tmp.value.boolVal);
                    else if (tmp.type == INT_VAL)
                        v.value.boolVal = (v.value.floatVal && tmp.value.intVal);
                    else if (tmp.type == FLOAT_VAL)
                            v.value.boolVal = (v.value.floatVal && tmp.value.floatVal);
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                } else {
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
            } else {
                ERR("not an add operation", INCOMPATIBLE_TYPES);
            }
        }
        return v;
    }
}

Value evalRelExpr(ParseTreeNode *node)
{
    unsigned int cnt = node->childCount;
    if (cnt == 1) {
        return evalAddExpr(node->children[0]);
    } else {
        Value v = evalAddExpr(node->children[0]);
        ERR_RETURN_EVAL(v);
        for (int i = 1; i < cnt; i += 2) {
            if (node->children[i]->type == EQ || node->children[i]->type == LT
                || node->children[i]->type == GT || node->children[i]->type == LE
                || node->children[i]->type == GE || node->children[i]->type == NE) {
                TokenType t = node->children[i]->type;
                Value tmp = evalAddExpr(node->children[i+1]);
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
                         else if (v.type == FLOAT_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.floatVal == tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.floatVal == tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.floatVal == tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        } else {
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
                        else if (v.type == FLOAT_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.floatVal < tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.floatVal < tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.floatVal < tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        } else {
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
                        else if (v.type == FLOAT_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.floatVal > tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.floatVal > tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.floatVal > tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        } else {
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
                        else if (v.type == FLOAT_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.floatVal <= tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.floatVal <= tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.floatVal <= tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        } else {
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
                        else if (v.type == FLOAT_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.floatVal >= tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.floatVal >= tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.floatVal >= tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        } else {
                            ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                        break;
                    case NE:
                        if (v.type == BOOL_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.boolVal != tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.boolVal != tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.boolVal != tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                        else if (v.type == INT_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.intVal != tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.intVal != tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.intVal != tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                        else if (v.type == FLOAT_VAL) {
                            v.type = BOOL_VAL;
                            if (tmp.type == BOOL_VAL)
                                v.value.boolVal = (v.value.floatVal != tmp.value.boolVal);
                            else if (tmp.type == INT_VAL)
                                v.value.boolVal = (v.value.floatVal != tmp.value.intVal);
                            else if (tmp.type == FLOAT_VAL)
                                v.value.boolVal = (v.value.floatVal != tmp.value.floatVal);
                            else
                                ERR("incompatible types", INCOMPATIBLE_TYPES);
                        } else {
                            ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                        break;
                    default:
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
            }
        }
        return v;
    }
}

Value evalAddExpr(ParseTreeNode *node)
{
    unsigned int cnt = node->childCount;
    if (cnt == 1) {
        return evalMulExpr(node->children[0]);
    } else {
        Value v = evalMulExpr(node->children[0]);
        ERR_RETURN_EVAL(v);
        for (int i = 1; i < cnt; i += 2) {
            enum ValueType t = v.type;
            if (node->children[i]->type == ADD_OP) {
                Value tmp = evalMulExpr(node->children[i+1]);
                ERR_RETURN_EVAL(tmp);
                v.type = INT_VAL;
                if (t == BOOL_VAL) {
                    if (tmp.type == BOOL_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "+") == 0)
                            v.value.intVal = (v.value.boolVal + tmp.value.boolVal);
                        else if (strcmp(node->children[i]->token->lexeme, "-") == 0)
                            v.value.intVal = (v.value.boolVal - tmp.value.boolVal);
                    }
                    else if (tmp.type == INT_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "+") == 0)
                            v.value.intVal = (v.value.boolVal + tmp.value.intVal);
                        else if (strcmp(node->children[i]->token->lexeme, "-") == 0)
                            v.value.intVal = (v.value.boolVal - tmp.value.intVal);
                    }
                    else if (tmp.type == FLOAT_VAL) {
                        v.type = FLOAT_VAL;
                        if (strcmp(node->children[i]->token->lexeme, "+") == 0)
                            v.value.floatVal = (v.value.boolVal + tmp.value.floatVal);
                        else if (strcmp(node->children[i]->token->lexeme, "-") == 0)
                            v.value.floatVal = (v.value.boolVal - tmp.value.floatVal);
                    }
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (t == INT_VAL) {
                    if (tmp.type == BOOL_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "+") == 0)
                            v.value.intVal = (v.value.intVal + tmp.value.boolVal);
                        else if (strcmp(node->children[i]->token->lexeme, "-") == 0)
                            v.value.intVal = (v.value.intVal - tmp.value.boolVal);
                    }
                    else if (tmp.type == INT_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "+") == 0)
                            v.value.intVal = (v.value.intVal + tmp.value.intVal);
                        else if (strcmp(node->children[i]->token->lexeme, "-") == 0)
                            v.value.intVal = (v.value.intVal - tmp.value.intVal);
                    }
                    else if (tmp.type == FLOAT_VAL) {
                        v.type = FLOAT_VAL;
                        if (strcmp(node->children[i]->token->lexeme, "+") == 0)
                            v.value.floatVal = (v.value.intVal + tmp.value.floatVal);
                        else if (strcmp(node->children[i]->token->lexeme, "-") == 0)
                            v.value.floatVal = (v.value.intVal - tmp.value.floatVal);
                    }
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (t == FLOAT_VAL) {
                    v.type = FLOAT_VAL;
                    if (tmp.type == BOOL_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "+") == 0)
                            v.value.floatVal = (v.value.floatVal + tmp.value.boolVal);
                        else if (strcmp(node->children[i]->token->lexeme, "-") == 0)
                            v.value.floatVal = (v.value.floatVal - tmp.value.boolVal);
                    }
                    else if (tmp.type == INT_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "+") == 0)
                            v.value.floatVal = (v.value.floatVal + tmp.value.intVal);
                        else if (strcmp(node->children[i]->token->lexeme, "-") == 0)
                            v.value.floatVal = (v.value.floatVal - tmp.value.intVal);
                    }
                    else if (tmp.type == FLOAT_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "+") == 0)
                            v.value.floatVal = (v.value.floatVal + tmp.value.floatVal);
                        else if (strcmp(node->children[i]->token->lexeme, "-") == 0)
                            v.value.floatVal = (v.value.floatVal - tmp.value.floatVal);
                    }
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                } else {
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
            } else {
                ERR("not an add operation", INCOMPATIBLE_TYPES);
            }
        }
        return v;
    }
}

Value evalMulExpr(ParseTreeNode *node)
{
    unsigned int cnt = node->childCount;
    if (cnt == 1) {
        return evalUnary(node->children[0]);
    } else {
        Value v = evalUnary(node->children[0]);
        ERR_RETURN_EVAL(v);
        for (int i = 1; i < cnt; i += 2) {
            if (node->children[i]->type == MUL_OP) {
                enum ValueType t = v.type;
                Value tmp = evalUnary(node->children[i+1]);
                ERR_RETURN_EVAL(tmp);
                v.type = INT_VAL;
                if (t == BOOL_VAL) {
                    if (tmp.type == BOOL_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "*") == 0)
                            v.value.intVal = (v.value.boolVal * tmp.value.boolVal);
                        else if (strcmp(node->children[i]->token->lexeme, "/") == 0) {
                            if (tmp.value.boolVal == false)
                                ERR("cannot divide by zero", DIVIDE_BY_ZERO);
                            v.value.intVal = (v.value.boolVal / tmp.value.boolVal);
                        }
                        else if (strcmp(node->children[i]->token->lexeme, "%") == 0) {
                            if (tmp.value.boolVal == false)
                                ERR("cannot divide by zero", DIVIDE_BY_ZERO);
                            v.value.intVal = (v.value.boolVal % tmp.value.boolVal);
                        }
                    }
                    else if (tmp.type == INT_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "*") == 0)
                            v.value.intVal = (v.value.boolVal * tmp.value.intVal);
                        else if (strcmp(node->children[i]->token->lexeme, "/") == 0) {
                            if (tmp.value.intVal == 0)
                                ERR("cannot divide by zero", DIVIDE_BY_ZERO);
                            v.value.intVal = (v.value.boolVal / tmp.value.intVal);
                        }
                        else if (strcmp(node->children[i]->token->lexeme, "%") == 0) {
                            if (tmp.value.intVal == 0)
                                ERR("cannot divide by zero", DIVIDE_BY_ZERO);
                            v.value.intVal = (v.value.boolVal % tmp.value.intVal);
                        }
                    }
                    else if (tmp.type == FLOAT_VAL) {
                        v.type = FLOAT_VAL;
                        if (strcmp(node->children[i]->token->lexeme, "*") == 0)
                            v.value.floatVal = (v.value.boolVal * tmp.value.floatVal);
                        else if (strcmp(node->children[i]->token->lexeme, "/") == 0) {
                            if (tmp.value.floatVal == 0.0)
                                ERR("cannot divide by zero", DIVIDE_BY_ZERO);
                            v.value.floatVal = (v.value.boolVal / tmp.value.floatVal);
                        }
                        else if (strcmp(node->children[i]->token->lexeme, "%") == 0) {
                            ERR("incompatible types", INCOMPATIBLE_TYPES);
                        }
                    }
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (t == INT_VAL) {
                    if (tmp.type == BOOL_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "*") == 0)
                            v.value.intVal = (v.value.intVal * tmp.value.boolVal);
                        else if (strcmp(node->children[i]->token->lexeme, "/") == 0) {
                            if (tmp.value.boolVal == false)
                                ERR("cannot divide by zero", DIVIDE_BY_ZERO);
                            v.value.intVal = (v.value.intVal / tmp.value.boolVal);
                        }
                         else if (strcmp(node->children[i]->token->lexeme, "%") == 0) {
                            if (tmp.value.boolVal == false)
                                ERR("cannot divide by zero", DIVIDE_BY_ZERO);
                            v.value.intVal = (v.value.intVal % tmp.value.boolVal);
                        }
                    }
                    else if (tmp.type == INT_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "*") == 0)
                            v.value.intVal = (v.value.intVal * tmp.value.intVal);
                        else if (strcmp(node->children[i]->token->lexeme, "/") == 0) {
                            if (tmp.value.intVal == 0)
                                ERR("cannot divide by zero", DIVIDE_BY_ZERO);
                            v.value.intVal = (v.value.intVal / tmp.value.intVal);
                        }
                        else if (strcmp(node->children[i]->token->lexeme, "%") == 0) {
                            if (tmp.value.intVal == 0)
                                ERR("cannot divide by zero", DIVIDE_BY_ZERO);
                            v.value.intVal = (v.value.intVal % tmp.value.intVal);
                        }
                    }
                    else if (tmp.type == FLOAT_VAL) {
                        v.type = FLOAT_VAL;
                        if (strcmp(node->children[i]->token->lexeme, "*") == 0)
                            v.value.floatVal = (v.value.intVal * tmp.value.floatVal);
                        else if (strcmp(node->children[i]->token->lexeme, "/") == 0) {
                            if (tmp.value.floatVal == 0.0)
                                ERR("cannot divide by zero", DIVIDE_BY_ZERO);
                            v.value.floatVal = (v.value.intVal / tmp.value.floatVal);
                        }
                        else if (strcmp(node->children[i]->token->lexeme, "%") == 0)
                            ERR("incompatible types", INCOMPATIBLE_TYPES);
                    }
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                else if (t == FLOAT_VAL) {
                    v.type = FLOAT_VAL;
                    if (strcmp(node->children[i]->token->lexeme, "%") == 0)
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                    if (tmp.type == BOOL_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "*") == 0)
                            v.value.floatVal = (v.value.floatVal * tmp.value.boolVal);
                        else if (strcmp(node->children[i]->token->lexeme, "/") == 0) {
                            if (tmp.value.boolVal == false)
                                ERR("cannot divide by zero", DIVIDE_BY_ZERO);
                            v.value.floatVal = (v.value.floatVal / tmp.value.boolVal);
                        }
                    }
                    else if (tmp.type == INT_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "*") == 0)
                            v.value.floatVal = (v.value.floatVal * tmp.value.intVal);
                        else if (strcmp(node->children[i]->token->lexeme, "/") == 0) {
                            if (tmp.value.intVal == 0)
                                ERR("cannot divide by zero", DIVIDE_BY_ZERO);
                            v.value.floatVal = (v.value.floatVal / tmp.value.intVal);
                        }
                    }
                    else if (tmp.type == FLOAT_VAL) {
                        if (strcmp(node->children[i]->token->lexeme, "*") == 0)
                            v.value.floatVal = (v.value.floatVal * tmp.value.floatVal);
                        else if (strcmp(node->children[i]->token->lexeme, "/") == 0) {
                            if (tmp.value.floatVal == 0.0)
                                ERR("cannot divide by zero", DIVIDE_BY_ZERO);
                            v.value.floatVal = (v.value.floatVal / tmp.value.floatVal);
                        }
                    }
                    else
                        ERR("incompatible types", INCOMPATIBLE_TYPES);
                } else {
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
            }
        }
        return v;
    }
}

Value evalUnary(ParseTreeNode *node)
{
    unsigned int cnt = node->childCount;
    if (cnt == 1) {
        return evalPrimary(node->children[0]);
    } else {
        Value v = evalPrimary(node->children[1]);
        ERR_RETURN_EVAL(v);
        enum ValueType t = v.type;
        if (node->children[0]->type == UNARY_OP) {
            if (strcmp(node->children[0]->token->lexeme, "+") == 0) {
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
            else if (strcmp(node->children[0]->token->lexeme, "-") == 0) {
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
            else if (strcasecmp(node->children[0]->token->lexeme, "NOT") == 0) {
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

Value evalPrimary(ParseTreeNode *node)
{
    Value v;
    if (node->children[0]->type == KEYWORD_TOKEN) {
        v.type = BOOL_VAL;
        if (strcasecmp(node->token->lexeme, "FALSE") == 0)
            v.value.boolVal = false;
        else if (strcasecmp(node->token->lexeme, "TRUE") == 0)
            v.value.boolVal = true;
        else
            ERR("unknown bool value", UNKNOWN_BOOL_VALUE);
    }
    else if (node->children[0]->type == INTEGER) {
        v.type = INT_VAL;
        v.value.intVal = node->children[0]->token->literal.intValue;
    }
    else if (node->children[0]->type == FLOAT) {
        v.type = FLOAT_VAL;
        v.value.floatVal = node->children[0]->token->literal.floatValue;
    }
    else if (node->children[0]->type == STRING) {
        v.type = STRING_VAL;
        v.value.string.str = node->children[0]->token->literal.string;
        v.value.string.refcnt = 1;
    }
    else if (node->children[0]->type == OR_EXPR) {
        Value tmp = evalOrExpr(node->children[0]);
        v.type = tmp.type;
        v.value = tmp.value;
    }
    else if (node->children[0]->type == IDENTI) {
        if (node->children[0]->childCount == 0
            || node->children[0]->children[0] == NULL) {
            Value id = retriveVar(node->children[0]->token->lexeme);
            ERR_RETURN_EVAL(id);
            v.type = id.type;
            v.value = id.value;
        } else {
            char *name = node->children[0]->token->lexeme;
            if (strcasecmp(name, "COS") == 0) {
                Value index_val = evalExpr(node->children[0]->children[0]);
                v.type = FLOAT_VAL;
                if (index_val.type == FLOAT_VAL) {
                    v.value.floatVal = cos(index_val.value.floatVal);
                }
                else if (index_val.type == INT_VAL) {
                    v.value.floatVal = cos(index_val.value.intVal);
                }
                else {
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                return v;
            }
            else if (strcasecmp(name, "SIN") == 0) {
                Value index_val = evalExpr(node->children[0]->children[0]);
                v.type = FLOAT_VAL;
                if (index_val.type == FLOAT_VAL) {
                    v.value.floatVal = sin(index_val.value.floatVal);
                }
                else if (index_val.type == INT_VAL) {
                    v.value.floatVal = sin(index_val.value.intVal);
                }
                else {
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                return v;
            }
            else if (strcasecmp(name, "TAN") == 0) {
                Value index_val = evalExpr(node->children[0]->children[0]);
                v.type = FLOAT_VAL;
                if (index_val.type == FLOAT_VAL) {
                    v.value.floatVal = tan(index_val.value.floatVal);
                }
                else if (index_val.type == INT_VAL) {
                    v.value.floatVal = tan(index_val.value.intVal);
                }
                else {
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                return v;
            }
            else if (strcasecmp(name, "COSF") == 0) {
                Value index_val = evalExpr(node->children[0]->children[0]);
                v.type = FLOAT_VAL;
                if (index_val.type == FLOAT_VAL) {
                    v.value.floatVal = cosf(index_val.value.floatVal);
                }
                else if (index_val.type == INT_VAL) {
                    v.value.floatVal = cosf(index_val.value.intVal);
                }
                else {
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                return v;
            }
            else if (strcasecmp(name, "SINF") == 0) {
                Value index_val = evalExpr(node->children[0]->children[0]);
                v.type = FLOAT_VAL;
                if (index_val.type == FLOAT_VAL) {
                    v.value.floatVal = sinf(index_val.value.floatVal);
                }
                else if (index_val.type == INT_VAL) {
                    v.value.floatVal = sinf(index_val.value.intVal);
                }
                else {
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                return v;
            }
            else if (strcasecmp(name, "TANF") == 0) {
                Value index_val = evalExpr(node->children[0]->children[0]);
                v.type = FLOAT_VAL;
                if (index_val.type == FLOAT_VAL) {
                    v.value.floatVal = tanf(index_val.value.floatVal);
                }
                else if (index_val.type == INT_VAL) {
                    v.value.floatVal = tanf(index_val.value.intVal);
                }
                else {
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                return v;
            }
            else if (strcasecmp(name, "EXP") == 0) {
                Value index_val = evalExpr(node->children[0]->children[0]);
                v.type = FLOAT_VAL;
                if (index_val.type == FLOAT_VAL) {
                    v.value.floatVal = exp(index_val.value.floatVal);
                }
                else if (index_val.type == INT_VAL) {
                    v.value.floatVal = exp(index_val.value.intVal);
                }
                else {
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                return v;
            }
            else if (strcasecmp(name, "INT") == 0) {
                Value index_val = evalExpr(node->children[0]->children[0]);
                v.type = INT_VAL;
                if (index_val.type == FLOAT_VAL) {
                    v.value.intVal = (int)index_val.value.floatVal;
                }
                else if (index_val.type == INT_VAL) {
                    v.value.intVal = index_val.value.intVal;
                }
                else if (index_val.type == BOOL_VAL) {
                    v.value.intVal = (int)index_val.value.boolVal;
                }
                else {
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                return v;
            }
            else if (strcasecmp(name, "FLOAT") == 0) {
                Value index_val = evalExpr(node->children[0]->children[0]);
                v.type = FLOAT_VAL;
                if (index_val.type == FLOAT_VAL) {
                    v.value.floatVal = index_val.value.floatVal;
                }
                else if (index_val.type == INT_VAL) {
                    v.value.floatVal = (float)index_val.value.intVal;
                }
                else if (index_val.type == BOOL_VAL) {
                    v.value.floatVal = (float)index_val.value.boolVal;
                }
                else {
                    ERR("incompatible types", INCOMPATIBLE_TYPES);
                }
                return v;
            }
            else if (strcasecmp(name, "NEW") == 0) {
                Value size = evalExpr(node->children[0]->children[0]);
                v.type = ARR_VAL;
                if (size.type != INT_VAL)
                    ERR("size has to be INT type", INCOMPATIBLE_TYPES);
                if (size.value.intVal <= 0) {
                    ERR("array size can't be zero", ERR_VAL_NULL);
                }
                v.value.arr.refcnt = 1;
                v.value.arr.ptr = (Value *)calloc(size.value.intVal, sizeof(Value));
                if (v.value.arr.ptr == NULL)
                    ERR("out of memory", OUT_OF_MEMORY);
                v.value.arr.size = size.value.intVal;
                insertArrPtr(v.value.arr.ptr, &arrPtrList);
                for (int i = 0; i < size.value.intVal; i++) {
                    v.value.arr.ptr[i].type = INT_VAL;
                }
                return v;
            }
            Value id = retriveVar(name);
            ERR_RETURN_EVAL(id);
            if (id.type != ARR_VAL)
                ERR("not an array", INCOMPATIBLE_TYPES);
            int i = 0;
            int cnt = node->children[0]->childCount;
next_index:
            Value index_val = evalExpr(node->children[0]->children[i]);
            ERR_RETURN_EVAL(index_val);
            if (index_val.type != INT_VAL)
                ERR("index has to be INT type", INCOMPATIBLE_TYPES);
            int index = index_val.value.intVal;
            if (index >= id.value.arr.size)
                ERR("index out of range", INDEX_OUT_OF_RANGE);
            Value *base = id.value.arr.ptr;
            Value *ptr = base + index;
            v.type = ptr->type;
            v.value = ptr->value;
            if (IS_ERR(v))
                ERR("uninitialized value", UNINIT_VAL);
            if (i < cnt - 1) {
                if (v.type != ARR_VAL)
                    ERR("not an array", INCOMPATIBLE_TYPES);
                id = v;
                i++;
                goto next_index;
            }
        }
    }
    return v;
}
