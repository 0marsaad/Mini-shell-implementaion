/*
 * shell.y: parser for shell with extended grammar
 *
 * Grammar:
 *	cmd [arg]* [ | cmd [arg]* ]* [ [> filename] [< filename] [>> filename] ]* [&]
 */

%union {
	char *string_val;
}

%token <string_val> WORD
%token NOTOKEN GREAT GREATGREAT LESS NEWLINE AMPERSAND PIPE



%{
extern "C" {
	int yylex();
	void yyerror(const char *s);
}
#define yylex yylex
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"
%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: pipeline_command iomodifier_opt background_opt NEWLINE {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE 
	| error NEWLINE { yyerrok; }
	;

pipeline_command:
	simple_command
	| pipeline_command PIPE simple_command {
        printf("   Yacc: insert pipe\n");
		Command::_currentCommand.insertPipe();
	}
	;

simple_command:	
	command_and_args {
		if (Command::_currentSimpleCommand) {
			Command::_currentCommand.insertSimpleCommand(Command::_currentSimpleCommand);
		}
	}
	;

command_and_args:
	command_word arg_list
	;

arg_list:
	arg_list argument
	| 
	;

argument:
	WORD {
		printf("   Yacc: insert argument \"%s\"\n", $1);
		if (Command::_currentSimpleCommand) {
			Command::_currentSimpleCommand->insertArgument(strdup($1)); /		}
	}
	;

command_word:
	WORD {
		printf("   Yacc: insert command \"%s\"\n", $1);
		Command::_currentSimpleCommand = new SimpleCommand();
		Command::_currentSimpleCommand->insertArgument(strdup($1)); 
        }
	;

iomodifier_opt:
	GREAT WORD {
		printf("   Yacc: set output to \"%s\"\n", $2);
		Command::_currentCommand._outFile = strdup($2); 
	}
	| GREATGREAT WORD {
		printf("   Yacc: set append output to \"%s\"\n", $2);
		Command::_currentCommand._append = 1;
		Command::_currentCommand._outFile = strdup($2); 
	}
	| LESS WORD {
		printf("   Yacc: set input to \"%s\"\n", $2);
		Command::_currentCommand._inputFile = strdup($2); 
	}
	| 
	;

background_opt:
	AMPERSAND {
		printf("   Yacc: set background execution\n");
		Command::_currentCommand._background = 1;
	}
	| 
	;

%%

void yyerror(const char * s)
{
	fprintf(stderr, "Syntax error: %s\n", s);
}
