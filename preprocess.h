#ifndef CHIBICC_PREPROCESS_H
#define CHIBICC_PREPROCESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Forward declarations of types used by preprocessor
typedef struct Token Token;
typedef struct File File;
typedef struct Type Type;
typedef struct MacroParam MacroParam;
typedef struct MacroArg MacroArg;
typedef struct Macro Macro;
typedef struct CondIncl CondIncl;
typedef struct Hideset Hideset;
typedef struct MacroStack MacroStack;

typedef Token *macro_handler_fn(Token *);

struct MacroParam {
  MacroParam *next;
  char *name;
};

struct MacroArg {
  MacroArg *next;
  char *name;
  bool is_va_args;
  Token *tok;
};

struct Macro {
  char *name;
  bool is_objlike; // Object-like or function-like
  MacroParam *params;
  char *va_args_name;
  Token *body;
  macro_handler_fn *handler;
};

struct MacroStack {
  MacroStack *next;
  Macro *macro;
};

typedef enum {
  IN_THEN,
  IN_ELIF,
  IN_ELSE
} CondInclCtx;

struct CondIncl {
  CondIncl *next;
  CondInclCtx ctx;
  Token *tok;
  bool included;
};

struct Hideset {
  Hideset *next;
  char *name;
};

// Preprocessor public API
char *search_include_paths(char *filename);
void init_macros(void);
void define_macro(char *name, char *buf);
void undef_macro(char *name);
Token *preprocess(Token *tok);

#endif // CHIBICC_PREPROCESS_H
