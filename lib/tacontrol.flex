/* tacontrol.flex */

/*
 * Copyright (C) 2014-2019 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */

/*
 * lexical analyzer for parsing ttfautohint control instructions
 *
 * Lexing errors are indicated by setting `context->error' to an appropriate
 * value; fatal lexer errors return token `INTERNAL_FLEX_ERROR'.
 */

/* you should use flex version >= 2.5.39 to avoid various buglets */
/* that don't have work-arounds */

%option outfile="tacontrol-flex.c"
%option header-file="tacontrol-flex.h"

%option batch
%option bison-bridge
%option bison-locations
%option never-interactive
%option noinput
%option nounput
%option noyyalloc
%option noyyfree
%option noyyrealloc
%option noyywrap
%option reentrant
%option yylineno


%top {
#include "config.h"
}


%{

#include <stdio.h>
#include <string.h>

#include "ta.h"
#include "tacontrol-bison.h"

/* option `yylineno' resets `yycolumn' to 0 after a newline */
/* (since version 2.5.30, March 2003) before the next token gets read; */
/* note we want both line number and column start with value 1 */

#define YY_USER_ACTION \
          yylloc->first_line = yylineno + 1; \
          yylloc->last_line = yylineno + 1; \
          yylloc->first_column = yycolumn; \
          yylloc->last_column = yycolumn + (int)yyleng - 1; \
          yycolumn += yyleng;

#define YY_EXTRA_TYPE Control_Context*

#define YY_FATAL_ERROR(msg) TA_control_scanner_fatal_error(msg, yyscanner)

/* by default, `yylex' simply calls `exit' (via YY_FATAL_ERROR) */
/* in case of a (more or less) fatal error -- */
/* this is bad for a library, thus we use `longjmp' to catch this, */
/* together with a proper handler for YY_FATAL_ERROR */

#define YY_USER_INIT \
          do \
          { \
            if (setjmp(yyextra->jump_buffer) != 0) \
            { \
              /* error and error message in `context' already stored by */ \
              /* `TA_control_scanner_fatal_error' */ \
              return INTERNAL_FLEX_ERROR; \
            } \
          } while (0)

/* this macro works around flex bug to suppress a compilation warning, */
/* cf. https://sourceforge.net/p/flex/bugs/115 */

#define YY_EXIT_FAILURE ((void)yyscanner, 2)

#define NAME_ASSIGN \
          do \
          { \
            yylval->name = strdup(yytext); \
            if (!yylval->name) \
              yyextra->error = TA_Err_Control_Allocation_Error; \
          } while (0)

void
TA_control_scanner_fatal_error(const char* msg,
                               yyscan_t yyscanner);
%}


%%


(?x: [ \t\f]+
) {
  /* skip whitespace */
}

(?x: ( "\n"
     | ";"
     )+
) {
  /* end of entry */
  return EOE;
}

<<EOF>> {
  /* we want an EOE token at end of file, too */
  if (!yyextra->eof)
  {
    yyextra->eof = 1;
    return EOE;
  }
  else
    yyterminate();
}

(?x: ( "\\\n"
     )+
) {
  /* skip escaped newline */
}

(?x: "#" [^\n]*
) {
  /* skip line comment */
}


(?x: [0+-]
) {
  /* values `0' and `-' need special treatment for number ranges */
  return yytext[0];
}

(?x:   [1-9] [0-9]*
     | "0" [xX] [0-9a-fA-F]+
     | "0" [0-7]+
) {
  /* we don't support localized formats like a thousands separator */
  errno = 0;
  yylval->integer = strtol(yytext, NULL, 0);
  if (errno == ERANGE)
  {
    /* overflow or underflow */
    yyextra->error = TA_Err_Control_Overflow;
  }
  return INTEGER;
}

(?x:   [0-9]* "." [0-9]+
     | [0-9]+ "." [0-9]*
) {
  /* we don't support exponents, */
  /* and we don't support localized formats like using `,' instead of `.' */
  errno = 0;
  yylval->real = strtod(yytext, NULL);
  if (yylval->real != 0 && errno == ERANGE)
  {
    /* overflow */
    yyextra->error = TA_Err_Control_Overflow;
  }
  return REAL;
}


(?x: [@,()*]
) {
  /* delimiters, wildcard */
  return yytext[0];
}

(?x:   "point"
     | "p"
) {
  NAME_ASSIGN;
  return POINT;
}

(?x:   "touch"
     | "t"
) {
  NAME_ASSIGN;
  return TOUCH;
}

(?x:   "xshift"
     | "x"
) {
  NAME_ASSIGN;
  return XSHIFT;
}

(?x:   "yshift"
     | "y"
) {
  NAME_ASSIGN;
  return YSHIFT;
}

(?x:   "left"
     | "l"
) {
  NAME_ASSIGN;
  return LEFT;
}

(?x:   "right"
     | "r"
) {
  NAME_ASSIGN;
  return RIGHT;
}

(?x:   "nodir"
     | "n"
) {
  NAME_ASSIGN;
  return NODIR;
}

(?x:   "width"
     | "w"
) {
  NAME_ASSIGN;
  return WIDTH;
}

(?x: [A-Za-z._] [A-Za-z0-9._]*
) {
  NAME_ASSIGN;
  return NAME;
}


(?x: .
) {
  /* invalid input */
  yylval->character = yytext[0];
  return INVALID_CHARACTER;
}


%%


/* the following two routines set `context->error' */

void*
yyalloc(yy_size_t size,
        yyscan_t yyscanner)
{
  YY_EXTRA_TYPE context;


  void* p = malloc(size);
  if (!p && yyscanner)
  {
    context = yyget_extra(yyscanner);
    context->error = TA_Err_Control_Allocation_Error;
  }
  return p;
}


void*
yyrealloc(void* ptr,
          yy_size_t size,
          yyscan_t yyscanner)
{
  YY_EXTRA_TYPE context;


  void* p = realloc(ptr, size);
  if (!p && yyscanner)
  {
    context = yyget_extra(yyscanner);
    context->error = TA_Err_Control_Allocation_Error;
  }
  return p;
}


/* we reimplement this routine also to avoid a compiler warning, */
/* cf. https://sourceforge.net/p/flex/bugs/115 */
void
yyfree(void* ptr,
       yyscan_t yyscanner)
{
  (void)yyscanner;
  free(ptr);
}


void
TA_control_scanner_fatal_error(const char* msg,
                               yyscan_t yyscanner)
{
  YY_EXTRA_TYPE context = yyget_extra(yyscanner);


  /* allocation routines set a different error value */
  if (!context->error)
    context->error = TA_Err_Control_Flex_Error;
  strncpy(context->errmsg, msg, sizeof (context->errmsg));

  longjmp(context->jump_buffer, 1);

  /* the next line, which we never reach, suppresses a warning about */
  /* `yy_fatal_error' defined but not used */
  yy_fatal_error(msg, yyscanner);
}


void
TA_control_scanner_init(Control_Context* context,
                        FONT* font)
{
  int flex_error;

  yyscan_t scanner;
  YY_BUFFER_STATE b;


  /* this function sets `errno' in case of error */
  flex_error = yylex_init(&scanner);
  if (flex_error && errno == ENOMEM)
  {
    context->error = FT_Err_Out_Of_Memory;
    context->errmsg[0] = '\0';
    return;
  }

  /* initialize some context fields */
  context->font = font;
  context->error = TA_Err_Ok;
  context->result = NULL;
  context->scanner = scanner;
  context->eof = 0;

  yyset_extra(context, scanner);

  /* by default, `yy_scan_bytes' simply calls `exit' */
  /* (via YY_FATAL_ERROR) in case of a (more or less) fatal error -- */
  /* this is bad for a library, thus we use `longjmp' to catch this, */
  /* together with a proper handler for YY_FATAL_ERROR */
  if (setjmp(context->jump_buffer) != 0)
  {
    /* error and error message in `context' already stored by */
    /* `TA_control_scanner_fatal_error' */
    return;
  }

  b = yy_scan_bytes(font->control_buf, font->control_len, scanner);

  /* flex bug: these two fields are not initialized, */
  /*           causing zillions of valgrind errors; see */
  /*           https://sourceforge.net/p/flex/bugs/180 */
  b->yy_bs_lineno = 0;
  b->yy_bs_column = 0;
}


void
TA_control_scanner_done(Control_Context* context)
{
  yylex_destroy(context->scanner);
}


#if 0

const char* input =
  "# Test\n"
  "\n"
  "# 0 a p 1-3 x 0.3 y -0.2 @ 20-30; \\\n"
  "0 exclam p 2-4 x 0.7 y -0.4 @ 6,7,9,10,11; \\\n"
  "0 a p 2 x 0.5 @ 23-25";


int
main(void)
{
  TA_Error error;
  int retval = 1;

  FONT font;
  Control_Context context;

  YYSTYPE yylval_param;
  YYLTYPE yylloc_param;


  /* we only need the control instructions buffer */
  font.control_buf = (char*)input;
  font.control_len = strlen(input);

  TA_control_scanner_init(&context, &font);
  if (context.error)
    goto Exit;

  yyset_debug(1, context.scanner);
  while (yylex(&yylval_param, &yylloc_param, context.scanner))
  {
    if (context.error)
      goto Exit;
  }

  retval = 0;

Exit:
  TA_control_scanner_done(&context);

  return retval;
}

#endif

/* end of tacontrol.flex */
