#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define MAX_BUFFER_LENGTH 256
#define MAX_ARGS 6
#define EXPR_LEN 100

enum {
  AST_NUMBER,
  AST_SYMBOL,
  AST_FUNCTION_CALL,
  AST_STRING,
};

typedef struct Var {
  char *name;
  int pos;
  struct Var *next;
} Var;

typedef struct AST {
  char type;
  union {
    int num_val;
    char *str_val;
    struct {
      char *sval;
      int sid;
      struct AST *snext;
    };
    Var *var;
    struct {
      struct AST *left;
      struct AST *right;
    };
    struct {
      char *fname;
      int nargs;
      struct AST **args;
    };
  };
}  AST;

Var *vars = NULL;
AST *strings = NULL;
char *REGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
void error(char *fmt, ...) __attribute__((noreturn));
AST *read_expr(void);
void emit_expr(AST *ast);
AST *read_expr2(int prec);
AST *read_string(void);

void error(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(1);
}

AST *make_ast_op(char type, AST *left, AST *right) {
  AST *r = malloc(sizeof(AST));
  r->type = type;
  r->left = left;
  r->right = right;
  return r;
}

AST *make_ast_int(int val) {
  AST *r = malloc(sizeof(AST));
  r->type = AST_NUMBER;
  r->num_val = val;
  return r;
}

AST *make_ast_sym(Var *var) {
  AST *r = malloc(sizeof(AST));
  r->type = AST_SYMBOL;
  r->var =var;
  return r;
}

AST *make_ast_str(char *str) {
  AST *r = malloc(sizeof(AST));
  r->type = AST_STRING;
  r->sval = str;
  if (strings == NULL) {
    r->sid = 0;
    r->snext = NULL;
  } else {
    r->sid = strings->sid + 1;
    r->snext = strings;
  }
  strings = r;
  return r;
}

AST *make_ast_funcall(char *fname, int nargs, AST **args) {
  AST *r = malloc(sizeof(AST));
  r->type = AST_FUNCTION_CALL;
  r->fname = fname;
  r->nargs = nargs;
  r->args = args;
  return r;
}


Var *find_var(char *name) {
for (Var *v = vars; v; v = v->next) {
    if (!strcmp(name, v->name))
      return v;
  }
  return NULL;
}

Var *make_var(char *name) {
  Var *v = malloc(sizeof(Var));
  v->name = name;
  v->pos = vars ? vars->pos + 1 : 1;
  v->next = vars;
  vars = v;
  return v;
}

void skip_space(void) {
  int c;
  while ((c = getc(stdin)) != EOF) {
    if (isspace(c))
      continue;
    ungetc(c, stdin);
    return;
  }
}

int priority(char op) {
  switch (op) {
    case '=':
      return 1;
    case '+':
    case '-':
        return 2;
    case '*':
    case '/':
        return 3;
    default:
        return -1;
  }
}

AST *read_number(int n) {
  for (;;) {
    int c = getc(stdin);
    if (!isdigit(c)) {
      ungetc(c, stdin);
      return make_ast_int(n);
    }
    n = n * 10 + (c - '0');
  }
}

char *read_ident(char c) {
  char *buf = malloc(MAX_BUFFER_LENGTH);
  buf[0] = c;
  int i = 1;
  while(1) {
    int c = getc(stdin);
    if (!isalnum(c)) {
      ungetc(c, stdin);
      break;
    }
    buf[i++] = c;
    if (i == MAX_BUFFER_LENGTH - 1)
      error("Identifier too long");
  }
  buf[i] = '\0';
  return buf;
}

AST *read_func_args(char *fname) {
  AST **args = malloc(sizeof(AST*) * (MAX_ARGS + 1));
  int i = 0, nargs = 0;
  for (; i < MAX_ARGS; i++) {
    skip_space();
    char c = getc(stdin);
   if (c == ')') break;
    ungetc(c, stdin);
    args[i] = read_expr2(0);
    nargs++;
    c = getc(stdin);
    if (c == ')') break;
    if (c == ',') skip_space();
    else error("Unexpected character: '%c'", c);
  }
  if (i == MAX_ARGS)
    error("Too many arguments: %s", fname);
  return make_ast_funcall(fname, nargs, args);
}

AST *read_ident_or_func(char c) {
  char *name = read_ident(c);
  skip_space();
  char c2 = getc(stdin);
  if (c2 == '(')
    return read_func_args(name);
  ungetc(c2, stdin);
  Var *v = find_var(name);
  if (!v) v = make_var(name);
  return make_ast_sym(v);
}


AST *read_prim(void) {
  int c = getc(stdin);
  if (isdigit(c))
    return read_number(c - '0');
  if (c == '"')
    return read_string();
  if (isalpha(c))
    return read_ident_or_func(c);
  else if (c == EOF)
    return NULL;
  error("Don't know how to handle '%c'", c);
}

AST *read_string(void) {
  char *buf = malloc(MAX_BUFFER_LENGTH);
  int i = 0;
  for (;;) {
    int c = getc(stdin);
    if (c == EOF)
      error("Unterminated string");
    if (c == '"')
      break;
    if (c == '\\') {
      c = getc(stdin);
     if (c == EOF) error("Unterminated \\");
    }
    buf[i++] = c;
    if (i == MAX_BUFFER_LENGTH - 1)
      error("String too long");
  }
  buf[i] = '\0';
  return make_ast_str(buf);
}


AST *read_expr2(int prec) {
  skip_space();
  AST *ast = read_prim();
  if(!ast)
    return NULL;
  for (;;) {
    skip_space();
    int c = getc(stdin);
    if (c == EOF) return ast;
    int prec2 = priority(c);
    if (prec2 < 0 || prec2 < prec) {
      ungetc(c, stdin);
      return ast;
    }
    skip_space();
    ast = make_ast_op(c, ast, read_expr2(prec2 + 1));
  }
  return ast;
}

AST *read_expr(void) {
  AST *r = read_expr2(0);
  if (!r) return NULL;
  skip_space();
  int c = getc(stdin);
  if (c != ';')
    error("Unterminated expression");
  return r;
}

void emit_binop(AST *ast) {
  if (ast->type == '=') {
   emit_expr(ast->right);
  if (ast->left->type != AST_SYMBOL)
    error("Symbol expected");
  printf("mov %%eax, -%d(%%rbp)\n\t", ast->left->var->pos * 4);
  return;
  }

  char *op;
  switch (ast->type) {
    case '+': op = "add"; break;
    case '-': op = "sub"; break;
    case '*': op = "imul"; break;
    case '/': break;
    default: error("invalid operator '%c'", ast->type);
  }

  emit_expr(ast->left);
  printf("push %%rax\n\t");
  emit_expr(ast->right);
  if (ast->type == '/') {
    printf("mov %%eax, %%ebx\n\t");
    printf("pop %%rax\n\t");
    printf("mov $0, %%edx\n\t");
    printf("idiv %%ebx\n\t");
  } else {
    printf("pop %%rbx\n\t");
    printf("%s %%ebx, %%eax\n\t", op);
  }
}

void emit_expr(AST *ast) {
  switch (ast->type) {
    case AST_NUMBER:
      printf("mov $%d, %%eax\n\t", ast->num_val);
      break;
    case AST_SYMBOL:
      printf("mov -%d(%%rbp), %%eax\n\t", ast->var->pos * 4);
      break;
    case AST_FUNCTION_CALL:
      for (int i = 1; i < ast->nargs; i++)
        printf("push %%%s\n\t", REGS[i]);
      for (int i = 0; i < ast->nargs; i++) {
        emit_expr(ast->args[i]);
        printf("push %%rax\n\t");
      }
      for (int i = ast->nargs - 1; i >= 0; i--)
        printf("pop %%%s\n\t", REGS[i]);
      printf("mov $0, %%eax\n\t");
      printf("call %s\n\t", ast->fname);
      for (int i = ast->nargs - 1; i > 0; i--)
        printf("pop %%%s\n\t", REGS[i]);
      break;
    case AST_STRING:
      printf("lea .s%d(%%rip), %%rax\n\t", ast->sid);
      break;
    default:
      emit_binop(ast);
  }
}

void print_quote(char *p) {
  while (*p) {
    if (*p == '\"' || *p == '\\')
      printf("\\");
    printf("%c", *p);
    p++;
  }
}


void print_ast(AST *ast) {
  switch (ast->type) {
    case AST_NUMBER:
      printf("%d", ast->num_val);
      break;
    case AST_SYMBOL:
      printf("%s", ast->var->name);
      break;
    case AST_FUNCTION_CALL:
      printf("%s(", ast->fname);
      for (int i = 0; ast->args[i]; i++) {
        print_ast(ast->args[i]);
        if (ast->args[i + 1])
          printf(",");
      }
      printf(")");
      break;
    case AST_STRING:
      printf("\"");
      print_quote(ast->sval);
      printf("\"");
      break;
    default:
      printf("(%c ", ast->type);
      print_ast(ast->left);
      printf(" ");
      print_ast(ast->right);
      printf(")");
  }
}

void emit_data_section(void) {
  if (!strings)
    return;
  printf("\t.data\n");
  for (AST *p = strings; p; p = p->snext) {
    printf(".s%d:\n\t", p->sid);
    printf(".string \"");
    print_quote(p->sval);
    printf("\"\n");
  }
  printf("\t");
}


int main(int argc, char **argv) {
  int wantast = (argc > 1 && !strcmp(argv[1], "-a"));
  AST *exprs[EXPR_LEN];
  int i;
  for (i = 0; i < EXPR_LEN; i++) {
    AST *t = read_expr();
    if (!t) break;
    exprs[i] = t;
  }
  int nexpr = i;
  if (!wantast) {
    emit_data_section();
    printf(".text\n\t"
           ".global mymain\n"
           "mymain:\n\t");
  }
  for (i = 0; i < nexpr; i++) {
    if (wantast)
      print_ast(exprs[i]);
    else
      emit_expr(exprs[i]);
  }
  if (!wantast)
    printf("ret\n");
  return 0;
}
