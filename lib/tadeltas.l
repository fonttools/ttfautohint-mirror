/* tadeltas.l */

/*
 * Copyright (C) 2014 by Werner Lemberg.
 *
 * This file is part of the ttfautohint library, and may only be used,
 * modified, and distributed under the terms given in `COPYING'.  By
 * continuing to use, modify, or distribute this file you indicate that you
 * have read `COPYING' and understand and accept it fully.
 *
 * The file `COPYING' mentioned in the previous paragraph is distributed
 * with the ttfautohint library.
 */

/* lexical analyzer for parsing delta exceptions */

%option outfile="tadeltas-flex.c"
%option header-file="tadeltas-flex.h"

%option batch
%option bison-bridge
%option bison-locations
%option never-interactive
%option noinput
%option nounput
%option noyywrap
%option reentrant
%option yylineno

%{

#include <stdio.h>
#include <string.h>

#include "tadeltas-bison.h"

/* option `yylineno' resets `yycolumn' to 0 after a newline */
/* (since version 2.5.30, March 2003) before the next token gets read; */
/* note we want both line number and column start with value 1 */

#define YY_USER_ACTION \
  yylloc->first_line = yylineno + 1; \
  yylloc->last_line = yylineno + 1; \
  yylloc->first_column = yycolumn; \
  yylloc->last_column = yycolumn + yyleng - 1; \
  yycolumn += yyleng;

#define YY_EXTRA_TYPE Deltas_Context*

%}


%%


(?x: [ \t\f]+
) {
  /* skip whitespace */
}

(?x: ( "\r\n"
     | "\n"
     | "\r"
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

(?x: ( "\\\r\n"
     | "\\\n"
     | "\\\r"
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
    yyextra->error = TA_Err_Deltas_Overflow;
    return BAD_INTEGER;
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
  if (yylval->real && errno == ERANGE)
  {
    /* overflow */
    yyextra->error = TA_Err_Deltas_Overflow;
    return BAD_REAL;
  }
  return REAL;
}


(?x: [pxy@,]
) {
  /* delimiters */
  return yytext[0];
}

(?x: [A-Za-z._] [A-Za-z0-9._]*
) {
  yylval->name = strdup(yytext);
  if (!yylval->name)
  {
    yyextra->error = TA_Err_Deltas_Allocation_Error;
    return BAD_NAME;
  }
  return NAME;
}


(?x: .
) {
  /* invalid input */
  yyextra->error = TA_Err_Deltas_Syntax_Error;
  return INVALID_CHARACTER;
}


%%


TA_Error
TA_deltas_scanner_init(Deltas_Context* context,
                       const char* input)
{
  int flex_error;

  yyscan_t scanner;
  YY_BUFFER_STATE b;


  /* this function sets `errno' in case of error */
  flex_error = yylex_init(&scanner);
  if (flex_error && errno == ENOMEM)
    return FT_Err_Out_Of_Memory;

  yyset_extra(context, scanner);

  /* XXX unfortunately, this flex function simply calls `exit' */
  /* (via YY_FATAL_ERROR) in case of a (more or less) fatal error -- */
  /* a possible error handling solution is using `longjmp', cf. */
  /* https://github.com/plusvic/yara/issues/45 */
  b = yy_scan_string(input, scanner);

  /* flex bug: those two fields are not initialized, */
  /*           causing zillions of valgrind errors; see */
  /*           https://sourceforge.net/p/flex/bugs/180 */
  b->yy_bs_lineno = 0;
  b->yy_bs_column = 0;

  /* initialize some context fields */
  context->error = TA_Err_Ok;

  context->result = NULL;

  context->scanner = scanner;
  context->eof = 0;

  return TA_Err_Ok;
}


void
TA_deltas_scanner_done(Deltas_Context* context)
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
  Deltas_Context context;

  YYSTYPE yylval_param;
  YYLTYPE yylloc_param;


  error = TA_deltas_scanner_init(&context, input);
  if (error)
    return 1;

  yyset_debug(1, context.scanner);
  while (yylex(&yylval_param, &yylloc_param, context.scanner))
    ;

  TA_deltas_scanner_done(&context);

  return 0;
}

#endif

/* end of tadeltas.l */
