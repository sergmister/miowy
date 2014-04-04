#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "board.h"
#include "move.h"
#include "shuff.h"
#include "vec.h"

void prtBlanks(int k) {
  for (int j=0; j<k; j++) printf(" ");
}

void prtLcn(int psn) {
  printf("%c%1d",alphaCol(psn),numerRow(psn)); }

char emit(int value) {
  switch(value) {
    case EMP: return EMP_CH;
    case BLK: return BLK_CH;
    case WHT: return WHT_CH;
    default: return '?'; } }

void emitString(int value) {
  switch(value) {
    case EMP: printf("empty"); break;
    case BLK: printf("black"); break;
    case WHT: printf("white"); break;
    default: printf(" ? "); } }

int b2(int shape, int j) { // inner loop bound in shapeAs
  switch(shape) {
    case RHOMBUS: return Np2G;  // will display as Np2G*Np2G rhombus
    case TRI:     return N-j;   // will display as N*N*N triangle
    default:      assert(0==1); return 0; } }

void shapeAs(int shape, int X[]) { // display as given shape
  int psn = 0; int b = (shape==RHOMBUS)? Np2G : N;
  for (int j = 0; j < b; j++) {
    for (int k = 0; k < j; k++) printf(" ");
    for (int k = 0; k < b2(shape,j); k++) 
      printf(" %3d", X[psn++]); 
    printf("\n"); }
  printf("\n"); }

void prtNonZero(int x, int fw) { // specified field width
  if (x!=0) printf("%*d ",fw,x); 
  else {prtBlanks(fw/2); printf("*"); prtBlanks((fw+1)/2);}
}

int numDigits(int n) {
  int digits = 0; do { n /= 10; digits++; } while (n != 0);
return digits;
}

void showYcore(int X[]) { int j,k,psn,x;
  for (j = 0; j < N; j++) {
    psn = fatten(j,0); 
    prtBlanks(j);
    for (k = 0; k < N-j; k++) {
      x = X[psn++]; prtNonZero(x,3); }
    printf("\n"); }
  printf("\n"); }

void showBothYcore(int X[], int Y[]) {int j,k,psn,x;
  int mx = numDigits(myMax(X,TotalGBCells));
  int my = numDigits(myMax(X,TotalGBCells));
  //printf("max values %d %d\n",mx,my);
  for (j = 0; j < N; j++) {
    psn = fatten(j,0); 
    prtBlanks(j*mx/2);
    for (k = 0; k < N-j; k++) {
      x = X[psn++]; prtNonZero(x,mx+1); }
    psn = fatten(j,0); 
    prtBlanks(j*(2+3*mx/2));
    for (k = 0; k < N-j; k++) {
      x = Y[psn++]; prtNonZero(x,my+1); }
    printf("\n"); }
  printf("\n"); }

void display_nearedges() { int T[TotalGBCells];
 for (int j=0; j<TotalGBCells; j++)
    T[j]=0;
  for (int j=0; j<TotalGBCells; j++)
    if ((board_row(j)>=0)&&(board_col(j)>=0)&&
        (board_row(j)<N)&&(board_col(j)<N)&&
        (board_row(j)+board_col(j)<N)&&near_edge(j))
      T[j]=9;
  shapeAs(RHOMBUS,T); }

///////////// Playout::

void Playout::single_playout(int& trn, int& k, bool useMiai) { 
  Board L(B);
  k = -1; // k+1 is num stones placed before winner found
  int bd_set = BRDR_NIL; 
  int turn = trn; // local copy
  do { 
    k++;
    assert(L.board[Avail[k]]==EMP);
    int miReply = -1;
    Move mv(turn, Avail[k]);
    miReply = L.move(mv, useMiai, bd_set);
    if (useMiai) {
      // played into oppt miai ?
      int resp = L.reply[nx(opt(turn))][Avail[k]];
      if (resp!=Avail[k]) {int z; //prep for autorespond on next move
        for (z=k+1; Avail[z]!=resp; z++) ;
        //if (z>=TotalCells) {
         //printf("miai problem   resp %d k %d turn %d\n",resp,k,turn);
         //B.show(); L.show(); shapeAs(RHOMBUS,Avail);
         //L.showMi(opt(turn)); B.showMi(opt(turn));
         //L.showMi(turn); B.showMi(turn);
       // }
        assert(z<TotalCells); 
        assert((k+1)<TotalCells);
        swap(Avail[k+1],Avail[z]);
      }
    }
    turn = opt(turn);
  } while (!has_win(bd_set)) ; 
  if (k==0) {prtLcn(Avail[0]); printf(" wins on 1st move of playout\n"); L.showAll();}
  trn = opt(turn);
}

Playout::Playout(Board& B):B(B) { int psn; int j=0;
// put locations of available moves in Avail, return how many
  for (int r=0; r<N; r++)
    for (int c=0; c<N-r; c++) {
      psn = fatten(r,c);
      if (EMP==B.board[psn])
        Avail[j++]=psn;
      }
  assert(j!=0); // must be available move
  numAvail = j;
  mpsz = numAvail;  // only initialize cells when mpsz is less
  memset(wins,0,sizeof(wins));
  memset(cellWins,0,sizeof(cellWins));
  memset(AMAF,0,sizeof(AMAF));
  memset(win_length,0,sizeof(win_length)); 
  for (int t=0; t<=1; t++) minwinlen[t] = INFNTY;
}

void Playout::listLive(int k) {
  printf("live cells ");
  for (int j=0; j<= k; j++) {prtLcn(Avail[j]); printf(" ");}
  printf("\n");
}

//////////////////// Board::

int Board::num(int kind) { int count = 0;
  for (int j=0; j<N; j++)
    for (int k=0; k<N; k++)
        if (board[fatten(j,k)]==kind) 
          count ++;
  return count; }

void prMiai(int p, int r) {
  //if (p!=r) printf(" %3d", p);
  if (p!=r) { printf("  "); prtLcn(p); }
  else      printf("   *");
}
  
void Board::showMi(int s) { emitString(s), printf(" miai\n");
  for (int j = 0; j < N; j++) {
    int psn = (j+GUARDS)*Np2G + GUARDS; //printf("psn %2d ",psn);
    for (int k = 0; k < j; k++) 
      printf(" ");
    for (int k = 0; k < N-j; k++) {
      prMiai(reply[nx(s)][psn],psn); 
      psn++; 
    } 
    printf("\n");
  }
  printf("\n"); }

void Board::showBothMi() { int psn; 
  //printf("            miai  (black,white)\n");
  for (int r = 0; r < N; r++) {
    for (int k = 0; k < r; k++) printf("  ");
    for (int c = 0; c < N-r; c++) { psn = fatten(r,c);
      prMiai(reply[nx(BLK)][psn],psn);
    }
    for (int k = 0; k < 2*r; k++) printf("  ");
    for (int c = 0; c < N-r; c++) { psn = fatten(r,c);
      prMiai(reply[nx(WHT)][psn],psn);
    }
    printf("\n");
  }
  printf("\n"); }

void Board::showMiaiPar() { int psn; 
  for (int r = 0; r < N; r++) {
    for (int k = 0; k < r; k++) printf("  ");
    for (int c = 0; c < N-r; c++) { psn = fatten(r,c);
      prMiai(reply[nx(BLK)][psn],psn);
    }
    for (int k = 0; k < 2*r; k++) printf("  ");
    for (int c = 0; c < N-r; c++) { psn = fatten(r,c);
      prMiai(reply[nx(WHT)][psn],psn);
    }
    for (int k = 0; k < 2*r; k++) printf("  ");
    for (int c = 0; c < N-r; c++) { psn = fatten(r,c);
      if (board[psn]!= EMP) {printf("  "); prtLcn(p[psn]);} else printf("  * ");
    }
    printf("\n");
  }
  printf("\n"); }

void Board::showP() { //printf("UF parents (for non-captains only)\n");
  for (int j = 0; j < N; j++) {
    int psn = (j+GUARDS)*Np2G + GUARDS; //printf("psn %2d ",psn);
    for (int k = 0; k < j; k++) 
      printf(" ");
    for (int k = 0; k < N-j; k++) {
      int x = p[psn++];
      if (x!=psn-1) { printf("  "); prtLcn(x); }
      else
        printf("  * ");
    }
    printf("\n");
  }
  printf("\n"); }

void Board::showBr() { printf("  brdr (top 1   left 2   right 4)\n");
  showYcore(brdr); 
}

void Board::show() {
  prtBlanks(2); for (char ch='a'; ch < 'a'+N; ch++) printf(" %c ",ch); 
  printf("\n");
  for (int j = 0; j < N; j++) {
    prtBlanks(j);
    printf("%2d ",j+1);
    for (int k=0; k<N-j; k++) printf(" %c ",emit(board[fatten(j,k)]));
    prtBlanks(3*(j+1));
    for (int k=0; k<N-j; k++) prtNonZero(brdr[fatten(j,k)],2);
    printf("\n");
  }
  printf("\n"); }

void Board::showAll() {
  showMiaiPar();
  show();
  //showBr();
  //show();
}

void Board::zero_connectivity(int stone, bool remS) { //printf("Z ");
  for (int j=0; j<TotalGBCells; j++) {
    reply[nx(stone)][j] = j;
    if (board[j]==stone) {
      //prtLcn(j);
      p[j]    = j;
      brdr[j] = BRDR_NIL;
      board[j] = TMP;
      if (remS)
        board[j]= EMP;
    }
  } 
  //printf("\n");
}

void Board::init() { int j,k;
  for (j=0; j<TotalGBCells; j++) {
    board[j] = GRD; brdr[j]  = BRDR_NIL; p[j]  = j;
    for (k=0; k<2; k++) 
      reply[k][j] = j;
  }
  for (j=0; j<N; j++) 
    for (k=0; k<N-j; k++)
      board[fatten(j,k)] = EMP;
  for (j=0; j<N+1; j++) 
    brdr[fatten(0,0)+j-Np2G] = BRDR_TOP;
  for (j=0; j<N+1; j++) 
    brdr[fatten(0,0)+j*Np2G-1] = BRDR_L;
  for (j=0; j<N+1; j++) 
    brdr[fatten(0,0)+j*(Np2G-1)+N] = BRDR_R;
}

Board::Board() { init(); }
