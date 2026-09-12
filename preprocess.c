// This file implements the C preprocessor.
//
// The preprocessor takes a list of tokens as an input and returns a
// new list of tokens as an output.
//
// The preprocessing language is designed in such a way that that's
// guaranteed to stop even if there is a recursive macro.
// Informally speaking, a macro is applied only once for each token.
// That is, if a macro token T appears in a result of direct or
// indirect macro expansion of T, T won't be expanded any further.
// For example, if T is defined as U, and U is defined as T, then
// token T is expanded to U and then to T and the macro expansion
// stops at that point.
//
// To achieve the above behavior, we attach for each token a set of
// macro names from which the token is expanded. The set is called
// "hideset". Hideset is initially empty, and every time we expand a
// macro, the macro name is added to the resulting tokens' hidesets.
//
// The above macro expansion algorithm is explained in this document
// written by Dave Prossor, which is used as a basis for the
// standard's wording:
// https://github.com/rui314/chibicc/wiki/cpp.algo.pdf

#include "chibicc.h"

typedef struct MacroParam MacroParam;
struct MacroParam {
  MacroParam *next;
  char *name;
};

typedef struct MacroArg MacroArg;
struct MacroArg {
  MacroArg *next;
  char *name;
  bool is_va_args;
  Token *tok;
};

typedef Token *macro_handler_fn(Token *);

typedef struct Macro Macro;
struct Macro {
  char *name;
  bool is_objlike; // Object-like or function-like
  MacroParam *params;
  char *va_args_name;
  Token *body;
  macro_handler_fn *handler;
};

// `#if` can be nested, so we use a stack to manage nested `#if`s.
typedef struct CondIncl CondIncl;
struct CondIncl {
  CondIncl *next;
  enum { IN_THEN, IN_ELIF, IN_ELSE } ctx;
  Token *tok;
  bool included;
};

typedef struct Hideset Hideset;
struct Hideset {
  Hideset *next;
  char *name;
};

static HashMap macros;
static CondIncl *cond_incl;
static HashMap pragma_once;
static int include_next_idx;

typedef struct EmbedParams EmbedParams;
struct EmbedParams {
  bool has_limit;
  long limit;
  bool has_prefix;
  Token *prefix;
  bool has_suffix;
  Token *suffix;
  bool has_if_empty;
  Token *if_empty;
  bool is_valid;
};

static Token *preprocess2(Token *tok);
static Macro *find_macro(Token *tok);
static bool expand_macro(Token **rest, Token *tok);
static char *search_embed_file(char *filename, bool is_dquote, char *cur_file_name);
static size_t get_binary_file_size(char *path);
static char *read_embed_filename(Token **rest, Token *tok, bool *is_dquote, bool is_has_embed);
static bool parse_embed_params(Token **rest, Token *tok, EmbedParams *params, bool is_has_embed);
static long eval_const_expr_token_list(Token *expr);

static bool is_ident(Token *tok) {
  return tok && (tok->kind == TK_IDENT || tok->kind == TK_KEYWORD);
}

static bool is_hash(Token *tok) {
  return tok->at_bol && equal(tok, "#");
}

static Token *skip_to_bol(Token *tok) {
  while (!tok->at_bol && tok->kind != TK_EOF)
    tok = tok->next;
  return tok;
}

// Some preprocessor directives such as #include allow extraneous
// tokens before newline. This function skips such tokens.
static Token *skip_line(Token *tok) {
  if (tok->at_bol)
    return tok;
  warn_tok(tok, "extra token");
  return skip_to_bol(tok);
}

static Token *copy_token(Token *tok) {
  Token *t = calloc(1, sizeof(Token));
  *t = *tok;
  t->next = NULL;
  return t;
}

static Token *new_eof(Token *tok) {
  Token *t = copy_token(tok);
  t->kind = TK_EOF;
  t->len = 0;
  return t;
}

static Hideset *new_hideset(char *name) {
  Hideset *hs = calloc(1, sizeof(Hideset));
  hs->name = name;
  return hs;
}

static bool hideset_contains(Hideset *hs, char *s, int len) {
  for (; hs; hs = hs->next)
    if (strlen(hs->name) == len && !strncmp(hs->name, s, len))
      return true;
  return false;
}

static Hideset *hideset_union(Hideset *hs1, Hideset *hs2) {
  Hideset head = {};
  Hideset *cur = &head;

  for (; hs1; hs1 = hs1->next) {
    if (!hideset_contains(hs2, hs1->name, strlen(hs1->name)))
      cur = cur->next = new_hideset(hs1->name);
  }
  cur->next = hs2;
  return head.next;
}

static Hideset *hideset_intersection(Hideset *hs1, Hideset *hs2) {
  Hideset head = {};
  Hideset *cur = &head;

  for (; hs1; hs1 = hs1->next)
    if (hideset_contains(hs2, hs1->name, strlen(hs1->name)))
      cur = cur->next = new_hideset(hs1->name);
  return head.next;
}

static Token *add_hideset(Token *tok, Hideset *hs) {
  Token head = {};
  Token *cur = &head;

  for (; tok; tok = tok->next) {
    Token *t = copy_token(tok);
    t->hideset = hideset_union(t->hideset, hs);
    cur = cur->next = t;
  }
  return head.next;
}

// Append tok2 to the end of tok1.
static Token *append(Token *tok1, Token *tok2) {
  if (tok1->kind == TK_EOF)
    return tok2;

  Token head = {};
  Token *cur = &head;

  for (; tok1->kind != TK_EOF; tok1 = tok1->next)
    cur = cur->next = copy_token(tok1);
  cur->next = tok2;
  return head.next;
}

static inline bool is_cond_begin(Token *tok) {
  return tok && (equal(tok, "if") || equal(tok, "ifdef") || equal(tok, "ifndef"));
}

static inline bool is_cond_branch(Token *tok) {
  return tok && (equal(tok, "elif") || equal(tok, "elifdef") || equal(tok, "elifndef") ||
                 equal(tok, "else") || equal(tok, "endif"));
}

static Token *skip_cond_incl2(Token *tok) {
  while (tok->kind != TK_EOF) {
    if (is_hash(tok) && is_cond_begin(tok->next)) {
      tok = skip_cond_incl2(tok->next->next);
      continue;
    }
    if (is_hash(tok) && equal(tok->next, "endif"))
      return tok->next->next;
    tok = tok->next;
  }
  return tok;
}

// Skip until next `#else`, `#elif`, `#elifdef`, `#elifndef` or `#endif`.
// Nested `#if` and `#endif` are skipped.
static Token *skip_cond_incl(Token *tok) {
  while (tok->kind != TK_EOF) {
    if (is_hash(tok) && is_cond_begin(tok->next)) {
      tok = skip_cond_incl2(tok->next->next);
      continue;
    }

    if (is_hash(tok) && is_cond_branch(tok->next))
      break;
    tok = tok->next;
  }
  return tok;
}

// Double-quote a given string and returns it.
static char *quote_string(char *str) {
  int bufsize = 3;
  for (int i = 0; str[i]; i++) {
    if (str[i] == '\\' || str[i] == '"')
      bufsize++;
    bufsize++;
  }

  char *buf = calloc(1, bufsize);
  char *p = buf;
  *p++ = '"';
  for (int i = 0; str[i]; i++) {
    if (str[i] == '\\' || str[i] == '"')
      *p++ = '\\';
    *p++ = str[i];
  }
  *p++ = '"';
  *p++ = '\0';
  return buf;
}

static Token *new_str_token(char *str, Token *tmpl) {
  char *buf = quote_string(str);
  return tokenize(new_file(tmpl->file->name, tmpl->file->file_no, buf));
}

// Copy all tokens until the next newline, terminate them with
// an EOF token and then returns them. This function is used to
// create a new list of tokens for `#if` arguments.
static Token *copy_line(Token **rest, Token *tok) {
  Token head = {};
  Token *cur = &head;

  for (; !tok->at_bol; tok = tok->next)
    cur = cur->next = copy_token(tok);

  cur->next = new_eof(tok);
  *rest = tok;
  return head.next;
}

static Token *new_num_token(int val, Token *tmpl) {
  char *buf = format("%d\n", val);
  return tokenize(new_file(tmpl->file->name, tmpl->file->file_no, buf));
}

static Token *read_const_expr(Token **rest, Token *tok) {
  return copy_line(rest, tok);
}

static Token *preprocess_const_expr(Token *tok) {
  Token head = {};
  Token *cur = &head;

  while (tok->kind != TK_EOF) {
    if (equal(tok, "defined")) {
      Token *start = tok;
      bool has_paren = consume(&tok, tok->next, "(");

      if (!is_ident(tok))
        error_tok(start, "macro name must be an identifier");
      Macro *m = find_macro(tok);
      tok = tok->next;

      if (has_paren)
        tok = skip(tok, ")");

      cur = cur->next = new_num_token(m ? 1 : 0, start);
      continue;
    }

    if (equal(tok, "__has_embed")) {
      Token *start = tok;
      tok = tok->next;
      if (!equal(tok, "("))
        error_tok(tok, "expected '(' after __has_embed");
      tok = tok->next;

      Token head_arg = {};
      Token *cur_arg = &head_arg;
      int depth = 1;
      while (tok->kind != TK_EOF) {
        if (equal(tok, "(")) {
          depth++;
        } else if (equal(tok, ")")) {
          depth--;
          if (depth == 0) {
            tok = tok->next;
            break;
          }
        }
        cur_arg = cur_arg->next = copy_token(tok);
        tok = tok->next;
      }
      cur_arg->next = new_eof(start);

      Token *has_embed_toks = head_arg.next;
      if (has_embed_toks && is_ident(has_embed_toks) && has_embed_toks->kind != TK_STR && !equal(has_embed_toks, "<")) {
        has_embed_toks = preprocess2(has_embed_toks);
      }

      int result = 0; // __STDC_EMBED_NOT_FOUND__
      bool is_dquote = false;
      Token *after_fn = NULL;
      char *filename = has_embed_toks ? read_embed_filename(&after_fn, has_embed_toks, &is_dquote, true) : NULL;
      if (filename) {
        if (after_fn && after_fn->kind != TK_EOF)
          after_fn = preprocess2(after_fn);

        EmbedParams params = {};
        Token *after_p = NULL;
        if (parse_embed_params(&after_p, after_fn, &params, true) && params.is_valid) {
          char *path = search_embed_file(filename, is_dquote, start->file ? start->file->name : NULL);
          if (path && file_exists(path)) {
            size_t sz = get_binary_file_size(path);
            if (sz != (size_t)-1) {
              if (params.has_limit && (size_t)params.limit < sz)
                sz = params.limit;
              if (sz == 0)
                result = 2; // __STDC_EMBED_EMPTY__
              else
                result = 1; // __STDC_EMBED_FOUND__
            }
          }
        }
      }

      cur = cur->next = new_num_token(result, start);
      continue;
    }

    if (expand_macro(&tok, tok))
      continue;

    cur = cur->next = tok;
    tok = tok->next;
  }
  cur->next = tok;
  return head.next;
}

// Read and evaluate a constant expression.
static long eval_const_expr(Token **rest, Token *tok) {
  Token *start = tok;
  Token *expr = read_const_expr(rest, tok->next);
  expr = preprocess_const_expr(expr);

  if (expr->kind == TK_EOF)
    error_tok(start, "no expression");

  // [https://www.sigbus.info/n1570#6.10.1p4] The standard requires
  // we replace remaining non-macro identifiers with "0" before
  // evaluating a constant expression. For example, `#if foo` is
  // equivalent to `#if 0` if foo is not defined.
  for (Token *t = expr; t->kind != TK_EOF; t = t->next) {
    if (is_ident(t)) {
      Token *next = t->next;
      *t = *new_num_token(0, t);
      t->next = next;
    }
  }

  // Convert pp-numbers to regular numbers
  convert_pp_tokens(expr);

  Token *rest2;
  long val = const_expr(&rest2, expr);
  if (rest2->kind != TK_EOF)
    error_tok(rest2, "extra token");
  return val;
}

static CondIncl *push_cond_incl(Token *tok, bool included) {
  CondIncl *ci = calloc(1, sizeof(CondIncl));
  ci->next = cond_incl;
  ci->ctx = IN_THEN;
  ci->tok = tok;
  ci->included = included;
  cond_incl = ci;
  return ci;
}

static Macro *find_macro(Token *tok) {
  if (!is_ident(tok))
    return NULL;
  return hashmap_get2(&macros, tok->loc, tok->len);
}

static Macro *add_macro(char *name, bool is_objlike, Token *body) {
  Macro *m = calloc(1, sizeof(Macro));
  m->name = name;
  m->is_objlike = is_objlike;
  m->body = body;
  hashmap_put(&macros, name, m);
  return m;
}

static MacroParam *read_macro_params(Token **rest, Token *tok, char **va_args_name) {
  MacroParam head = {};
  MacroParam *cur = &head;

  while (!equal(tok, ")")) {
    if (cur != &head)
      tok = skip(tok, ",");

    if (equal(tok, "...")) {
      *va_args_name = "__VA_ARGS__";
      *rest = skip(tok->next, ")");
      return head.next;
    }

    if (!is_ident(tok))
      error_tok(tok, "expected an identifier");

    if (equal(tok->next, "...")) {
      *va_args_name = strndup(tok->loc, tok->len);
      *rest = skip(tok->next->next, ")");
      return head.next;
    }

    MacroParam *m = calloc(1, sizeof(MacroParam));
    m->name = strndup(tok->loc, tok->len);
    cur = cur->next = m;
    tok = tok->next;
  }

  *rest = tok->next;
  return head.next;
}

static void read_macro_definition(Token **rest, Token *tok) {
  if (!is_ident(tok))
    error_tok(tok, "macro name must be an identifier");
  char *name = strndup(tok->loc, tok->len);
  tok = tok->next;

  if (!tok->has_space && equal(tok, "(")) {
    // Function-like macro
    char *va_args_name = NULL;
    MacroParam *params = read_macro_params(&tok, tok->next, &va_args_name);

    Macro *m = add_macro(name, false, copy_line(rest, tok));
    m->params = params;
    m->va_args_name = va_args_name;
  } else {
    // Object-like macro
    add_macro(name, true, copy_line(rest, tok));
  }
}

static MacroArg *read_macro_arg_one(Token **rest, Token *tok, bool read_rest) {
  Token head = {};
  Token *cur = &head;
  int level = 0;

  for (;;) {
    if (level == 0 && equal(tok, ")"))
      break;
    if (level == 0 && !read_rest && equal(tok, ","))
      break;

    if (tok->kind == TK_EOF)
      error_tok(tok, "premature end of input");

    if (equal(tok, "("))
      level++;
    else if (equal(tok, ")"))
      level--;

    cur = cur->next = copy_token(tok);
    tok = tok->next;
  }

  cur->next = new_eof(tok);

  MacroArg *arg = calloc(1, sizeof(MacroArg));
  arg->tok = head.next;
  *rest = tok;
  return arg;
}

static MacroArg *
read_macro_args(Token **rest, Token *tok, MacroParam *params, char *va_args_name) {
  Token *start = tok;
  tok = tok->next->next;

  MacroArg head = {};
  MacroArg *cur = &head;

  MacroParam *pp = params;
  for (; pp; pp = pp->next) {
    if (cur != &head)
      tok = skip(tok, ",");
    cur = cur->next = read_macro_arg_one(&tok, tok, false);
    cur->name = pp->name;
  }

  if (va_args_name) {
    MacroArg *arg;
    if (equal(tok, ")")) {
      arg = calloc(1, sizeof(MacroArg));
      arg->tok = new_eof(tok);
    } else {
      if (pp != params)
        tok = skip(tok, ",");
      arg = read_macro_arg_one(&tok, tok, true);
    }
    arg->name = va_args_name;;
    arg->is_va_args = true;
    cur = cur->next = arg;
  } else if (pp) {
    error_tok(start, "too many arguments");
  }

  skip(tok, ")");
  *rest = tok;
  return head.next;
}

static MacroArg *find_arg(MacroArg *args, Token *tok) {
  for (MacroArg *ap = args; ap; ap = ap->next)
    if (tok->len == strlen(ap->name) && !strncmp(tok->loc, ap->name, tok->len))
      return ap;
  return NULL;
}

// Concatenates all tokens in `tok` and returns a new string.
static char *join_tokens(Token *tok, Token *end) {
  // Compute the length of the resulting token.
  int len = 1;
  for (Token *t = tok; t != end && t->kind != TK_EOF; t = t->next) {
    if (t != tok && t->has_space)
      len++;
    len += t->len;
  }

  char *buf = calloc(1, len);

  // Copy token texts.
  int pos = 0;
  for (Token *t = tok; t != end && t->kind != TK_EOF; t = t->next) {
    if (t != tok && t->has_space)
      buf[pos++] = ' ';
    strncpy(buf + pos, t->loc, t->len);
    pos += t->len;
  }
  buf[pos] = '\0';
  return buf;
}

// Concatenates all tokens in `arg` and returns a new string token.
// This function is used for the stringizing operator (#).
static Token *stringize(Token *hash, Token *arg) {
  // Create a new string token. We need to set some value to its
  // source location for error reporting function, so we use a macro
  // name token as a template.
  char *s = join_tokens(arg, NULL);
  return new_str_token(s, hash);
}

// Concatenate two tokens to create a new token.
static Token *paste(Token *lhs, Token *rhs) {
  // Paste the two tokens.
  char *buf = format("%.*s%.*s", lhs->len, lhs->loc, rhs->len, rhs->loc);

  // Tokenize the resulting string.
  Token *tok = tokenize(new_file(lhs->file->name, lhs->file->file_no, buf));
  if (tok->next->kind != TK_EOF)
    error_tok(lhs, "pasting forms '%s', an invalid token", buf);
  return tok;
}

static bool has_varargs(MacroArg *args) {
  for (MacroArg *ap = args; ap; ap = ap->next)
    if (!strcmp(ap->name, "__VA_ARGS__"))
      return ap->tok->kind != TK_EOF;
  return false;
}

// Replace func-like macro parameters with given arguments.
static Token *subst(Token *tok, MacroArg *args) {
  Token head = {};
  Token *cur = &head;

  while (tok->kind != TK_EOF) {
    // "#" followed by a parameter is replaced with stringized actuals.
    if (equal(tok, "#")) {
      MacroArg *arg = find_arg(args, tok->next);
      if (!arg)
        error_tok(tok->next, "'#' is not followed by a macro parameter");
      cur = cur->next = stringize(tok, arg->tok);
      tok = tok->next->next;
      continue;
    }

    // [GNU] If __VA_ARG__ is empty, `,##__VA_ARGS__` is expanded
    // to the empty token list. Otherwise, its expaned to `,` and
    // __VA_ARGS__.
    if (equal(tok, ",") && equal(tok->next, "##")) {
      MacroArg *arg = find_arg(args, tok->next->next);
      if (arg && arg->is_va_args) {
        if (arg->tok->kind == TK_EOF) {
          tok = tok->next->next->next;
        } else {
          cur = cur->next = copy_token(tok);
          tok = tok->next->next;
        }
        continue;
      }
    }

    if (equal(tok, "##")) {
      if (cur == &head)
        error_tok(tok, "'##' cannot appear at start of macro expansion");

      if (tok->next->kind == TK_EOF)
        error_tok(tok, "'##' cannot appear at end of macro expansion");

      MacroArg *arg = find_arg(args, tok->next);
      if (arg) {
        if (arg->tok->kind != TK_EOF) {
          *cur = *paste(cur, arg->tok);
          for (Token *t = arg->tok->next; t->kind != TK_EOF; t = t->next)
            cur = cur->next = copy_token(t);
        }
        tok = tok->next->next;
        continue;
      }

      *cur = *paste(cur, tok->next);
      tok = tok->next->next;
      continue;
    }

    MacroArg *arg = find_arg(args, tok);

    if (arg && equal(tok->next, "##")) {
      Token *rhs = tok->next->next;

      if (arg->tok->kind == TK_EOF) {
        MacroArg *arg2 = find_arg(args, rhs);
        if (arg2) {
          for (Token *t = arg2->tok; t->kind != TK_EOF; t = t->next)
            cur = cur->next = copy_token(t);
        } else {
          cur = cur->next = copy_token(rhs);
        }
        tok = rhs->next;
        continue;
      }

      for (Token *t = arg->tok; t->kind != TK_EOF; t = t->next)
        cur = cur->next = copy_token(t);
      tok = tok->next;
      continue;
    }

    // If __VA_ARG__ is empty, __VA_OPT__(x) is expanded to the
    // empty token list. Otherwise, __VA_OPT__(x) is expanded to x.
    if (equal(tok, "__VA_OPT__") && equal(tok->next, "(")) {
      MacroArg *arg = read_macro_arg_one(&tok, tok->next->next, true);
      if (has_varargs(args))
        for (Token *t = arg->tok; t->kind != TK_EOF; t = t->next)
          cur = cur->next = t;
      tok = skip(tok, ")");
      continue;
    }

    // Handle a macro token. Macro arguments are completely macro-expanded
    // before they are substituted into a macro body.
    if (arg) {
      Token *t = preprocess2(arg->tok);
      t->at_bol = tok->at_bol;
      t->has_space = tok->has_space;
      for (; t->kind != TK_EOF; t = t->next)
        cur = cur->next = copy_token(t);
      tok = tok->next;
      continue;
    }

    // Handle a non-macro token.
    cur = cur->next = copy_token(tok);
    tok = tok->next;
    continue;
  }

  cur->next = tok;
  return head.next;
}

// If tok is a macro, expand it and return true.
// Otherwise, do nothing and return false.
static bool expand_macro(Token **rest, Token *tok) {
  if (hideset_contains(tok->hideset, tok->loc, tok->len))
    return false;

  Macro *m = find_macro(tok);
  if (!m)
    return false;

  // Built-in dynamic macro application such as __LINE__
  if (m->handler) {
    *rest = m->handler(tok);
    (*rest)->next = tok->next;
    return true;
  }

  // Object-like macro application
  if (m->is_objlike) {
    Hideset *hs = hideset_union(tok->hideset, new_hideset(m->name));
    Token *body = add_hideset(m->body, hs);
    for (Token *t = body; t->kind != TK_EOF; t = t->next)
      t->origin = tok;
    *rest = append(body, tok->next);
    (*rest)->at_bol = tok->at_bol;
    (*rest)->has_space = tok->has_space;
    return true;
  }

  // If a funclike macro token is not followed by an argument list,
  // treat it as a normal identifier.
  if (!equal(tok->next, "("))
    return false;

  // Function-like macro application
  Token *macro_token = tok;
  MacroArg *args = read_macro_args(&tok, tok, m->params, m->va_args_name);
  Token *rparen = tok;

  // Tokens that consist a func-like macro invocation may have different
  // hidesets, and if that's the case, it's not clear what the hideset
  // for the new tokens should be. We take the interesection of the
  // macro token and the closing parenthesis and use it as a new hideset
  // as explained in the Dave Prossor's algorithm.
  Hideset *hs = hideset_intersection(macro_token->hideset, rparen->hideset);
  hs = hideset_union(hs, new_hideset(m->name));

  Token *body = subst(m->body, args);
  body = add_hideset(body, hs);
  for (Token *t = body; t->kind != TK_EOF; t = t->next)
    t->origin = macro_token;
  *rest = append(body, tok->next);
  (*rest)->at_bol = macro_token->at_bol;
  (*rest)->has_space = macro_token->has_space;
  return true;
}

char *search_include_paths(char *filename) {
  if (filename[0] == '/')
    return filename;

  static HashMap cache;
  char *cached = hashmap_get(&cache, filename);
  if (cached)
    return cached;

  // Search a file from the include paths.
  for (int i = 0; i < include_paths.len; i++) {
    char *path = format("%s/%s", include_paths.data[i], filename);
    if (!file_exists(path))
      continue;
    hashmap_put(&cache, filename, path);
    include_next_idx = i + 1;
    return path;
  }
  return NULL;
}

static char *search_include_next(char *filename) {
  for (; include_next_idx < include_paths.len; include_next_idx++) {
    char *path = format("%s/%s", include_paths.data[include_next_idx], filename);
    if (file_exists(path))
      return path;
  }
  return NULL;
}

// Read an #include argument.
static char *read_include_filename(Token **rest, Token *tok, bool *is_dquote) {
  // Pattern 1: #include "foo.h"
  if (tok->kind == TK_STR) {
    // A double-quoted filename for #include is a special kind of
    // token, and we don't want to interpret any escape sequences in it.
    // For example, "\f" in "C:\foo" is not a formfeed character but
    // just two non-control characters, backslash and f.
    // So we don't want to use token->str.
    *is_dquote = true;
    *rest = skip_line(tok->next);
    return strndup(tok->loc + 1, tok->len - 2);
  }

  // Pattern 2: #include <foo.h>
  if (equal(tok, "<")) {
    // Reconstruct a filename from a sequence of tokens between
    // "<" and ">".
    Token *start = tok;

    // Find closing ">".
    for (; !equal(tok, ">"); tok = tok->next)
      if (tok->at_bol || tok->kind == TK_EOF)
        error_tok(tok, "expected '>'");

    *is_dquote = false;
    *rest = skip_line(tok->next);
    return join_tokens(start->next, tok);
  }

  // Pattern 3: #include FOO
  // In this case FOO must be macro-expanded to either
  // a single string token or a sequence of "<" ... ">".
  if (is_ident(tok)) {
    Token *tok2 = preprocess2(copy_line(rest, tok));
    return read_include_filename(&tok2, tok2, is_dquote);
  }

  error_tok(tok, "expected a filename");
}

// Detect the following "include guard" pattern.
//
//   #ifndef FOO_H
//   #define FOO_H
//   ...
//   #endif
static char *detect_include_guard(Token *tok) {
  // Detect the first two lines.
  if (!is_hash(tok) || !equal(tok->next, "ifndef"))
    return NULL;
  tok = tok->next->next;

  if (!is_ident(tok))
    return NULL;

  char *macro = strndup(tok->loc, tok->len);
  tok = tok->next;

  if (!is_hash(tok) || !equal(tok->next, "define") || !equal(tok->next->next, macro))
    return NULL;

  // Read until the end of the file.
  while (tok->kind != TK_EOF) {
    if (!is_hash(tok)) {
      tok = tok->next;
      continue;
    }

    if (equal(tok->next, "endif") && tok->next->next->kind == TK_EOF)
      return macro;

    if (is_cond_begin(tok->next))
      tok = skip_cond_incl(tok->next->next);
    else
      tok = tok->next;
  }
  return NULL;
}

static char *clean_path(char *path) {
  if (!path)
    return NULL;
#ifdef _WIN32
  char full[4096];
  if (_fullpath(full, path, sizeof(full)))
    path = full;
#endif
  char *p = strdup(path);
  for (char *s = p; *s; s++) {
    if (*s == '\\')
      *s = '/';
#ifdef _WIN32
    *s = tolower((unsigned char)*s);
#endif
  }
  return p;
}

static Token *include_file(Token *tok, char *path, Token *filename_tok) {
  char *cpath = clean_path(path);
  // Check for "#pragma once"
  if (hashmap_get(&pragma_once, cpath))
    return tok;

  // If we read the same file before, and if the file was guarded
  // by the usual #ifndef ... #endif pattern, we may be able to
  // skip the file without opening it.
  static HashMap include_guards;
  char *guard_name = hashmap_get(&include_guards, cpath);
  if (guard_name && hashmap_get(&macros, guard_name))
    return tok;

  Token *tok2 = tokenize_file(path);
  if (!tok2)
    error_tok(filename_tok, "%s: cannot open file: %s", path, strerror(errno));

  guard_name = detect_include_guard(tok2);
  if (guard_name)
    hashmap_put(&include_guards, cpath, guard_name);

  return append(tok2, tok);
}

// Read #line arguments
static void read_line_marker(Token **rest, Token *tok) {
  Token *start = tok;
  tok = preprocess(copy_line(rest, tok));

  if (tok->kind != TK_NUM || tok->ty->kind != TY_INT)
    error_tok(tok, "invalid line marker");
  start->file->line_delta = tok->val - start->line_no;

  tok = tok->next;
  if (tok->kind == TK_EOF)
    return;

  if (tok->kind != TK_STR)
    error_tok(tok, "filename expected");
  start->file->display_name = tok->str;
}

static uint8_t *read_binary_file(char *path, size_t *size_out) {
  if (!path)
    return NULL;
  FILE *fp = fopen(path, "rb");
  if (!fp)
    return NULL;

  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return NULL;
  }
  long sz = ftell(fp);
  if (sz < 0) {
    fclose(fp);
    return NULL;
  }
  if (fseek(fp, 0, SEEK_SET) != 0) {
    fclose(fp);
    return NULL;
  }

  uint8_t *buf = malloc(sz > 0 ? sz : 1);
  if (!buf) {
    fclose(fp);
    return NULL;
  }

  if (sz > 0) {
    size_t read_bytes = fread(buf, 1, sz, fp);
    if (read_bytes != (size_t)sz) {
      free(buf);
      fclose(fp);
      return NULL;
    }
  }

  fclose(fp);
  *size_out = (size_t)sz;
  return buf;
}

static size_t get_binary_file_size(char *path) {
  if (!path)
    return (size_t)-1;
  FILE *fp = fopen(path, "rb");
  if (!fp)
    return (size_t)-1;
  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return (size_t)-1;
  }
  long sz = ftell(fp);
  fclose(fp);
  return sz < 0 ? (size_t)-1 : (size_t)sz;
}

static char *search_embed_file(char *filename, bool is_dquote, char *cur_file_name) {
  if (filename[0] == '/')
    return filename;

  if (is_dquote && cur_file_name) {
    char *path = format("%s/%s", dirname(strdup(cur_file_name)), filename);
    if (file_exists(path))
      return path;
  }

  char *path = search_include_paths(filename);
  if (path && file_exists(path))
    return path;

  if (file_exists(filename))
    return filename;

  return NULL;
}

static Token *read_balanced_tokens_safe(Token **rest, Token *tok, bool *ok) {
  if (!equal(tok, "(")) {
    *ok = false;
    return NULL;
  }
  tok = tok->next;
  Token head = {};
  Token *cur = &head;
  int depth = 1;

  while (tok->kind != TK_EOF) {
    if (equal(tok, "(")) {
      depth++;
    } else if (equal(tok, ")")) {
      depth--;
      if (depth == 0) {
        *rest = tok->next;
        cur->next = new_eof(tok);
        *ok = true;
        return head.next;
      }
    }
    cur = cur->next = copy_token(tok);
    tok = tok->next;
  }
  *ok = false;
  return NULL;
}

static long eval_const_expr_token_list(Token *expr) {
  expr = preprocess_const_expr(expr);

  for (Token *t = expr; t->kind != TK_EOF; t = t->next) {
    if (is_ident(t)) {
      Token *next = t->next;
      *t = *new_num_token(0, t);
      t->next = next;
    }
  }

  convert_pp_tokens(expr);

  Token *rest2 = NULL;
  long val = const_expr(&rest2, expr);
  if (rest2 && rest2->kind != TK_EOF)
    error_tok(rest2, "extra token in constant expression");
  return val;
}

static char *read_embed_filename(Token **rest, Token *tok, bool *is_dquote, bool is_has_embed) {
  if (!tok || tok->kind == TK_EOF) {
    if (!is_has_embed)
      error_tok(tok, "expected a filename");
    return NULL;
  }

  if (tok->kind == TK_STR) {
    *is_dquote = true;
    *rest = tok->next;
    return strndup(tok->loc + 1, tok->len - 2);
  }

  if (equal(tok, "<")) {
    Token *start = tok;
    for (; !equal(tok, ">"); tok = tok->next) {
      if (tok->at_bol || tok->kind == TK_EOF || (is_has_embed && equal(tok, ")"))) {
        if (!is_has_embed)
          error_tok(tok, "expected '>'");
        return NULL;
      }
    }
    *is_dquote = false;
    *rest = tok->next;
    return join_tokens(start->next, tok);
  }

  if (is_ident(tok)) {
    Token *tok2 = preprocess2(copy_line(rest, tok));
    return read_embed_filename(&tok2, tok2, is_dquote, is_has_embed);
  }

  if (!is_has_embed)
    error_tok(tok, "expected a filename");
  return NULL;
}

static bool parse_embed_params(Token **rest, Token *tok, EmbedParams *params, bool is_has_embed) {
  params->is_valid = true;

  while (tok && tok->kind != TK_EOF && (is_has_embed || !tok->at_bol)) {
    if (is_has_embed && equal(tok, ")"))
      break;

    if (equal(tok, "limit") || equal(tok, "__limit__")) {
      if (params->has_limit) {
        if (!is_has_embed)
          error_tok(tok, "duplicate limit parameter");
        params->is_valid = false;
        return false;
      }
      bool ok = false;
      Token *arg = read_balanced_tokens_safe(&tok, tok->next, &ok);
      if (!ok) {
        if (!is_has_embed)
          error_tok(tok, "invalid limit parameter");
        params->is_valid = false;
        return false;
      }
      long val = eval_const_expr_token_list(arg);
      params->has_limit = true;
      params->limit = val < 0 ? 0 : val;
      continue;
    }

    if (equal(tok, "prefix") || equal(tok, "__prefix__")) {
      if (params->has_prefix) {
        if (!is_has_embed)
          error_tok(tok, "duplicate prefix parameter");
        params->is_valid = false;
        return false;
      }
      bool ok = false;
      Token *arg = read_balanced_tokens_safe(&tok, tok->next, &ok);
      if (!ok) {
        if (!is_has_embed)
          error_tok(tok, "invalid prefix parameter");
        params->is_valid = false;
        return false;
      }
      params->has_prefix = true;
      params->prefix = arg;
      continue;
    }

    if (equal(tok, "suffix") || equal(tok, "__suffix__")) {
      if (params->has_suffix) {
        if (!is_has_embed)
          error_tok(tok, "duplicate suffix parameter");
        params->is_valid = false;
        return false;
      }
      bool ok = false;
      Token *arg = read_balanced_tokens_safe(&tok, tok->next, &ok);
      if (!ok) {
        if (!is_has_embed)
          error_tok(tok, "invalid suffix parameter");
        params->is_valid = false;
        return false;
      }
      params->has_suffix = true;
      params->suffix = arg;
      continue;
    }

    if (equal(tok, "if_empty") || equal(tok, "__if_empty__")) {
      if (params->has_if_empty) {
        if (!is_has_embed)
          error_tok(tok, "duplicate if_empty parameter");
        params->is_valid = false;
        return false;
      }
      bool ok = false;
      Token *arg = read_balanced_tokens_safe(&tok, tok->next, &ok);
      if (!ok) {
        if (!is_has_embed)
          error_tok(tok, "invalid if_empty parameter");
        params->is_valid = false;
        return false;
      }
      params->has_if_empty = true;
      params->if_empty = arg;
      continue;
    }

    // Vendor extension parameter: ident :: ident ( ... ) or ident :: ident
    if (is_ident(tok) && equal(tok->next, "::")) {
      tok = tok->next->next;
      if (!is_ident(tok)) {
        if (!is_has_embed)
          error_tok(tok, "expected identifier after '::'");
        params->is_valid = false;
        return false;
      }
      tok = tok->next;
      if (equal(tok, "(")) {
        bool ok = false;
        read_balanced_tokens_safe(&tok, tok, &ok);
        if (!ok) {
          params->is_valid = false;
          return false;
        }
      }
      if (is_has_embed)
        params->is_valid = false;
      continue;
    }

    // Unknown parameter
    if (!is_has_embed)
      error_tok(tok, "unknown embed parameter");
    params->is_valid = false;
    return false;
  }

  if (rest)
    *rest = tok;
  return params->is_valid;
}

static Token *new_num_token_val(int val, Token *tmpl) {
  static char num_strs[256][4];
  static bool inited = false;
  if (!inited) {
    for (int i = 0; i < 256; i++)
      snprintf(num_strs[i], sizeof(num_strs[i]), "%d", i);
    inited = true;
  }

  Token *t = calloc(1, sizeof(Token));
  t->kind = TK_PP_NUM;
  t->loc = (val >= 0 && val <= 255) ? num_strs[val] : format("%d", val);
  t->len = strlen(t->loc);
  t->file = tmpl->file;
  t->filename = tmpl->filename ? tmpl->filename : tmpl->file->display_name;
  t->line_no = tmpl->line_no;
  return t;
}

static Token *new_comma_token(Token *tmpl) {
  Token *t = calloc(1, sizeof(Token));
  t->kind = TK_PUNCT;
  t->loc = ",";
  t->len = 1;
  t->file = tmpl->file;
  t->filename = tmpl->filename ? tmpl->filename : tmpl->file->display_name;
  t->line_no = tmpl->line_no;
  return t;
}

static Token *handle_embed(Token *start, Token *tok) {
  Token *rest_line = NULL;
  Token *line_toks = copy_line(&rest_line, tok);

  // If the first token after embed is an identifier (not string or '<'), macro-expand the line
  if (is_ident(line_toks) && line_toks->kind != TK_STR && !equal(line_toks, "<")) {
    line_toks = preprocess2(line_toks);
  }

  bool is_dquote = false;
  Token *after_filename = NULL;
  char *filename = read_embed_filename(&after_filename, line_toks, &is_dquote, false);
  if (!filename)
    error_tok(tok, "expected a filename");

  // Macro-expand the parameter tokens if any
  Token *param_toks = after_filename;
  if (param_toks && param_toks->kind != TK_EOF) {
    param_toks = preprocess2(param_toks);
  }

  EmbedParams params = {};
  Token *end_params = NULL;
  parse_embed_params(&end_params, param_toks, &params, false);

  char *path = search_embed_file(filename, is_dquote, start->file ? start->file->name : NULL);
  if (!path)
    error_tok(start, "cannot find embed file: %s", filename);

  size_t file_len = 0;
  uint8_t *data = read_binary_file(path, &file_len);
  if (!data)
    error_tok(start, "cannot read embed file: %s", path);

  size_t count = file_len;
  if (params.has_limit && (size_t)params.limit < count)
    count = params.limit;

  Token head = {};
  Token *cur = &head;

  if (count == 0) {
    if (params.if_empty) {
      for (Token *t = params.if_empty; t && t->kind != TK_EOF; t = t->next)
        cur = cur->next = copy_token(t);
    }
  } else {
    if (params.prefix && params.prefix->kind != TK_EOF) {
      for (Token *t = params.prefix; t && t->kind != TK_EOF; t = t->next)
        cur = cur->next = copy_token(t);
      cur = cur->next = new_comma_token(start);
    }

    for (size_t i = 0; i < count; i++) {
      if (i > 0)
        cur = cur->next = new_comma_token(start);
      cur = cur->next = new_num_token_val(data[i], start);
    }

    if (params.suffix && params.suffix->kind != TK_EOF) {
      cur = cur->next = new_comma_token(start);
      for (Token *t = params.suffix; t && t->kind != TK_EOF; t = t->next)
        cur = cur->next = copy_token(t);
    }
  }

  free(data);

  if (head.next) {
    cur->next = rest_line;
    return head.next;
  }
  return rest_line;
}

// Preprocessor directive handler dispatch table
typedef Token *(*DirectiveHandler)(Token *start, Token *tok);

typedef struct {
  char *name;
  DirectiveHandler handler;
} Directive;

static Token *handle_include(Token *start, Token *tok) {
  bool is_dquote;
  char *filename = read_include_filename(&tok, tok->next, &is_dquote);

  if (filename[0] != '/' && is_dquote) {
    char *path = format("%s/%s", dirname(strdup(start->file->name)), filename);
    if (file_exists(path))
      return include_file(tok, path, start->next->next);
  }

  char *path = search_include_paths(filename);
  return include_file(tok, path ? path : filename, start->next->next);
}

static Token *handle_include_next(Token *start, Token *tok) {
  bool ignore;
  char *filename = read_include_filename(&tok, tok->next, &ignore);
  char *path = search_include_next(filename);
  return include_file(tok, path ? path : filename, start->next->next);
}

static Token *handle_embed_directive(Token *start, Token *tok) {
  return handle_embed(start, tok->next);
}

static Token *handle_define(Token *start, Token *tok) {
  read_macro_definition(&tok, tok->next);
  return tok;
}

static Token *handle_undef(Token *start, Token *tok) {
  tok = tok->next;
  if (!is_ident(tok))
    error_tok(tok, "macro name must be an identifier");
  undef_macro(strndup(tok->loc, tok->len));
  return skip_line(tok->next);
}

static Token *handle_if(Token *start, Token *tok) {
  long val = eval_const_expr(&tok, tok);
  push_cond_incl(start, val);
  if (!val)
    tok = skip_cond_incl(tok);
  return tok;
}

static Token *handle_ifdef(Token *start, Token *tok) {
  bool defined = find_macro(tok->next);
  push_cond_incl(tok, defined);
  tok = skip_line(tok->next->next);
  if (!defined)
    tok = skip_cond_incl(tok);
  return tok;
}

static Token *handle_ifndef(Token *start, Token *tok) {
  bool defined = find_macro(tok->next);
  push_cond_incl(tok, !defined);
  tok = skip_line(tok->next->next);
  if (defined)
    tok = skip_cond_incl(tok);
  return tok;
}

static Token *handle_elif(Token *start, Token *tok) {
  if (!cond_incl || cond_incl->ctx == IN_ELSE)
    error_tok(start, "stray #elif");
  cond_incl->ctx = IN_ELIF;

  if (!cond_incl->included && eval_const_expr(&tok, tok))
    cond_incl->included = true;
  else
    tok = skip_cond_incl(tok);
  return tok;
}

static Token *handle_elifdef(Token *start, Token *tok) {
  if (!cond_incl || cond_incl->ctx == IN_ELSE)
    error_tok(start, "stray #elifdef");
  cond_incl->ctx = IN_ELIF;

  bool defined = find_macro(tok->next);
  tok = skip_line(tok->next->next);
  if (!cond_incl->included && defined)
    cond_incl->included = true;
  else
    tok = skip_cond_incl(tok);
  return tok;
}

static Token *handle_elifndef(Token *start, Token *tok) {
  if (!cond_incl || cond_incl->ctx == IN_ELSE)
    error_tok(start, "stray #elifndef");
  cond_incl->ctx = IN_ELIF;

  bool defined = find_macro(tok->next);
  tok = skip_line(tok->next->next);
  if (!cond_incl->included && !defined)
    cond_incl->included = true;
  else
    tok = skip_cond_incl(tok);
  return tok;
}

static Token *handle_else(Token *start, Token *tok) {
  if (!cond_incl || cond_incl->ctx == IN_ELSE)
    error_tok(start, "stray #else");
  cond_incl->ctx = IN_ELSE;
  tok = skip_line(tok->next);

  if (cond_incl->included)
    tok = skip_cond_incl(tok);
  return tok;
}

static Token *handle_endif(Token *start, Token *tok) {
  if (!cond_incl)
    error_tok(start, "stray #endif");
  cond_incl = cond_incl->next;
  return skip_line(tok->next);
}

static Token *handle_line(Token *start, Token *tok) {
  read_line_marker(&tok, tok->next);
  return tok;
}

static Token *handle_pragma(Token *start, Token *tok) {
  if (equal(tok->next, "once")) {
    hashmap_put(&pragma_once, clean_path(tok->file->name), (void *)1);
    return skip_line(tok->next->next);
  }

  do {
    tok = tok->next;
  } while (!tok->at_bol);
  return tok;
}

static Token *handle_error(Token *start, Token *tok) {
  error_tok(tok, "error");
  return tok;
}

static Token *handle_warning(Token *start, Token *tok) {
  warn_tok(tok, "warning directive");
  return skip_to_bol(tok->next);
}

static const Directive directives[] = {
  {"include", handle_include},
  {"include_next", handle_include_next},
  {"embed", handle_embed_directive},
  {"define", handle_define},
  {"undef", handle_undef},
  {"if", handle_if},
  {"ifdef", handle_ifdef},
  {"ifndef", handle_ifndef},
  {"elif", handle_elif},
  {"elifdef", handle_elifdef},
  {"elifndef", handle_elifndef},
  {"else", handle_else},
  {"endif", handle_endif},
  {"line", handle_line},
  {"pragma", handle_pragma},
  {"error", handle_error},
  {"warning", handle_warning},
  {NULL, NULL}
};

static DirectiveHandler find_directive(Token *tok) {
  if (!is_ident(tok))
    return NULL;
  for (const Directive *d = directives; d->name; d++) {
    if (equal(tok, d->name))
      return d->handler;
  }
  return NULL;
}

// Visit all tokens in `tok` while evaluating preprocessing
// macros and directives.
static Token *preprocess2(Token *tok) {
  Token head = {};
  Token *cur = &head;

  while (tok->kind != TK_EOF) {
    // If it is a macro, expand it.
    if (expand_macro(&tok, tok))
      continue;

    // Pass through if it is not a "#".
    if (!is_hash(tok)) {
      tok->line_delta = tok->file->line_delta;
      tok->filename = tok->file->display_name;
      cur = cur->next = tok;
      tok = tok->next;
      continue;
    }

    Token *start = tok;
    tok = tok->next;

    // `#`-only line is legal. It's called a null directive.
    if (tok->at_bol)
      continue;

    if (tok->kind == TK_PP_NUM) {
      read_line_marker(&tok, tok);
      continue;
    }

    DirectiveHandler handler = find_directive(tok);
    if (handler) {
      tok = handler(start, tok);
      continue;
    }

    error_tok(tok, "invalid preprocessor directive");
  }

  cur->next = tok;
  return head.next;
}

void define_macro(char *name, char *buf) {
  Token *tok = tokenize(new_file("<built-in>", 1, buf));
  add_macro(name, true, tok);
}

void undef_macro(char *name) {
  hashmap_delete(&macros, name);
}

static Macro *add_builtin(char *name, macro_handler_fn *fn) {
  Macro *m = add_macro(name, true, NULL);
  m->handler = fn;
  return m;
}

static Token *file_macro(Token *tmpl) {
  while (tmpl->origin)
    tmpl = tmpl->origin;
  return new_str_token(tmpl->file->display_name, tmpl);
}

static Token *line_macro(Token *tmpl) {
  while (tmpl->origin)
    tmpl = tmpl->origin;
  int i = tmpl->line_no + tmpl->file->line_delta;
  return new_num_token(i, tmpl);
}

// __COUNTER__ is expanded to serial values starting from 0.
static Token *counter_macro(Token *tmpl) {
  static int i = 0;
  return new_num_token(i++, tmpl);
}

// __TIMESTAMP__ is expanded to a string describing the last
// modification time of the current file. E.g.
// "Fri Jul 24 01:32:50 2020"
static Token *timestamp_macro(Token *tmpl) {
  struct stat st;
  if (stat(tmpl->file->name, &st) != 0)
    return new_str_token("??? ??? ?? ??:??:?? ????", tmpl);

  char buf[30];
  ctime_r(&st.st_mtime, buf);
  buf[24] = '\0';
  return new_str_token(buf, tmpl);
}

static Token *base_file_macro(Token *tmpl) {
  return new_str_token(base_file, tmpl);
}

// __DATE__ is expanded to the current date, e.g. "May 17 2020".
static char *format_date(struct tm *tm) {
  static char mon[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
  };

  return format("\"%s %2d %d\"", mon[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900);
}

// __TIME__ is expanded to the current time, e.g. "13:34:03".
static char *format_time(struct tm *tm) {
  return format("\"%02d:%02d:%02d\"", tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void init_macros(void) {
  // Define predefined macros
  define_macro("__C99_MACRO_WITH_VA_ARGS", "1");
  define_macro("__SIZEOF_DOUBLE__", "8");
  define_macro("__SIZEOF_FLOAT__", "4");
  define_macro("__SIZEOF_INT__", "4");
  define_macro("__SIZEOF_LONG_DOUBLE__", "8");
  define_macro("__SIZEOF_LONG_LONG__", "8");
  define_macro("__SIZEOF_LONG__", "8");
  define_macro("__SIZEOF_POINTER__", "8");
  define_macro("__SIZEOF_PTRDIFF_T__", "8");
  define_macro("__SIZEOF_SHORT__", "2");
  define_macro("__SIZEOF_SIZE_T__", "8");
  define_macro("__SIZE_TYPE__", "unsigned long");
  define_macro("__PTRDIFF_TYPE__", "long");
  define_macro("__INTPTR_TYPE__", "long");
  define_macro("__UINTPTR_TYPE__", "unsigned long");
  define_macro("__INTMAX_TYPE__", "long long");
  define_macro("__UINTMAX_TYPE__", "unsigned long long");
  define_macro("__WCHAR_TYPE__", "int");
  define_macro("__WINT_TYPE__", "unsigned int");
  define_macro("__STDC_HOSTED__", "1");
  define_macro("__STDC_NO_COMPLEX__", "1");
  define_macro("__STDC_UTF_16__", "1");
  define_macro("__STDC_UTF_32__", "1");
  define_macro("__STDC_VERSION__", "201112L");
  define_macro("__STDC__", "1");
  define_macro("__STDC_EMBED_NOT_FOUND__", "0");
  define_macro("__STDC_EMBED_FOUND__", "1");
  define_macro("__STDC_EMBED_EMPTY__", "2");
  define_macro("__USER_LABEL_PREFIX__", "");
  define_macro("__alignof__", "_Alignof");
  define_macro("__amd64", "1");
  define_macro("__amd64__", "1");
  define_macro("__chibicc__", "1");
  define_macro("__const__", "const");
  define_macro("__inline__", "inline");
  define_macro("__signed__", "signed");
  define_macro("__typeof__", "typeof");
  define_macro("__volatile__", "volatile");
  define_macro("__x86_64", "1");
  define_macro("__x86_64__", "1");

  if (!current_abi || current_abi->size_long == 8) {
    define_macro("_LP64", "1");
    define_macro("__LP64__", "1");
  }

  add_builtin("__FILE__", file_macro);
  add_builtin("__LINE__", line_macro);
  add_builtin("__COUNTER__", counter_macro);
  add_builtin("__TIMESTAMP__", timestamp_macro);
  add_builtin("__BASE_FILE__", base_file_macro);

  time_t now = time(NULL);
  struct tm *tm = localtime(&now);
  define_macro("__DATE__", format_date(tm));
  define_macro("__TIME__", format_time(tm));

  if (current_abi && current_abi->define_macros)
    current_abi->define_macros();
}

typedef enum {
  STR_NONE, STR_UTF8, STR_UTF16, STR_UTF32, STR_WIDE,
} StringKind;

static StringKind getStringKind(Token *tok) {
  if (!strcmp(tok->loc, "u8"))
    return STR_UTF8;

  switch (tok->loc[0]) {
  case '"': return STR_NONE;
  case 'u': return STR_UTF16;
  case 'U': return STR_UTF32;
  case 'L': return STR_WIDE;
  }
  unreachable();
}

// Concatenate adjacent string literals into a single string literal
// as per the C spec.
static void join_adjacent_string_literals(Token *tok) {
  // First pass: If regular string literals are adjacent to wide
  // string literals, regular string literals are converted to a wide
  // type before concatenation. In this pass, we do the conversion.
  for (Token *tok1 = tok; tok1->kind != TK_EOF;) {
    if (tok1->kind != TK_STR || tok1->next->kind != TK_STR) {
      tok1 = tok1->next;
      continue;
    }

    StringKind kind = getStringKind(tok1);
    Type *basety = tok1->ty->base;

    for (Token *t = tok1->next; t->kind == TK_STR; t = t->next) {
      StringKind k = getStringKind(t);
      if (kind == STR_NONE) {
        kind = k;
        basety = t->ty->base;
      } else if (k != STR_NONE && kind != k) {
        error_tok(t, "unsupported non-standard concatenation of string literals");
      }
    }

    if (basety->size > 1)
      for (Token *t = tok1; t->kind == TK_STR; t = t->next)
        if (t->ty->base->size == 1)
          *t = *tokenize_string_literal(t, basety);

    while (tok1->kind == TK_STR)
      tok1 = tok1->next;
  }

  // Second pass: concatenate adjacent string literals.
  for (Token *tok1 = tok; tok1->kind != TK_EOF;) {
    if (tok1->kind != TK_STR || tok1->next->kind != TK_STR) {
      tok1 = tok1->next;
      continue;
    }

    Token *tok2 = tok1->next;
    while (tok2->kind == TK_STR)
      tok2 = tok2->next;

    int len = tok1->ty->array_len;
    for (Token *t = tok1->next; t != tok2; t = t->next)
      len = len + t->ty->array_len - 1;

    char *buf = calloc(tok1->ty->base->size, len);

    int i = 0;
    for (Token *t = tok1; t != tok2; t = t->next) {
      memcpy(buf + i, t->str, t->ty->size);
      i = i + t->ty->size - t->ty->base->size;
    }

    *tok1 = *copy_token(tok1);
    tok1->ty = array_of(tok1->ty->base, len);
    tok1->str = buf;
    tok1->next = tok2;
    tok1 = tok2;
  }
}

// Entry point function of the preprocessor.
Token *preprocess(Token *tok) {
  tok = preprocess2(tok);
  if (cond_incl)
    error_tok(cond_incl->tok, "unterminated conditional directive");
  convert_pp_tokens(tok);
  join_adjacent_string_literals(tok);

  for (Token *t = tok; t; t = t->next)
    t->line_no += t->line_delta;
  return tok;
}
