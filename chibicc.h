#ifndef CHIBICC_H
#define CHIBICC_H

#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <glob.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#ifndef UINT64_C
# define UINT64_C(x) (x##ULL)
#endif
#ifndef INT64_C
# define INT64_C(x) (x##LL)
#endif
#ifndef UINT32_C
# define UINT32_C(x) (x##U)
#endif
#ifndef INT32_C
# define INT32_C(x) (x)
#endif
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void chibicc_assert_fail(const char *expr, const char *file, int line, const char *func);

#undef assert
#ifdef NDEBUG
# define assert(expr) ((void)0)
#else
# define assert(expr) \
    ((expr) ? (void)0 : chibicc_assert_fail(#expr, __FILE__, __LINE__, __func__))
#endif

#ifndef MAX
#define MAX(x, y) ((x) < (y) ? (y) : (x))
#endif
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef __GNUC__
# define __attribute__(x)
#endif

typedef struct Type Type;
typedef struct Node Node;
typedef struct Member Member;
typedef struct Relocation Relocation;
typedef struct Hideset Hideset;

//
// strings.c
//

typedef struct {
  char **data;
  int capacity;
  int len;
} StringArray;

void strarray_push(StringArray *arr, char *s);
char *format(char *fmt, ...) __attribute__((format(printf, 1, 2)));
extern StringArray tmpfiles;
char *create_tmpfile(void);

//
// tokenize.c
//

// Token
typedef enum {
  TK_IDENT,   // Identifiers
  TK_PUNCT,   // Punctuators
  TK_KEYWORD, // Keywords
  TK_STR,     // String literals
  TK_NUM,     // Numeric literals
  TK_PP_NUM,  // Preprocessing numbers
  TK_EOF,     // End-of-file markers
} TokenKind;

typedef struct File File;
struct File {
  char *name;
  int file_no;
  char *contents;

  // For #line directive
  char *display_name;
  int line_delta;
};

// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind;   // Token kind
  Token *next;      // Next token
  int64_t val;      // If kind is TK_NUM, its value
  long double fval; // If kind is TK_NUM, its value
  char *loc;        // Token location
  int len;          // Token length
  Type *ty;         // Used if TK_NUM or TK_STR
  char *str;        // String literal contents including terminating '\0'

  File *file;       // Source location
  char *filename;   // Filename
  int line_no;      // Line number
  int line_delta;   // Line number
  uint32_t at_bol : 1;      // True if this token is at beginning of line
  uint32_t has_space : 1;   // True if this token follows a space character
  Hideset *hideset; // For macro expansion
  Token *origin;    // If this is expanded from a macro, the original token
};

noreturn void error(char *fmt, ...) __attribute__((format(printf, 1, 2)));
noreturn void error_at(char *loc, char *fmt, ...) __attribute__((format(printf, 2, 3)));
noreturn void error_tok(Token *tok, char *fmt, ...) __attribute__((format(printf, 2, 3)));
void warn_tok(Token *tok, char *fmt, ...) __attribute__((format(printf, 2, 3)));
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *op);
bool consume(Token **rest, Token *tok, char *str);
void convert_pp_tokens(Token *tok);
File **get_input_files(void);
File *new_file(char *name, int file_no, char *contents);
Token *tokenize_string_literal(Token *tok, Type *basety);
Token *tokenize(File *file);
Token *tokenize_file(char *filename);

#define unreachable() \
  error("internal error at %s:%d", __FILE__, __LINE__)

//
// preprocess.c
//

#include "preprocess.h"

//
// parse.c
//

typedef struct ABI ABI;

// Variable or function
typedef struct Obj2 Obj2;
typedef Obj2 Obj;

struct Obj2 {
  Obj2 *next;
  char *name;    // Variable name
  Type *ty;      // Type
  Token *tok;    // representative token
  int align;     // alignment

  // Flags packed as bitfields
  uint32_t is_local : 1;      // local or global/function
  uint32_t is_function : 1;   // function or variable
  uint32_t is_definition : 1; // is definition or forward declaration
  uint32_t is_static : 1;     // static linkage
  uint32_t is_tentative : 1;  // tentative definition
  uint32_t is_tls : 1;        // thread-local storage
  uint32_t is_readonly : 1;   // const / read-only data
  uint32_t is_inline : 1;     // inline function
  uint32_t is_live : 1;       // static inline function reached / referenced
  uint32_t is_root : 1;       // static inline root
  uint32_t is_asm : 1;        // top-level asm block

  // Union payload partitioned by object kind
  union {
    // Local variable
    struct {
      int offset;
    };

    // Global variable
    struct {
      char *init_data;
      Relocation *rel;
    };

    // Function
    struct {
      Obj2 *params;
      Node *body;
      Obj2 *locals;
      Obj2 *va_area;
      Obj2 *alloca_bottom;
      int stack_size;
      int callee_saved_mask;
      ABI *abi; // Specific ABI / calling convention override for this function
      StringArray refs; // Static inline function refs
    };

    // Top-level asm
    struct {
      char *asm_str;
    };
  };
};

// Global variable can be initialized either by a constant expression
// or a pointer to another global variable. This struct represents the
// latter.
typedef struct Relocation Relocation;
struct Relocation {
  Relocation *next;
  int offset;
  char **label;
  long addend;
};

// AST node
typedef enum {
  ND_NULL_EXPR, // Do nothing
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_NEG,       // unary -
  ND_MOD,       // %
  ND_BITAND,    // &
  ND_BITOR,     // |
  ND_BITXOR,    // ^
  ND_SHL,       // <<
  ND_SHR,       // >>
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_ASSIGN,    // =
  ND_COND,      // ?:
  ND_COMMA,     // ,
  ND_MEMBER,    // . (struct member access)
  ND_ADDR,      // unary &
  ND_DEREF,     // unary *
  ND_NOT,       // !
  ND_BITNOT,    // ~
  ND_LOGAND,    // &&
  ND_LOGOR,     // ||
  ND_RETURN,    // "return"
  ND_IF,        // "if"
  ND_FOR,       // "for" or "while"
  ND_DO,        // "do"
  ND_SWITCH,    // "switch"
  ND_CASE,      // "case"
  ND_BLOCK,     // { ... }
  ND_GOTO,      // "goto"
  ND_GOTO_EXPR, // "goto" labels-as-values
  ND_LABEL,     // Labeled statement
  ND_LABEL_VAL, // [GNU] Labels-as-values
  ND_FUNCALL,   // Function call
  ND_EXPR_STMT, // Expression statement
  ND_STMT_EXPR, // Statement expression
  ND_VAR,       // Variable
  ND_VLA_PTR,   // VLA designator
  ND_NUM,       // Integer
  ND_CAST,      // Type cast
  ND_MEMZERO,   // Zero-clear a stack variable
  ND_ASM,       // "asm"
  ND_CAS,       // Atomic compare-and-swap
  ND_EXCH,      // Atomic exchange
} NodeKind;

typedef struct AsmOperand AsmOperand;
typedef struct LLIRVReg LLIRVReg;
typedef struct LLIRVReg IRVReg;

struct AsmOperand {
  AsmOperand *next;
  char *name;         // "[name]" or NULL
  char *constraint;   // e.g. "=r", "+m", "i", "0", etc.
  Node *expr;         // C expression (AST node)
  uint32_t is_output : 2;      // 1: output, 0: input, 2: goto label
  uint32_t is_imm_val : 1;    // True if imm_val is valid
  char *label_name;   // For asm goto
  char *unique_label; // Resolved unique label for asm goto
  Token *tok;         // Token for error reporting
  IRVReg *vreg;       // Allocated vreg in IR
  IRVReg *addr_vreg;  // Address vreg in IR (for indirect memory operands)
  Obj2 *var;          // Direct variable (if local or global variable)
  int64_t imm_val;    // Immediate value (if constant)
};

typedef struct AsmClobber AsmClobber;
struct AsmClobber {
  AsmClobber *next;
  char *clobber;      // e.g. "memory", "cc", "rax"
};

// AST node type
struct Node {
  NodeKind kind; // Node kind
  Node *next;    // Next node
  Type *ty;      // Type, e.g. int or pointer to int
  Token *tok;    // Representative token

  Node *lhs;     // Left-hand side
  Node *rhs;     // Right-hand side

  // Compact union payload to minimize AST memory footprint
  union
  {
    // Control flow: "if", "for", "while", "do", "switch", "case", "goto", labels, or "?:" conditional
    struct {
      Node *cond;
      Node *then;
      Node *els;
      Node *init;
      Node *inc;
      char *brk_label;
      char *cont_label;
      Node *case_next;
      Node *default_case;
      long begin;
      long end;
      char *label;
      char *unique_label;
      Node *goto_next;
    };

    // Block or statement expression
    struct {
      Node *body;
    };

    // Struct member access
    struct {
      Member *member;
    };

    // Function call
    struct {
      Type *func_ty;
      Node *args;
      Obj2 *ret_buffer;
      uint32_t pass_by_stack : 1;
    };

    // "asm" inline assembly
    struct {
      char *asm_str;
      AsmOperand *asm_outputs;
      AsmOperand *asm_inputs;
      AsmClobber *asm_clobbers;
      AsmOperand *asm_labels;
      uint32_t asm_is_volatile : 1;
      uint32_t asm_is_inline : 1;
      uint32_t asm_is_goto : 1;
    };

    // Atomic compare-and-swap and atomic op= operators
    struct {
      Node *cas_addr;
      Node *cas_old;
      Node *cas_new;
      Obj2 *atomic_addr;
      Node *atomic_expr;
    };

    // Variable or VLA designator or memzero
    struct {
      Obj2 *var;
    };

    // Numeric literal
    struct {
      int64_t val;
      long double fval;
    };
  };
};

typedef struct VarScope VarScope;
struct VarScope {
  Obj2 *var;
  Type *type_def;
  Type *enum_ty;
  int enum_val;
  char *func_name;
};

Node *new_cast(Node *expr, Type *ty);
Node *new_node(NodeKind kind, Token *tok);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok);
Node *new_unary(NodeKind kind, Node *expr, Token *tok);
Node *new_num(int64_t val, Token *tok);
Node *new_long(int64_t val, Token *tok);
Node *new_ulong(long val, Token *tok);
Node *new_var_node(Obj2 *var, Token *tok);
Node *new_vla_ptr(Obj2 *var, Token *tok);
Node *new_add(Node *lhs, Node *rhs, Token *tok);
Node *new_sub(Node *lhs, Node *rhs, Token *tok);
Node *new_if_node(Node *cond, Node *then, Node *els, Token *tok);
Node *new_for_node(Node *init, Node *cond, Node *inc, Node *then, Token *tok);
Node *new_do_node(Node *then, Node *cond, Token *tok);
Node *new_switch_node(Node *cond, Node *then, Token *tok);
Node *new_case_node(long begin, long end, Token *tok);
Node *new_block_node(Node *body, Token *tok);
Node *new_goto_node(char *label, Token *tok);
Node *new_goto_expr_node(Node *expr, Token *tok);
Node *new_label_node(char *label, Token *tok);
Node *new_return_node(Node *expr, Token *tok);
Node *new_expr_stmt_node(Node *expr, Token *tok);
Node *new_stmt_expr_node(Node *body, Token *tok);
Node *new_member_node(Node *lhs, Member *member, Token *tok);
Node *new_funcall_node(Token *tok, Type *func_ty, Node *args);
Node *new_asm_node(char *asm_str, Token *tok);
Node *new_cas_node(Node *addr, Node *old_val, Node *new_val, Token *tok);
Node *new_exch_node(Node *addr, Node *val, Token *tok);

const char *node_kind_name(NodeKind kind);
bool node_is_binary(NodeKind kind);
bool node_is_unary(NodeKind kind);
bool node_is_control_flow(NodeKind kind);
bool node_is_atomic(NodeKind kind);

Obj2 *new_lvar(char *name, Type *ty);
VarScope *find_var(Token *tok);
VarScope *push_scope(char *name);
int64_t const_expr(Token **rest, Token *tok);
Obj2 *parse(Token *tok);

//
// type.c
//

typedef enum {
  TY_VOID,
  TY_BOOL,
  TY_CHAR,
  TY_SHORT,
  TY_INT,
  TY_LONG,
  TY_LONGLONG,
  TY_FLOAT,
  TY_DOUBLE,
  TY_LDOUBLE,
  TY_ENUM,
  TY_PTR,
  TY_FUNC,
  TY_ARRAY,
  TY_VLA, // variable-length array
  TY_STRUCT,
  TY_UNION,
} TypeKind;

struct Type {
  TypeKind kind;
  int size;           // sizeof() value
  int align;          // alignment
  uint32_t is_unsigned : 1;   // unsigned or signed
  uint32_t is_atomic : 1;     // true if _Atomic
  uint32_t is_flexible : 1;   // struct with flexible array member
  uint32_t is_packed : 1;     // packed struct
  uint32_t is_variadic : 1;   // variadic function type
  Type *origin;       // for type compatibility check

  // Pointer-to or array-of type. We intentionally use the same member
  // to represent pointer/array duality in C.
  Type *base;

  // Declaration
  Token *name;
  Token *name_pos;

  Type *next; // for parameter list / type chaining

  // Union payload partitioned by kind
  union {
    // Array
    struct {
      int array_len;
    };

    // Variable-length array
    struct {
      Node *vla_len; // # of elements
      Obj2 *vla_size; // sizeof() value
    };

    // Struct / Union
    struct {
      Member *members;
    };

    // Function type
    struct {
      Type *return_ty;
      Type *params;
      ABI *abi; // Specific ABI / calling convention for this function type
    };
  };
};

// Struct member
struct Member {
  Member *next;
  Type *ty;
  Token *tok; // for error message
  Token *name;
  int idx;
  int align;
  int offset;

  // Bitfield
  uint32_t is_bitfield : 1;
  int bit_offset;
  int bit_width;
};

extern Type *ty_void;
extern Type *ty_bool;

extern Type *ty_char;
extern Type *ty_short;
extern Type *ty_int;
extern Type *ty_long;
extern Type *ty_llong;

extern Type *ty_uchar;
extern Type *ty_ushort;
extern Type *ty_uint;
extern Type *ty_ulong;
extern Type *ty_ullong;

extern Type *ty_float;
extern Type *ty_double;
extern Type *ty_ldouble;

bool is_integer(Type *ty);
bool is_flonum(Type *ty);
bool is_numeric(Type *ty);
bool is_compatible(Type *t1, Type *t2);
Type *copy_type(Type *ty);
Type *pointer_to(Type *base);
Type *func_type(Type *return_ty);
Type *array_of(Type *base, int size);
Type *vla_of(Type *base, Node *expr);
Type *enum_type(void);
Type *struct_type(void);
void add_type(Node *node);

//
// codegen and ABI
//

#include "abi/abi.h"
#include "codegen/codegen.h"
#include "codegen/common/common.h"

void codegen(Obj2 *prog, FILE *out);
void init_target(const char *target_name, const char *abi_name);
void init_all_targets_and_abis(void);

static inline ABI *get_node_abi(Node *node) {
  if (node && node->func_ty && node->func_ty->abi)
    return node->func_ty->abi;
  return current_abi;
}

static inline ABI *get_fn_abi(Obj2 *fn) {
  if (fn && fn->abi)
    return fn->abi;
  if (fn && fn->ty && fn->ty->abi)
    return fn->ty->abi;
  return current_abi;
}

//
// unicode.c
//

int encode_utf8(char *buf, uint32_t c);
uint32_t decode_utf8(char **new_pos, char *p);
bool is_ident1(uint32_t c);
bool is_ident2(uint32_t c);
int display_width(char *p, int len);

//
// hashmap.c
//

typedef struct {
  char *key;
  int keylen;
  void *val;
} HashEntry;

typedef struct HashMap HashMap;
struct HashMap {
  HashEntry *buckets;
  int capacity;
  int used;
};

void *hashmap_get(HashMap *map, char *key);
void *hashmap_get2(HashMap *map, char *key, int keylen);
void hashmap_put(HashMap *map, char *key, void *val);
void hashmap_put2(HashMap *map, char *key, int keylen, void *val);
void hashmap_delete(HashMap *map, char *key);
void hashmap_delete2(HashMap *map, char *key, int keylen);
void hashmap_test(void);

//
// main.c
//

bool file_exists(const char *path);

extern StringArray include_paths;
extern bool opt_fpic;
extern bool opt_fcommon;
extern bool opt_ffunction_sections;
extern bool opt_fdata_sections;
extern bool opt_g;
extern int opt_O;
extern bool opt_dump_ir;
extern char *base_file;

#endif // CHIBICC_H
