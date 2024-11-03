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
">>"    { return GREATGREAT; } 
">"     { return GREAT; }      
"<"     { return LESS; }       
"|"     { return PIPE; }       
"&"     { return AMPERSAND; }  

[^ \t\n><|&]+ {
        yylval.string_val = strdup(yytext);
        return WORD;
}

.       {
        fprintf(stderr, "Invalid token: %s\n", yytext);
        return NOTOKEN;
}

%%
