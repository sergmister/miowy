#ifndef GENMOVE_H
#define GENMOVE_H
#include "board.h"

static const int ROLLOUTS = 300000;  // about 30 000 rollouts/sec on base10 board
static const int WDTH = 10;  // negamax search width

int      rand_move     (Board&) ;
int      rand_miai_move(Board&, int s) ;
ScoreLcn flat_MCS (int&, Board&, Playout&, int s, bool uzMi, bool acc, bool v) ;
ScoreLcn ngmx_MCS (int r, Board& Bd, int s, bool rtClr, int d, int w, int alf, int bet, bool v) ;
int      uct_move (int rollouts, Board&, int s, bool useMiai) ;
#endif
