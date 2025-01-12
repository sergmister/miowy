#pragma once
#include "board.hpp"

// static const char HELP_CH = '?';
static const char GENMOVE_CH = '?';
static const char PLAYHEX_CH = 'h';
static const char UNDO_CH = 'u';
static const char QUIT_CH = '\n';
static const char UNUSED_CH = '!';

void prtHelp();
void welcome();
void help();
void interact();
void getCommand(char& cmd, int& s, int& lcn);
