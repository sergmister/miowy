#include <stdio.h>
#include <cassert>
#include "board.h"
#include "connect.h"
#include "move.h"
#include "shuff.h"

int numSetBits(int n) { 
    int count = 0;
    while (n) { n &= (n-1) ; count++; }
    return count;
}

int   fltToScr(float x) {return int(x*MAXSCORE); }
float scrToFlt(int x)   {return float(x)/float(MAXSCORE); } 

void prtMove(Move m) {
  printf("%c",emit(m.s)); printf("["); prtLcn(m.lcn); printf("]"); }

const int Nbr_offsets[NumNbrs+2] // wraparound, avoid mod, can always access [x+2]
  = {-Np2G, 1-Np2G, 1, Np2G, Np2G-1, -1,-Np2G, 1-Np2G};
const int Bridge_offsets[NumNbrs+2] // wraparound, avoid mod, can always access [x+2]
  = {-2*Np2G+1, 2-Np2G,   Np2G+1,
      2*Np2G-1, Np2G-2, -(Np2G+1),
     -2*Np2G+1, 2-Np2G };

void show_winners(struct Board B, int st, bool useMiai) { 
  int MP[TotalCells]; int MPsize;
  printf("  %c next-move winners\n",emit(st));
  for (int r=0; r<N; r++) {
    for (int k=0; k<r; k++) printf("  ");
    for (int c=0; c<N-r; c++) {
      int lcn = fatten(r,c);
      Move mv(st,lcn);
      if ((B.board[lcn]==EMP)&&(is_win(B,mv,useMiai,MP,MPsize,false))) 
        printf(" *  ");
      else
        printf(" .  ");
    }
    printf("\n");
  }
  printf("\n");
}

void getCarrier(struct Board& B, Move mv, int C[], int& csize) {
  C[0] = mv.lcn; csize = 1;
  for (int q = 0; q < NumNbrs; q++) {
    int xLcn = mv.lcn+Nbr_offsets[q];
    Move mx(mv.s,xLcn);
    if ((B.board[xLcn]==EMP)&&(!B.not_in_miai(mx)))
     C[csize++] = xLcn;
  }
}

bool is_win(struct Board& B, Move mv, bool useMiai, int C[], int& csize, bool vrbs) { 
// if winmove, carrier is cell plus any nbring self-miai
  assert(B.board[mv.lcn]==EMP);
  int bd_set = BRDR_NIL;
  Board local = B;
  local.move(mv, useMiai, bd_set);
  if (useMiai && has_win(bd_set)) { // leave carrier of winmove in C[]
    getCarrier(local, mv, C, csize);
    if (vrbs) {prtLcn(C[0]); printf(" wins carrier size %d\n",csize); }
  }
  return has_win(bd_set);
}

bool Board::not_in_miai(Move mv) { return reply[nx(mv.s)][mv.lcn]==mv.lcn ; }

void Board::set_miai(int s, int x, int y) { 
  reply[nx(s)][x] = y; 
  reply[nx(s)][y] = x; }

void Board::release_miai(Move mv) { // WARNING: does not alter miai connectivity
  int y = reply[nx(mv.s)][mv.lcn]; 
  reply[nx(mv.s)][mv.lcn] = mv.lcn; 
  reply[nx(mv.s)][y] = y; }

void Board::put_stone(Move mv) { 
  assert((board[mv.lcn]==EMP)||(board[mv.lcn]==mv.s)||board[mv.lcn]==TMP); 
  board[mv.lcn] = mv.s; }

void Board::YborderRealign(Move mv, int& cpt, int c1, int c2, int c3) {
  // no need to undo connectivity :)
  // realign Y border miai as shown  ... x = mv.lcn
  //   * * * *       * * * *
  //    2 1 y         . 1 y
  //     x             x 3
  assert(near_edge(c1) && near_edge(c2)); //printf("Y border realign %c",emit(s));
  release_miai(Move(mv.s,c1));
  set_miai(mv.s,c1,c3); assert(not_in_miai(Move(mv.s,c2)));
  //printf(" new miai"); prtLcn(c1); prtLcn(c3); show();
  int xRoot = Find(p,mv.lcn);
  brdr[xRoot] |= brdr[cpt];
  cpt = Union(p,cpt,xRoot);
}

int Board::moveMiaiPart(Move mv, bool useMiai, int& bdset, int cpt) {
// useMiai true... continue with move( )...
  int nbr,nbrRoot; int lcn = mv.lcn; int s = mv.s;
  release_miai(mv); // your own miai, so connectivity ok
  int miReply = reply[nx(opt(s))][lcn];
  if (miReply != lcn) { // playing into opponent miai... which will be released
    // WARNING: if opp'ts next move is not to the miai response, conn'ty needs to be recomputed
    //prtLcn(lcn); printf(" released opponent miai\n"); 
    release_miai(Move(opt(s),lcn));
  }
  // avoid directional bridge bias: search in random order
  int x, perm[NumNbrs] = {0, 1, 2, 3, 4, 5}; 
  shuffle_interval(perm,0,NumNbrs-1); 
  for (int t=0; t<NumNbrs; t++) {  // look for miai nbrs
    // in this order 1) connecting to a stone  2) connecting to a side
    x = perm[t]; assert((x>=0)&&(x<NumNbrs));
    nbr = lcn + Bridge_offsets[x]; // nbr via bridge
    int c1 = lcn+Nbr_offsets[x];     // miai carrier 
    int c2 = lcn+Nbr_offsets[x+1];   // other miai carrier
    Move mv1(s,c1); 
    Move mv2(s,c2); 
         // y border realign: move the miai so stones are connected *and* adjacent to border
         //   * -         becomes  * m
         //  m m *                - m *
         // g g g g              g g g g     <-  border guards 
    // rearrange this: 4 cases: both before/after miais possible, only after, only before, none of above
    if (board[nbr] == s &&
        board[c1] == EMP &&
        board[c2] == EMP &&
        (not_in_miai(mv1)||not_in_miai(mv2))) {
          if (!not_in_miai(mv1)) {
            if (near_edge(c1) && (near_edge(lcn) || near_edge(nbr)))
              YborderRealign(Move(s,nbr),cpt,c1,reply[nx(s)][c1],c2);
          }
          else if (!not_in_miai(mv2)) {
            if (near_edge(c2) && (near_edge(lcn) || near_edge(nbr)))
              YborderRealign(Move(s,nbr),cpt,c2,reply[nx(s)][c2],c1);
         }
         else if (Find(p,nbr)!=Find(p,cpt)) {  // new miai candidate
           //nbrRoot = Find(p,nbr); //int b  = brdr[nbrRoot]  | brdr[cpt];
           //int nbr0 = lcn+Bridge_offsets[x-1]; int c0 = lcn + Nbr_offsets[x-1];
           //if ((board[nbr0]==s) && (board[c0]==EMP) && (Find(p,nbr0)!=Find(p,cpt))) {
             //// nbr0 also new miai candidate, which is better?
             //int nbrRoot0 = Find(p,nbr0);
             //int b0 = brdr[nbrRoot0] | brdr[cpt];
             //if (numSetBits(b0) > numSetBits(b)) { c2 = c0; nbrRoot = nbrRoot0; }
           //} 
           //int nbr2 = lcn+Bridge_offsets[x+1]; int c3 = lcn + Nbr_offsets[x+2];
           //if ((board[nbr2]==s) && (board[c3]==EMP) && (Find(p,nbr2)!=Find(p,cpt))) {
             //int nbrRoot2 = Find(p,nbr2);
             //int b2 = brdr[nbrRoot2] | brdr[cpt];
             //int b  = brdr[nbrRoot]  | brdr[cpt];
             //if (numSetBits(b2) > numSetBits(b)) { c1 = c3; nbrRoot = nbrRoot2; }
             nbrRoot = Find(p,nbr); //int b  = brdr[nbrRoot]  | brdr[cpt];
             brdr[nbrRoot] |= brdr[cpt];
             cpt = Union(p,cpt,nbrRoot); 
             set_miai(s,c1,c2);
             } 
         }
    else if ((board[nbr] == GRD) &&
             (board[c1]  == EMP) &&
             (board[c2]  == EMP) &&
             (not_in_miai(mv1))&&
             (not_in_miai(mv2))) { // new miai
      brdr[cpt] |= brdr[nbr];
      set_miai(s,c1,c2);
      }
  }
  bdset = brdr[cpt];
  return miReply;
}

int Board::move(Move mv, bool useMiai, int& bdset) { //bdset comp. from scratch
// put mv.s on board, update connectivity for mv.s
// WARNING  opt(mv.s) connectivity will be broken if mv.s hits opt miai
//   useMiai ? miai adjacency : stone adjacency
// return opponent miai reply of mv, will be mv.lcn if no miai
// WARNING: there are 6 possible miai choices, each consecutive pair conflicts...
//          right now, selection is made randomly... so might not always find
//          winning choice .... might be better to
//          pick a maximal set that maximizes new brdr connectivity
  int nbr,nbrRoot,cpt; int lcn = mv.lcn; int s = mv.s;
  assert(brdr[lcn]==BRDR_NIL);
  put_stone(mv);
  cpt = lcn; // cpt of s group containing lcn
  for (int t=0; t<NumNbrs; t++) {  // look for absolute nbrs
    nbr = lcn + Nbr_offsets[t];
    if (board[nbr] == s) {
      nbrRoot = Find(p,nbr);
      brdr[nbrRoot] |= brdr[cpt];
      cpt = Union(p,cpt,nbrRoot); } 
    else if (board[nbr] == GRD) {
      brdr[cpt] |= brdr[nbr]; }
  }
  if (!useMiai) {
    bdset = brdr[cpt];
    return lcn;       // no miai, so return lcn
  } // else 
  return moveMiaiPart(mv, useMiai, bdset,cpt);
}
