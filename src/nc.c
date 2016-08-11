#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define MAX_BUFFER_LENGTH 256

enum {
  AST_NUMBER,
  AST_STRING,
};

typedef struct AST {
  char type;
  union {
    int num_val;
    char *str_val;
    struct {
      struct AST *left;
      struct AST *right;
    };
  };
}  AST;

void error(char *fmt, ...) __attribute__((noreturn));
void emit_intexpr(AST *ast);
AST *read_string(void);
AST *read_expr(void);

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

AST *make_ast_str(char *str) {
  AST *r = malloc(sizeof(AST));
  r->type = AST_STRING;
  r->str_val = str;
  return r;
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
    case '+':
    case '-':
        return 1;
    case '*':
    case '/':
        return 2;
    default:
      error("Unknown binary operator: %c", op);
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

AST *read_prim(void) {
  int c = getc(stdin);
  if (isdigit(c))
    return read_number(c - '0');
  else if (c == '"')
    return read_string();
  else if (c == EOF)
    error("Unexpected EOF");
  error("Don't know how to handle '%c'", c);
}

AST *read_expr2(int prec) {
  AST *ast = read_prim();
  for (;;) {
    skip_space();
    int c = getc(stdin);
    if (c == EOF) return ast;
    int prec2 = priority(c);
    if (prec2 < prec) {
      ungetc(c, stdin);
      return ast;
    }
    skip_space();
    ast = make_ast_op(c, ast, read_expr2(prec2 + 1));
  }
  return ast;
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

AST *read_expr(void) {
  return read_expr2(0);
}

void print_quote(char *p) {
  while (*p) {
    if (*p == '\"' || *p == '\\')
      printf("\\");
    printf("%c", *p);
    p++;
  }
}

void emit_string(AST *ast) {
  printf("\t.data\n"
         ".mydata:\n\t"
         ".string \"");
  print_quote(ast->str_val);
  printf("\"\n\t"
         ".text\n\t"
         ".global str_func\n"
         "str_func:\n\t"
         "lea .mydata(%%rip), %%rax\n\t"
         "ret\n");
  return;
}

void emit_binop(AST *ast) {
  char *op;
  switch (ast->type) {
    case '+': op = "add"; break;
    case '-': op = "sub"; break;
    case '*': op = "imul"; break;
    case '/': break;
    default: error("invalid operator '%c'", ast->type);
  }
  emit_intexpr(ast->left);
  printf("push %%rax\n\t");
  emit_intexpr(ast->right);
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

void ensure_intexpr(AST *ast) {
  switch (ast->type) {
    case '+': case '-': case '*': case '/': case AST_NUMBER:
      return;
    default:
      error("integer or binary operator expected");
  }
}

void emit_intexpr(AST *ast) {
  ensure_intexpr(ast);
  if (ast->type == AST_NUMBER)
    printf("mov $%d, %%eax\n\t", ast->num_val);
  else
    emit_binop(ast);
}

void print_ast(AST *ast) {
  switch (ast->type) {
    case AST_NUMBER:
      printf("%d", ast->num_val);
      break;
    case AST_STRING:
      print_quote(ast->str_val);
      break;
    default:
      printf("(%c ", ast->type);
      print_ast(ast->left);
      printf(" ");
      print_ast(ast->right);
      printf(")");
  }
}

void compile(AST *ast) {
  if (ast->type == AST_STRING) {
    emit_string(ast);
  } else {
    printf(".text\n\t"
           ".global num_func\n"
           "num_func:\n\t");
    emit_intexpr(ast);
    printf("ret\n");
  }
}

int main(int argc, char **argv) {
  AST *ast = read_expr();
  if (argc > 1 && !strcmp(argv[1], "-a"))
    print_ast(ast);
  else
    compile(ast);
  return 0;
}
