#ifndef EQUATION_H_
#define EQUATION_H_

#include <stddef.h> // size_t, NULL
#include <ctype.h> // isalnum
#include <string.h> // strlen, cmp
#include <stdbool.h>
#include <stdio.h> // sscanf
#include <math.h> // NAN, fmodf, and others

#include "dynarray.h"


/* String */

typedef struct {
    char * items; // [!] keep this null terminated
    size_t count;
    size_t capacity;

    size_t cursor;
} String;

String string_createEmpty();
void string_append(String * str, char c);
void string_insert(String * str, char c);
void string_backspace(String * str);

// make sure to add initial 0
String string_createEmpty() {
    String str = {};
    da_append(&str, 0);
    return str;
}

void string_append(String * str, char c) {
    str->items[str->count - 1] = c;
    da_append(str, 0);
}

void string_insert(String * str, char c) {
    for (size_t i = str->count - 1; i > str->cursor; --i) {
        // @assert i > 0
        str->items[i] = str->items[i-1];
    }
    str->items[str->cursor] = c;
    str->cursor += 1;
    da_append(str, 0);
}
void string_backspace(String * str) {
    if (str->cursor == 0) return;
    for (size_t i = str->cursor; i < str->count; ++i) {
        str->items[i-1] = str->items[i];
    }
    str->cursor -= 1;
    str->count -= 1;
}


/* register built-in tokens */

// [!] keep the order the same
const char builtin_funcs[][6] = {
    "sinh", "cosh", "tanh",
    "asin", "acos", "atan",
    "sin", "cos", "tan",
    "exp", "log", /*"pow", */"sqrt",
    "floor", "ceil", "round",
    "abs", "sgn", /*"hypot", "min", "max", */
};
typedef enum {
    BFUNC_SINH, BFUNC_COSH, BFUNC_TANH,
    BFUNC_ASIN, BFUNC_ACOS, BFUNC_ATAN,
    BFUNC_SIN, BFUNC_COS, BFUNC_TAN,
    BFUNC_EXP, BFUNC_LOG, /*BFUNC_POW, */BFUNC_SQRT,
    BFUNC_FLOOR, BFUNC_CEIL, BFUNC_ROUND,
    BFUNC_ABS, BFUNC_SGN, /*BFUNC_HYPOT, BFUNC_MIN, BFUNC_MAX, */
} BFuncType;
const char builtin_vars[][3] = {
    "pi", "e", "x",
};
typedef enum {
    BVAR_PI, BVAR_E, BVAR_X,
} BVarType;
const char builtin_binops[][2] = {
    "+", "-", "*", "/", "%",
};
typedef enum {
    BINOP_PLUS, BINOP_MINUS, BINOP_MULT, BINOP_DIV, BINOP_MOD,
} BinopType;
const char builtin_unprecops[][3] = {
    "+", "-",
};
typedef enum {
    UPOP_PLUS, UPOP_MINUS,
} UnPrecOpType;
const size_t n_builtin_funcs = sizeof(builtin_funcs) / sizeof(builtin_funcs[0]);
const size_t n_builtin_vars = sizeof(builtin_vars) / sizeof(builtin_vars[0]);
const size_t n_builtin_binops = sizeof(builtin_binops) / sizeof(builtin_binops[0]);
const size_t n_builtin_unprecops = sizeof(builtin_unprecops) / sizeof(builtin_unprecops[0]);

/*
const char builtin_op_prec[][3][3] = { // with precedence info
    {"*", "/", "%"},
    {"+", "-"},
    //{"<=", ">=", "<", ">"},
    //{"="},
    {"(", ")"}, // `)` shouldn't really have a precedence
};
const size_t n_builtin_op_precs = sizeof(builtin_op_prec) / sizeof(builtin_op_prec[0]);
const size_t n_builtin_ops_per_prec = sizeof(builtin_op_prec[0]) / sizeof(builtin_op_prec[0][0]);
*/


/* Token */

typedef enum {
    TT_NONE, // invalid type or start of expression
    TT_NUMBER, // real number
    TT_VAR,
    TT_BVAR,
    TT_BFUNC,
    TT_BINOP, // binary operation
    TT_UNPRECOP, // unary preceding operation
    TT_LPARE, TT_RPARE,
} TokenType;

typedef struct {
    TokenType type;
    union {
        /*
        struct {
            char * begin;
            size_t len;
        } strview; // for VAR - now it is never used
        */
        float number;
        BinopType binop;
        UnPrecOpType unprecop;
        BVarType bvar;
        BFuncType bfunc;
    } as;
} Token;

typedef struct {
    Token * items;
    size_t count;
    size_t capacity;
} Tokens;

size_t expr_parse_token(Token * ret, char * begin, TokenType prev_type);
char * trim_left(char * begin);
int expr_tokenize(Tokens * tokens, String str);

// tokenize
// @param prev_type: TT_NONE for start of expr
size_t expr_parse_token(Token * ret, char * begin, TokenType prev_type) { // return token length, 0 for failure
    if (!begin || begin[0] == '\0') return 0;
    
    // built-in tokens
    // bfunc
    for (size_t i = 0; i < n_builtin_funcs; ++i) {
        int len = strlen(builtin_funcs[i]);
        if (strncmp(begin, builtin_funcs[i], len) == 0) {
            *ret = (Token) {TT_BFUNC, {.bfunc = i}}; // cast from size_t to BFuncType
            return len;
        }
    }
    // bvar
    for (size_t i = 0; i < n_builtin_vars; ++i) {
        int len = strlen(builtin_vars[i]);
        if (strncmp(begin, builtin_vars[i], len) == 0) {
            *ret = (Token) {TT_BVAR, {.bvar = i}}; // cast from size_t to BFuncType
            return len;
        }
    }
    // 1xf( follows everything, +) follows 1xf), ! follows (s)+!(
    switch (prev_type) {
    case TT_NUMBER: case TT_VAR: case TT_BVAR: case TT_BFUNC: case TT_RPARE:
        // binop
        for (size_t i = 0; i < n_builtin_binops; ++i) {
            int len = strlen(builtin_binops[i]);
            if (strncmp(begin, builtin_binops[i], len) == 0) {
                *ret = (Token) {TT_BINOP, {.binop = i}}; // cast from size_t to BFuncType
                return len;
            }
        } break;
    case TT_NONE: case TT_BINOP: case TT_UNPRECOP: case TT_LPARE:
        // unprecop
        for (size_t i = 0; i < n_builtin_unprecops; ++i) {
            int len = strlen(builtin_unprecops[i]);
            if (strncmp(begin, builtin_unprecops[i], len) == 0) {
                *ret = (Token) {TT_UNPRECOP, {.unprecop = i}}; // cast from size_t to BFuncType
                return len;
            }
        } break;
    // @assert unreachable
    default: ;
    }
    
    // real number
    float number;
    int numberlen;
    if (sscanf(begin, "%f%n", &number, &numberlen) == 1) {
        *ret = (Token) {TT_NUMBER, {.number = number}};
        return numberlen;
    }
    
    // 1 character var
    /*
    if (isalpha((unsigned char)begin[0])) {
        *ret = (Token) {TT_VAR, {.strview = {begin, 1}}};
        return 1;
    }
    */

    // pare
    if (begin[0] == '(') {
        *ret = (Token) {TT_LPARE};
        return 1;
    }
    if (begin[0] == ')') {
        *ret = (Token) {TT_RPARE};
        return 1;
    }

    // undefined token
    return 0;
}

char * trim_left(char * begin) { // non-mutating
    while (isspace(*begin)) begin += 1; // safe for '\0'
    return begin;
}

int expr_tokenize(Tokens * tokens, String str) { // return 1 on failure
    if (!tokens) return 1;
    
    char * view = trim_left(str.items);
    Token tmp;
    TokenType prev_type = TT_NONE;
    while (*view != '\0') {
        size_t len = expr_parse_token(&tmp, view, prev_type);
        if (len == 0) return 1;
        da_append(tokens, tmp);
        prev_type = tmp.type;
        view = trim_left(view + len);
    }
    return 0;
}



/* Expr */

typedef struct ExprNode {
    Token self;
    // children
    struct ExprNode * items;
    size_t count;
    size_t capacity;
} ExprNode;

typedef enum {
    ES_NONE, // just after creation
    ES_INVALID,
    ES_VALID,
} EquationState;

typedef struct {
    String editor;
    String text; // internal copy of raw text
    ExprNode expr; // references `text`
    EquationState state;
} Equation;

typedef struct {
    // to which the successfully parsed tree should be attached, by the function called
    ExprNode * parent;
    // range of `Token`s that the function called shall parse
    Token * begin;
    Token * end;
} Expr_Builder_Frame;

int build_op_node(ExprNode * operands, ExprNode operator);
int expr_parse_BFUNC(Expr_Builder_Frame * frame); // responsible for deciding the end of subexpr
int expr_parse(Expr_Builder_Frame * frame); // `frame` should already mark the end of this expr
float expr_eval(ExprNode node, float x);
void expr_free_node(ExprNode * node);
int equation_parse(Equation * eq);
void equation_free(Equation * eq);

// build tree
#define tokstrcmp(tok, str) strncmp((tok).begin, str, (tok).len)

int build_op_node(ExprNode * operands, ExprNode operator) {
    switch (operator.self.type) {
    case TT_BINOP:
        if (operands->count < 2) return 1;
        da_append(&operator, operands->items[operands->count - 2]);
        da_append(&operator, operands->items[operands->count - 1]);
        operands->count -= 2;
        da_append(operands, operator);
        return 0;
    case TT_UNPRECOP:
        if (operands->count < 1) return 1;
        da_append(&operator, operands->items[operands->count - 1]);
        operands->count -= 1;
        da_append(operands, operator);
        return 0;
    default: // not op
        return 1;
    }
}

int get_op_prec(Token tok) { // -1 for not in list, op: BinopType | UnPrecOpType
    switch (tok.type) {
    case TT_UNPRECOP:
        // only +-
        return 0;
    case TT_BINOP:
        switch (tok.as.binop) {
        case BINOP_MULT: case BINOP_DIV: case BINOP_MOD:
            return 1;
        case BINOP_PLUS: case BINOP_MINUS:
            return 2;
        }
    case TT_LPARE:
        return 3;
    default:
        return -1;
    }
}
int expr_parse(Expr_Builder_Frame * frame) { // `frame` should contain the range of this expression
    if (frame->begin >= frame->end) return 1;
    
    // 1. parse a list of atomics and ops
    
    // @algo: what can follow what?
    // element classes: (start), 1, x, f, +, !, (, )
    // (s) followed by 1xf !( 
    //   1 followed by  xf+ () -- however due to greed sscanf, writing `1` or not here is irrelevant
    //   x followed by 1xf+ ()
    //   f followed by 1xf+ ()
    //   + followed by 1xf !(  -- allow 1+-2 
    //   ! followed by 1xf !(  -- allow --2
    //   ( followed by 1xf !( 
    //   ) followed by 1xf+ ()
    // happily we see here that + and ! are mutually exclusive
    
    // 1xf( follows everything, +) follows 1xf), ! follows (s)+!(
    
    ExprNode node_list = {}; // owns its items until parse success
    Expr_Builder_Frame subframe = {
        .parent = &node_list,
        .begin = frame->begin,
        .end = frame->end,
    };
    
#define error_cleanup()                         \
    do {                                        \
        expr_free_node(&node_list);             \
        return 1;                               \
    } while (0)
    
    TokenType prev_type = TT_NONE;
    ExprNode multiply = {.self = {TT_BINOP, {.binop = BINOP_MULT}}};
    while (subframe.begin < subframe.end) { // while there exists a next token
        switch (prev_type) {
        case TT_NONE: case TT_BINOP: case TT_UNPRECOP: case TT_LPARE:
            switch (subframe.begin->type) {
            case TT_BFUNC:
                if (expr_parse_BFUNC(&subframe)) error_cleanup();
                break;
            case TT_NUMBER: case TT_VAR: case TT_BVAR: case TT_UNPRECOP: case TT_LPARE:
                da_append(&node_list, (ExprNode) {subframe.begin[0]});
                subframe.begin += 1;
                break;
            default: error_cleanup();
            }
            break;
        case TT_NUMBER: case TT_VAR: case TT_BVAR: case TT_BFUNC: case TT_RPARE:
            switch (subframe.begin->type) {
            case TT_BFUNC:
                da_append(&node_list, multiply); // implicit multiplication
                if (expr_parse_BFUNC(&subframe)) error_cleanup();
                break;
            case TT_NUMBER: case TT_VAR: case TT_BVAR: case TT_LPARE:
                da_append(&node_list, multiply); // implicit multiplication
                // fallthrough
            case TT_BINOP: case TT_RPARE:
                da_append(&node_list, (ExprNode) {subframe.begin[0]});
                subframe.begin += 1;
                break;
            default: error_cleanup();
            }
            break;
        // @assert unreachable
        default: ;
        }
        prev_type = node_list.items[node_list.count - 1].self.type;
    }

#undef error_cleanup
    
    // 2.
    // @algo: build AST from infix expression
    // if operand: add to parent node
    // if op: pop till a lower precedence op, then push
    //   note (extendability): lower / lowerOrEqual depends on the ASSOCIATIVITY of the binop
    // if (: push
    // if ): pop till (, or error
    // pop := check available operands, pick 1~2 make a layer 

    ExprNode node = {}; // operand stack as well as the result tree, shallow copies from `node_list`
    ExprNode op_stack = {}; // shallow copies from `node_list`
#define error_cleanup()                         \
    do {                                        \
        da_free(&node);                         \
        da_free(&op_stack);                     \
        expr_free_node(&node_list);             \
        return 1;                               \
    } while (0)
    
    for (ExprNode * elem = node_list.items; elem < node_list.items + node_list.count; ++elem) {
        switch (elem->self.type) {
        case TT_NUMBER: case TT_VAR: case TT_BVAR: case TT_BFUNC:
            da_append(&node, *elem);
            break;
            
        case TT_BINOP: case TT_UNPRECOP: {
            int thisprec = get_op_prec(elem->self); // [!]
            while (op_stack.count > 0 &&
                   thisprec >= get_op_prec(op_stack.items[op_stack.count - 1].self)) {
                // pop
                if (build_op_node(&node, op_stack.items[op_stack.count - 1])) error_cleanup();
                op_stack.count -= 1;
            }
            // ok if count == 0
            da_append(&op_stack, *elem);
        } break;
            
        case TT_LPARE:
            da_append(&op_stack, *elem);
            break;

        case TT_RPARE:
            while (op_stack.count > 0 &&
                   op_stack.items[op_stack.count - 1].self.type != TT_LPARE) {
                // pop
                if (build_op_node(&node, op_stack.items[op_stack.count - 1])) error_cleanup();
                op_stack.count -= 1;
            }
            // this should never happen! dealt within `seek_expr_end`
            if (op_stack.count == 0) error_cleanup();
            // delete the open parenthesis
            op_stack.count -= 1;
            break;
        // @assert unreachable
        default: ;
        }
    }
    while (op_stack.count > 0 &&
           !build_op_node(&node, op_stack.items[op_stack.count - 1])) op_stack.count -= 1;
    if (op_stack.count > 0) error_cleanup();

    if (node.count != 1) error_cleanup();

    // success and cleanup
    da_free(&op_stack); // only delete the container
    da_free(&node_list); // this can contain info that overlaps with main AST
    da_append(frame->parent, node.items[0]);
    da_free(&node);
    frame->begin = frame->end;
    return 0;
    
#undef error_cleanup
} // expr_parse

// @return NULL if early end, otherwise the location of saught RPARE
Token * seek_expr_end(Token * begin, Token * end) { // end is oob pointer
    size_t depth = 1; // including outmost layer
    while (begin < end && depth > 0) {
        depth += (begin->type == TT_LPARE) - (begin->type == TT_RPARE);
        begin += 1;
    }
    if (depth > 0) return NULL;
    return begin - 1; // overshoots at the last cycle
}

int expr_parse_BFUNC(Expr_Builder_Frame * frame) {
    if (frame->begin + 1 >= frame->end || frame->begin->type != TT_BFUNC) return 1;
    ExprNode node = {.self = frame->begin[0]};

    // f(expr)
    if (frame->begin[1].type == TT_LPARE) {
        Expr_Builder_Frame subframe = {
            .parent = &node,
            .begin = frame->begin + 2,
            .end = seek_expr_end(frame->begin + 2, frame->end),
        };
        if (subframe.end == NULL) return 1;
        if (expr_parse(&subframe)) return 1; // node should not have anything in it anyway, freeing is redundant
        // @assert subframe.begin == subframe.end
        da_append(frame->parent, node);
        frame->begin = subframe.end + 1;
        return 0;
    }

    // ffx
    if (frame->begin[1].type == TT_BFUNC) {
        Expr_Builder_Frame subframe = {
            .parent = &node,
            .begin = frame->begin + 1,
            .end = frame->end,
        };
        if (expr_parse_BFUNC(&subframe)) return 1; // same as above
        da_append(frame->parent, node);
        frame->begin = subframe.begin;
        return 0;
    }
    
    // fxfy => f(x) * f(y), f2x => f(2x), f2(x) => f(2) * x
    Token * end = frame->begin + 1;
    while (end < frame->end &&
           (end->type == TT_NUMBER ||
            end->type == TT_VAR ||
            end->type == TT_BVAR)) {
        end += 1;
    }
    Expr_Builder_Frame subframe = {
        .parent = &node,
        .begin = frame->begin + 1,
        .end = end,
    };
    if (expr_parse(&subframe)) return 1; // same as above, here building a series of multiplications
    da_append(frame->parent, node);
    frame->begin = end;
    return 0;
}


// evaluate - demo, only one var
//#define M_PI 3.14159265358979323846
//#define M_E 2.7182818284590452354
float expr_eval(ExprNode node, float x) {
    switch (node.self.type) {
    case TT_NONE: return expr_eval(node.items[0], x); // redundant layer
    case TT_NUMBER: return node.self.as.number;
    case TT_VAR: return NAN; // not implemented
    case TT_BVAR:
        switch (node.self.as.bvar) {
        case BVAR_E: return M_E;
        case BVAR_PI: return M_PI;
        case BVAR_X: return x;
        default: return NAN;
        }
    case TT_BFUNC: {
        float t = expr_eval(node.items[0], x);
        switch (node.self.as.bfunc) {
        case BFUNC_SINH: return sinhf(t);
        case BFUNC_COSH: return coshf(t);
        case BFUNC_TANH: return tanhf(t);
        case BFUNC_ASIN: return asinf(t);
        case BFUNC_ACOS: return acosf(t);
        case BFUNC_ATAN: return atanf(t);
        case BFUNC_SIN: return sinf(t);
        case BFUNC_COS: return cosf(t);
        case BFUNC_TAN: return tanf(t);
        case BFUNC_EXP: return expf(t);
        case BFUNC_LOG: return logf(t);
            //case BFUNC_POW: return pow(??);
        case BFUNC_SQRT: return sqrtf(t);
        case BFUNC_FLOOR: return floorf(t);
        case BFUNC_CEIL: return ceilf(t);
        case BFUNC_ROUND: return roundf(t);
        case BFUNC_ABS: return fabsf(t);
        case BFUNC_SGN: return (t > 0) - (t < 0);
            //case BFUNC_HYPOT: return hypotf(??);
            //case BFUNC_MIN: return ;
            //case BFUNC_MAX: return ;
        }
    } break;
    case TT_BINOP: {
        float l = expr_eval(node.items[0], x);
        float r = expr_eval(node.items[1], x);
        switch (node.self.as.binop) {
        case BINOP_PLUS: return l+r;
        case BINOP_MINUS: return l-r;
        case BINOP_MULT: return l*r;
        case BINOP_DIV: return l/r;
        case BINOP_MOD: return fmodf(l, r);
        default: return NAN;
        }
    } break;
    case TT_UNPRECOP: {
        float t = expr_eval(node.items[0], x);
        switch (node.self.as.unprecop) {
        case UPOP_PLUS: return t;
        case UPOP_MINUS: return -t;
        default: return NAN;
        }
    } break;
    default: return NAN;
    }
}


void expr_free_node(ExprNode * node) {
    for (size_t i = 0; i < node->count; ++i) {
        expr_free_node(node->items + i);
    }
    da_free(node);
}

void token_print(Token tok) {
    switch (tok.type) {
    case TT_NONE:
        printf("None");
        break;
    case TT_NUMBER:
        printf("Number (%f)", tok.as.number);
        break;
    case TT_VAR:
        printf("Var");
        break;
    case TT_BVAR:
        printf("BuiltinVar");
        break;
    case TT_BFUNC:
        printf("BuiltinFunc");
        break;
    case TT_BINOP:
        printf("BinaryOp (%s)",
               tok.as.binop == BINOP_PLUS ? "plus" :
               tok.as.binop == BINOP_MINUS ? "minus" :
               tok.as.binop == BINOP_MULT ? "mult" :
               tok.as.binop == BINOP_DIV ? "div" :
               tok.as.binop == BINOP_MOD ? "mod" :
               "");
        break;
    case TT_UNPRECOP:
        printf("UnaryOp (%s)",
               tok.as.unprecop == UPOP_PLUS ? "plus" :
               tok.as.unprecop == UPOP_MINUS ? "minus" :
               "");
        break;
    case TT_LPARE:
        printf("(");
        break;
    case TT_RPARE:
        printf(")");
        break;
    }
}

void expr_print(ExprNode node, int indent) {
    printf("%*s", indent * 2, "");
    token_print(node.self);
    if (node.count > 0) {
        printf(" {\n");
        for (size_t i = 0; i < node.count; ++i) {
            expr_print(node.items[i], indent + 1);
        }
        printf("%*s}", indent * 2, "");
    }
    printf("\n");
}

int equation_parse(Equation * eq) {
    expr_free_node(&eq->expr); // cleanup old

    printf("Parsing equation: %s\n", eq->text.items);
    
    Tokens tokens = {}; // shall not be referenced by expr tree
    eq->state = expr_tokenize(&tokens, eq->text) == 0 ? ES_VALID : ES_INVALID;
    if (eq->state == ES_INVALID) {
        da_free(&tokens);
        return 1;
    }

    printf("Tokenizer output: ");
    for (size_t i = 0; i < tokens.count; ++i) {
        token_print(tokens.items[i]);
        printf(", ");
    }
    printf("\n");

    Expr_Builder_Frame rootframe = {
        &eq->expr,
        tokens.items,
        tokens.items + tokens.count,
    };
    eq->state = expr_parse(&rootframe) == 0 ? ES_VALID : ES_INVALID;
    if (eq->state == ES_INVALID || rootframe.begin != rootframe.end) {
        da_free(&tokens);
        // `expr` should have nothing in it
        return 1;
    }

    printf("Syntax tree:\n");
    expr_print(eq->expr, 0);
    printf("\n\n");

    da_free(&tokens);
    return 0;
}

void equation_free(Equation * eq) {
    da_free(&eq->editor);
    da_free(&eq->text);
    expr_free_node(&eq->expr);
}

#endif // EQUATION_H_
