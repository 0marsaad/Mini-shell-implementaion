#ifndef command_h
#define command_h

// Command Data Structure
struct SimpleCommand
{
    int _numberOfAvailableArguments;
    int _numberOfArguments;
    char **_arguments;
    int _pipe;
    SimpleCommand();
    void insertArgument(char *argument);
};

struct Command
{
    int _numberOfAvailableSimpleCommands;
    int _numberOfSimpleCommands;
    SimpleCommand **_simpleCommands;
    char *_outFile;
    char *_inputFile;
    char *_errFile;
    int _background;
    int _append;
    int _pipe;
    void prompt();
    void print();
    void insertPipe();
    void execute();
    void clear();
    Command();
    void insertSimpleCommand(SimpleCommand *simpleCommand);
    static Command _currentCommand;
    static SimpleCommand *_currentSimpleCommand;
};

#endif
