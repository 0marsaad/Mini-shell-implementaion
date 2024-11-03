/*
 * shell.l: lexical analyzer for shell
 */

%{
#include <string.h>
#include "y.tab.h"
%}

%%

\n      { return NEWLINE; }
[ \t]   { /* Discard spaces and tabs */ }	
">>"    { return GREATGREAT; } // Append redirection
">"     { return GREAT; }      // Output redirection
"<"     { return LESS; }       // Input redirection
"|"     { return PIPE; }       // Pipe
"&"     { return AMPERSAND; }  // Background execution

[^ \t\n><|&]+ {
        /* Matches words and file names */
        yylval.string_val = strdup(yytext);
        return WORD;
}

.       {
        /* Invalid character in input */
        fprintf(stderr, "Invalid token: %s\n", yytext);
        return NOTOKEN;
}

%%
