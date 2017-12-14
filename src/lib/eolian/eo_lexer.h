#ifndef __EO_LEXER_H__
#define __EO_LEXER_H__

#include <setjmp.h>

#include <Eina.h>
#include <Eolian.h>

#include "eolian_database.h"

/* a token is an int, custom tokens start at this - single-char tokens are
 * simply represented by their ascii */
#define START_CUSTOM 257

enum Tokens
{
   TOK_EQ = START_CUSTOM, TOK_NQ, TOK_GE, TOK_LE,
   TOK_AND, TOK_OR, TOK_LSH, TOK_RSH,

   TOK_DOC, TOK_STRING, TOK_CHAR, TOK_NUMBER, TOK_VALUE
};

/* all keywords in eolian, they can still be used as names (they're TOK_VALUE)
 * they just fill in the "kw" field of the token */
#define KEYWORDS KW(class), KW(const), KW(enum), KW(return), KW(struct), \
    \
    KW(abstract), KW(constructor), KW(constructors), KW(data), \
    KW(destructor), KW(eo), KW(eo_prefix), KW(event_prefix), KW(events), KW(free), \
    KW(get), KW(implements), KW(import), KW(interface), KW(keys), KW(legacy), \
    KW(legacy_prefix), KW(methods), KW(mixin), KW(params), KW(parts), KW(ptr), \
    KW(set), KW(type), KW(values), KW(var), KWAT(auto), KWAT(beta), \
    KWAT(class), KWAT(const), KWAT(cref), KWAT(empty), KWAT(extern), \
    KWAT(free), KWAT(hot), KWAT(in), KWAT(inout), KWAT(nonull), KWAT(nullable), \
    KWAT(optional), KWAT(out), KWAT(owned), KWAT(private), KWAT(property), \
    KWAT(protected), KWAT(restart), KWAT(pure_virtual), \
    KWAT(warn_unused), \
    \
    KW(byte), KW(ubyte), KW(char), KW(short), KW(ushort), KW(int), KW(uint), \
    KW(long), KW(ulong), KW(llong), KW(ullong), \
    \
    KW(int8), KW(uint8), KW(int16), KW(uint16), KW(int32), KW(uint32), \
    KW(int64), KW(uint64), KW(int128), KW(uint128), \
    \
    KW(size), KW(ssize), KW(intptr), KW(uintptr), KW(ptrdiff), \
    \
    KW(time), \
    \
    KW(float), KW(double), \
    \
    KW(bool), \
    \
    KW(void), \
    \
    KW(accessor), KW(array), KW(iterator), KW(hash), KW(list), KW(inarray), KW(inlist), \
    KW(future),                                   \
    KW(any_value), KW(any_value_ptr), \
    KW(mstring), KW(string), KW(stringshare), KW(strbuf), \
    \
    KW(void_ptr), \
    KW(__builtin_free_cb), \
    KW(function), \
    KW(__undefined_type), \
    \
    KW(true), KW(false), KW(null)

/* "regular" keyword and @ prefixed keyword */
#define KW(x) KW_##x
#define KWAT(x) KW_at_##x

enum Keywords
{
   KW_UNKNOWN = 0,
   KEYWORDS
};

#undef KW
#undef KWAT

enum Numbers
{
   NUM_INT,
   NUM_UINT,
   NUM_LONG,
   NUM_ULONG,
   NUM_LLONG,
   NUM_ULLONG,
   NUM_FLOAT,
   NUM_DOUBLE
};

typedef union
{
   char               c;
   const    char     *s;
   signed   int       i;
   unsigned int       u;
   signed   long      l;
   unsigned long      ul;
   signed   long long ll;
   unsigned long long ull;
   float              f;
   double             d;
   Eolian_Documentation *doc;
} Eo_Token_Union;

/* a token - "token" is the actual token id, "value" is the value of a token
 * if needed - NULL otherwise - for example the value of a TOK_VALUE, "kw"
 * is the keyword id if this is a keyword, it's 0 when not a keyword */
typedef struct _Eo_Token
{
   int token, kw;
   Eo_Token_Union value;
} Eo_Token;

typedef struct _Lexer_Ctx
{
   int line, column;
   const char *linestr;
   Eo_Token token;
} Lexer_Ctx;

typedef struct _Eo_Lexer_Temps
{
   Eolian_Class *kls;
   Eolian_Variable *var;
   Eina_List *str_bufs;
   Eina_List *type_defs;
   Eina_List *type_decls;
   Eina_List *expr_defs;
   Eina_List *strs;
} Eo_Lexer_Temps;

/* keeps all lexer state */
typedef struct _Eo_Lexer
{
   /* current character being tested */
   int          current;
   /* column is token aware column number, for example when lexing a keyword
    * it points to the beginning of it after the lexing is done, icolumn is
    * token unaware, always pointing to current column */
   int          column, icolumn;
   /* the current line number, token aware and unaware */
   int          line_number, iline_number;
   /* t: "normal" - token to lex into, "lookahead" - a lookahead token, used
    * to look one token past "t", when we need to check for a token after the
    * current one and use it in a conditional without consuming the current
    * token - used in pretty few cases - because we have one extra lookahead
    * token, that makes our grammar LL(2) - two tokens in total */
   Eo_Token     t, lookahead;
   /* a string buffer used to keep contents of token currently being read,
    * if needed at all */
   Eina_Strbuf *buff;
   /* a handle pointing to a memory mapped file representing the file we're
    * currently lexing */
   Eina_File   *handle;
   /* the source file name */
   const char  *source;
   /* only basename */
   const char  *filename;
   /* points to the current character in our mmapped file being lexed, just
    * incremented until the end */
   const char  *stream;
   /* end pointer - required to check if we've reached past the file, as
    * mmapped data will give us no EOF */
   const char  *stream_end;
   /* points to the current line being lexed, used by error messages to
    * display the current line with a caret at the respective column */
   const char  *stream_line;
   /* a pointer to the state this lexer belongs to */
   Eolian      *state;
   /* the unit being filled during current parsing */
   Eolian_Unit *unit;
   /* this is jumped to when an error happens */
   jmp_buf      err_jmp;

   /* saved context info */
   Eina_List *saved_ctxs;

   /* represents the temporaries, every object that is allocated by the
    * parser is temporarily put here so the resources can be reclaimed in
    * case of error - and it's nulled when it's written into a more permanent
    * position (e.g. as part of another struct, or into nodes */
   Eo_Lexer_Temps tmp;

   /* whether we allow lexing expression related tokens */
   Eina_Bool expr_mode;

   /* decimal point, by default '.' */
   char decpoint;
} Eo_Lexer;

int         eo_lexer_init           (void);
int         eo_lexer_shutdown       (void);
Eo_Lexer   *eo_lexer_new            (Eolian *state, const char *source);
void        eo_lexer_free           (Eo_Lexer *ls);
/* gets a regular token, singlechar or one of TOK_something */
int         eo_lexer_get            (Eo_Lexer *ls);
/* lookahead token - see Eo_Lexer */
int         eo_lexer_lookahead      (Eo_Lexer *ls);
/* "throws" an error, with a custom message and custom token */
void        eo_lexer_lex_error      (Eo_Lexer *ls, const char *msg, int token);
/* like above, but uses the lexstate->t.token, aka current token */
void        eo_lexer_syntax_error   (Eo_Lexer *ls, const char *msg);
/* turns the token into a string, writes into the given buffer */
void        eo_lexer_token_to_str   (int token, char *buf);
/* returns the string representation of a keyword */
const char *eo_lexer_keyword_str_get(int kw);
/* checks if the given keyword is a builtin type */
Eina_Bool   eo_lexer_is_type_keyword(int kw);
/* gets a keyword id from the keyword string */
int         eo_lexer_keyword_str_to_id(const char *kw);
/* gets the C type name for a builtin type name - e.g. uchar -> unsigned char */
const char *eo_lexer_get_c_type     (int kw);
/* save, restore and clear context (line, column, line string) */
void eo_lexer_context_push   (Eo_Lexer *ls);
void eo_lexer_context_pop    (Eo_Lexer *ls);
void eo_lexer_context_restore(Eo_Lexer *ls);
void eo_lexer_context_clear  (Eo_Lexer *ls);

#endif /* __EO_LEXER_H__ */
