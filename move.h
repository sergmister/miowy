#ifndef MOVE_H
#define MOVE_H
#include "board.h"

static const int NumNbrs = 6;              // num nbrs of each cell
extern const int Nbr_offsets[NumNbrs+2] ;  // last = 1st to avoid using %mod
extern const int Bridge_offsets[NumNbrs+2] ;

struct Move {
  int s;
  int lcn;
  Move(int x, int y) : s(x), lcn(y) {}
  Move() {}
} ;

struct ScoreLcn {
  int scr;   // represents a fraction, with MAXSCORE as denominator
  int lcn;
  ScoreLcn(int x, int y) : scr(x), lcn(y) {}
  ScoreLcn() {}
} ;

extern int   fltToScr(float x) ;
extern float scrToFlt(int x) ; 

extern void prtMove(Move m) ;
extern void getCarrier(struct Board& B, Move mv, int C[], int& csize) ;

bool is_win(struct Board& B, Move mv, bool useMiai, int C[], int& csize, bool vrbs) ;
void show_winners(struct Board B, int st, bool useMiai) ;
#endif
