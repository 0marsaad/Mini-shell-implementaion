/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <glob.h>
#include "command.h"
#include <errno.h>

void handle_sigint(int sig)
{
	// Ignore SIGINT
	printf("\n");
	Command::_currentCommand.prompt();
}

void handle_sigchld(int sig)
{
	int saved_errno = errno;
	while (waitpid(-1, NULL, WNOHANG) > 0)
	{
		// Log the termination of the child process
		int log_fd = open("child_termination.log", O_WRONLY | O_CREAT | O_APPEND, 0666);
		if (log_fd != -1)
		{
			const char *log_msg = "Child process terminated\n";
			write(log_fd, log_msg, strlen(log_msg));
			close(log_fd);
		}
	}
	errno = saved_errno;
}
void Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");

	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		printf("  %-3d ", i);
		for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
		{
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
		}
	}

	printf("\n\n");
	printf("  Output       Input        Error        Background\n");
	printf("  ------------ ------------ ------------ ------------\n");
	printf("  %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
		   _inputFile ? _inputFile : "default", _errFile ? _errFile : "default",
		   _background ? "YES" : "NO");
	printf("\n\n");
}

SimpleCommand::SimpleCommand()
{
	// Create available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
	// Check if the argument contains wildcard characters
	if (strchr(argument, '*') || strchr(argument, '?'))
	{
		// Perform wildcard expansion
		glob_t glob_result;
		memset(&glob_result, 0, sizeof(glob_result));

		int return_value = glob(argument, GLOB_TILDE, NULL, &glob_result);
		if (return_value != 0)
		{
			globfree(&glob_result);
			return;
		}

		// Insert each matched filename as an argument
		for (size_t i = 0; i < glob_result.gl_pathc; ++i)
		{
			if (_numberOfArguments == _numberOfAvailableArguments)
			{
				_numberOfAvailableArguments *= 2;
				_arguments = (char **)realloc(_arguments, _numberOfAvailableArguments * sizeof(char *));
			}
			_arguments[_numberOfArguments] = strdup(glob_result.gl_pathv[i]);
			_numberOfArguments++;
		}

		globfree(&glob_result);
	}
	else
	{
		// Insert the argument as is
		if (_numberOfArguments == _numberOfAvailableArguments)
		{
			_numberOfAvailableArguments *= 2;
			_arguments = (char **)realloc(_arguments, _numberOfAvailableArguments * sizeof(char *));
		}
		_arguments[_numberOfArguments] = strdup(argument);
		_numberOfArguments++;
	}
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)malloc(_numberOfSimpleCommands * sizeof(SimpleCommand *));
	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_pipe = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
	if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
	{
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **)realloc(_simpleCommands, _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
	}

	_simpleCommands[_numberOfSimpleCommands] = simpleCommand;
	_numberOfSimpleCommands++;
}

void Command::clear()
{
	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
		{
			free(_simpleCommands[i]->_arguments[j]);
		}

		free(_simpleCommands[i]->_arguments);
		free(_simpleCommands[i]);
	}

	if (_outFile)
	{
		free(_outFile);
	}

	if (_inputFile)
	{
		free(_inputFile);
	}

	if (_errFile)
	{
		free(_errFile);
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
	_pipe = 0;
}

void Command::insertPipe()
{
	_simpleCommands[_numberOfSimpleCommands - 1]->_pipe = 1;
}

void Command::execute()
{
	if (_numberOfSimpleCommands == 0)
	{
		prompt();
		return;
	}

	// Check for the 'exit' command
	if (strcmp(_simpleCommands[0]->_arguments[0], "exit") == 0)
	{
		printf("Exiting shell...\n");
		exit(0);
	}

	// Check for the 'cd' command
	if (strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0)
	{
		if (_simpleCommands[0]->_numberOfArguments == 1)
		{
			// Change to home directory
			const char *home = getenv("HOME");
			if (home == NULL)
			{
				home = "/";
			}
			if (chdir(home) != 0)
			{
				perror("chdir");
			}
		}
		else
		{
			// Change to specified directory
			if (chdir(_simpleCommands[0]->_arguments[1]) != 0)
			{
				perror("chdir");
			}
		}
		clear();
		prompt();
		return;
	}

	print();

	int tmpin = dup(0);
	int tmpout = dup(1);
	int tmperr = dup(2);

	int fdin;
	if (_inputFile)
	{
		fdin = open(_inputFile, O_RDONLY);
	}
	else
	{
		fdin = dup(tmpin);
	}

	int ret;
	int fdout;
	for (int i = 0; i < _numberOfSimpleCommands; i++)
	{
		dup2(fdin, 0);
		close(fdin);

		if (i == _numberOfSimpleCommands - 1)
		{
			if (_outFile)
			{
				if (_append)
				{
					fdout = open(_outFile, O_WRONLY | O_CREAT | O_APPEND, 0666);
				}
				else
				{
					fdout = open(_outFile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
				}
			}
			else
			{
				fdout = dup(tmpout);
			}
		}
		else
		{
			int fdpipe[2];
			pipe(fdpipe);
			fdout = fdpipe[1];
			fdin = fdpipe[0];
		}

		dup2(fdout, 1);
		close(fdout);

		ret = fork();
		if (ret == 0)
		{
			execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
			perror("execvp");
			_exit(1);
		}
	}

	if (!_background)
	{
		waitpid(ret, NULL, 0);
	}

	dup2(tmpin, 0);
	dup2(tmpout, 1);
	dup2(tmperr, 2);

	close(tmpin);
	close(tmpout);
	close(tmperr);

	clear();
	prompt();
}

void Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int yyparse(void);

int main()
{
	// Set up the signal handler to ignore SIGINT
	signal(SIGINT, handle_sigint);

	// Set up the signal handler for SIGCHLD
	struct sigaction sa;
	sa.sa_handler = &handle_sigchld;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	sigaction(SIGCHLD, &sa, NULL);

	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}
