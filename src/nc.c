#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define MAX_BUFFER_LENGTH 256

enum {
  AST_NUMBER,
  AST_SYMBOL,
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
    Var *var;
    struct {
      struct AST *left;
      struct AST *right;
    };
  };
}  AST;

Var *vars = NULL;
void error(char *fmt, ...) __attribute__((noreturn));
AST *read_expr(void);
void emit_expr(AST *ast);

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

Var *find_var(char *name) {
 Var *v = vars;
 for (; v; v = v->next) {
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

AST *read_symbol(c) {
  char *buf = malloc(MAX_BUFFER_LENGTH);
  buf[0] = c;
  int i = 1;
  while(1) {
    int c = getc(stdin);
    if (!isalpha(c)) {
      ungetc(c, stdin);
      break;
    }
    buf[i++] = c;
    if (i == MAX_BUFFER_LENGTH - 1)
      error("Symbol too long");
  }
  buf[i] = '\0';
  Var *v = find_var(buf);
  if (!v) v = make_var(buf);
  return make_ast_sym(v);
}


AST *read_prim(void) {
  int c = getc(stdin);
  if (isdigit(c))
    return read_number(c - '0');
  if (isalpha(c))
    return read_symbol(c);
  else if (c == EOF)
    return NULL;
  error("Don't know how to handle '%c'", c);
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
    default:
      emit_binop(ast);
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
    default:
      printf("(%c ", ast->type);
      print_ast(ast->left);
      printf(" ");
      print_ast(ast->right);
      printf(")");
  }
}

int main(int argc, char **argv) {
  int wantast = (argc > 1 && !strcmp(argv[1], "-a"));
  if (!wantast) {
    printf(".text\n\t"
           ".global mymain\n"
           "mymain:\n\t");
  }
  for (;;) {
    AST *ast = read_expr();
    if (!ast) break;
    if (wantast)
      print_ast(ast);
    else
      emit_expr(ast);
  }
  if (!wantast)
    printf("ret\n");
  return 0;
}
