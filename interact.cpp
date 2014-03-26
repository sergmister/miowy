#include <cassert>
#include <cstdio>
#include "connect.h"
#include "genmove.h"
#include "interact.h"
#include "move.h"
#include "ui.h"

void printBye()             { printf("\n ... adios ... \n\n"); }
void printGameAlreadyOver() { printf(" game over\n"); }
void printIllegal()         { printf(" illegal... occupied or off-board"); }
void printHasWon(int s)     { printf(" %c wins  ...\n", emit(s)); }
void printCanWin(int s)     { printf(" %c happy ...\n", emit(s)); }
void printMiaiWin(int ptm, int haswin)    { printf(" %c has miai-win, %c to move  ...", emit(haswin), emit(ptm)); }
void printMoveMade(Move m) {
  printf("play %c %c%1d\n", emit(m.s),alphaCol(m.lcn),numerRow(m.lcn)); }

int movePlus(Board& B, Move mv, bool miai, int&bdst, Move& h) { 
  // move, update history ... bdst computed from scratch
  h = mv;
  return B.move(mv,miai,bdst); }

bool check_hex_dimensions(int x, int y) {
  if ((x<1)||(y<1)) {
    printf("invalid hex board dimensions\n"); return false; }
  if (x+y>N+1) {
    printf("hex board dimensions too large\n"); return false; }
  printf("play %dx%d hex\n",x,y); 
  return true; }

void playHex(Board&B, Move h[], int& m, int x, int y) {
  if (check_hex_dimensions(x,y)) { int j,k,psn,dummy;
    for (j=0; j<N; j++)
      for (k=0; k<N-j; k++) { 
        psn = fatten(j,k);
        if (j>=x) 
          movePlus(B,Move(BLK,psn),false,dummy,h[m++]);
        else if (k>=y) 
            movePlus(B,Move(WHT,psn),false,dummy,h[m++]);
      }
  }
  B.show(); }

int updateConnViaHistory(Board& B, int st, bool useMiai, Move h[], int mvs) { int bdst;
// needed eg if prev opt mv hits miai 
  B.zero_connectivity(st);
  for (int j=0; j<mvs; j++)
    if (st == h[j].s)
      B.move(h[j], useMiai, bdst);
  return bdst; }

void moveAndUpdate(Board& B, Move mv, Move h[], int& m, bool useMiai, int& bdst, bool& w) { 
  movePlus(B, mv, useMiai, bdst, h[m++]);
  updateConnViaHistory(B, opt(mv.s), useMiai, h, m);
  w = has_win(bdst);
  if (w) { // 
    if (useMiai) { // check for absolute win
      Board local = B;
      bdst = updateConnViaHistory(local, mv.s, false, h, m);
      w = has_win(bdst);
    }
    if (w) // absolute win
      printHasWon(mv.s); 
    else        
      printCanWin(mv.s); 
  }
}

void undoMove(Board& B, Move h[], int& moves, bool useMiai, bool& w) { 
  if (moves>0) { // start from scratch, replay all but last move
    w = false; // no moves after winner, so no winner after undo
    B.init(); int pX = h[0].s;  
    updateConnViaHistory(B, pX,        useMiai, h, moves-1); // place pX stones (conn valid ?)
    updateConnViaHistory(B, opt(pX), useMiai, h, moves-1); // place pY stones, update pY conn
    updateConnViaHistory(B, pX,        useMiai, h, moves-1); // (place pX stones), update pX conn
    moves--;
  }
}

void prtHist(Move h[], int n) {
  printf("moves:");
  for (int j=0; j<n; j++) { 
    printf(" %d.",j+1); 
    prtMove(h[j]); } 
  printf("\n");
}

ScoreLcn easyMove(Board& B, int st, Move h[], int mvs, bool v) { // already have winning vc, so free move, so... ?
  int bstM[TotalCells]; int k = 3;
  printMiaiWin(st, st);                                      // ... so good quiet move is opt's best move
  updateConnViaHistory(B, st, false, h, mvs);                // update conn'y with no miai
  ScoreLcn osl = flat_MCS(ROLLOUTS, B, opt(st), 
    false, false, v, k, bstM); // best **opponent** move (no miai, no accel)
  return ScoreLcn(1.0-osl.scr, osl.lcn);
}

ScoreLcn futileMove(Board& B, int st, Move h[], int mvs, bool v) { // miai loss before moving
  int bstM[TotalCells]; int k = 3;
  printMiaiWin(st,opt(st)); 
  updateConnViaHistory(B, st, false, h, mvs);         // update conn'y with no miai
  updateConnViaHistory(B, opt(st), false, h, mvs);         // update conn'y with no miai
  ScoreLcn sl = flat_MCS(ROLLOUTS, B, opt(st), 
    false, true, v, k, bstM);  // best opt move (no miai, yes accel)
  //return ScoreLcn(0.0, rand_miai_move(B,opt(st))); 
  return sl;
}

void interact(Board& B) { bool useMiai = true; bool accelerate = true;
  assert(B.num(EMP)==TotalCells); displayHelp(); 
  Move h[TotalCells]; // history
  int bestMoves[TotalCells]; int kmvs = 5; // for flat_MCS()
  bool quit = false; bool abswin = false;
  int st, lcn, bdst; bool vrbs = true;
  char cmd = UNUSED_CH;  // initialize to unused character
  int moves = 0;   // when parameter (eg miai-reply) not used
  ScoreLcn sl;
  B.showAll();
  maxn(32);
  while(!quit) {
    prtHist(h,moves);
    getCommand(cmd,st,lcn);
    switch (cmd) {
      case QUIT_CH:     
        quit = true; break; 
      case PLAYHEX_CH:  
        playHex(B, h, moves, st, lcn); break;  // st,lcn used for x,y
      case UNDO_CH:     
        undoMove(B, h, moves, useMiai, abswin);  B.showAll(); break;
      case GENMOVE_CH:
          if (abswin)  { printGameAlreadyOver(); break; }
          if (has_win(updateConnViaHistory(B, st, useMiai, h, moves))) { 
            sl = easyMove(B, st, h, moves, vrbs); lcn = sl.lcn; }
          else if (has_win(updateConnViaHistory(B, opt(st), true, h, moves))) { 
            sl = futileMove(B, st, h, moves, vrbs); lcn = sl.lcn; }
          else {
            sl = flat_MCS(ROLLOUTS, B, st, useMiai, accelerate, vrbs, kmvs, bestMoves);
            if (kmvs == 0)
              sl = flat_MCS(ROLLOUTS, B, st, false, accelerate, vrbs, kmvs, bestMoves);
            lcn = sl.lcn;
          }
          //lcn = uct_move   (ROLLOUTS, B, st, useMiai);  // needs debugging
          //lcn = rand_move  (B);
          printf("score %.2f\n",scrToFlt(sl.scr)); printMoveMade( Move(st,lcn) );  // continue ...
      default: // specified move
        if (abswin) { printGameAlreadyOver(); break; }
        if ((EMP != st)&&(EMP != B.board[lcn]))  { printIllegal(); break; }
        moveAndUpdate(B, Move(st,lcn), h, moves, useMiai, bdst, abswin);
        B.showAll();
    }
  }
  printBye();
}
