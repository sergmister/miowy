#include <cassert>
#include <cstdio>
#include <cstring>
#include "connect.h"
#include "genmove.h"
#include "move.h"
#include "shuff.h"
#include "node.h"
#include "vec.h"

void topKLcns(int& k, int topKL[], int val[]) { // lcns of top k vals, sorted by val[]
  int local[TotalGBCells]; int dummy; copyvec(val, TotalGBCells, local, dummy);
  int j;
  for (j = 0; j<k; j++) {
    int x = index_of_max(local, j, TotalGBCells);
    //printf("%d ",val[x]); 
    if (local[x]==0) { printf("oops\n"); break; }
    swap(local[x], local[j]);
    topKL[j] = x;
  }
  k = j;
  printf("top %d moves: ",k);
  for (int j = 0; j<k; j++) {
    prtLcn(topKL[j]); printf(" ");
  }
  printf("\n");
}

ScoreLcn ngmx_MCS (int r, Board& Brd, int s, int d, bool v) {
  bool uzM = true; bool acc = true;
  int BestLcns[TotalCells]; int k = WDTH;
  Board B = Brd; // local copy
  ScoreLcn rootsl = flat_MCS(r, B, s, uzM, acc, v, k, BestLcns);
  if (d==0) return rootsl;
  double bestScr = -2.0;
  return flat_MCS(r, B, s, uzM, acc, v, k, BestLcns);
}

void prtPlayoutMsg(int sim, char c, int lcn, int moves) {
  printf("sim %d: %c wins ",sim,c); 
  prtLcn(lcn); 
  printf(" move %d\n",moves);
}

void prtThrtMsg(int sim, char c, int lcn, int moves, int threatn) {
  printf("  threat %d: ",threatn); 
  prtPlayoutMsg(sim,c,lcn,moves);
}

void init_to_zero(int A[], int n) { int j;
  for (j=0; j<n; j++) A[j] = 0;
}

double ratio(int n, int d) {
  return (d==0) ? double(INFINITY): (double)n/(double)d;
}

double wrate(int wins, int opt_wins, int rollouts) { // zero sum
  assert(wins+opt_wins==rollouts);
  if (wins==0) return     0.00000001;
  if (opt_wins==0) return 0.99999999;
  return (double)wins/(double)rollouts;
}
// TODO: more sophisticated ?
//double score(int wins, int opt_wins, int sum_lengths, int opt_sum_lengths) {
  //return -.5 + (double)wins/(double)ROLLOUTS+ //monte carlo win prob
         //(double)opt_sum_lengths/(double)opt_wins- 
	 //(double)sum_lengths/(double)wins;

int rand_move(Board& B) { 
  Playout pl(B);
  return pl.Avail[myrand(0,pl.numAvail)];
}

int rand_miai_move(Board& B, int s) { 
  Playout pl(B);
  int numMiai = 0;
  int miaiLcns[TotalCells];
  for (int j = 0; j < pl.numAvail; j++)
    if (!B.not_in_miai(Move(s,pl.Avail[j]))) 
      miaiLcns[numMiai++] = pl.Avail[j];
  assert(numMiai>0);
  //printf("miai carrier size %d  ",numMiai);
  //for (int j = 0; j < numMiai; j++) {prtLcn(miaiLcns[j]); printf(" "); }
  //printf("\n");
  //int r = myrand(0,numMiai); printf("rand = %d\n",r);
  int lcn = miaiLcns[myrand(0,numMiai)];
  assert(B.board[lcn]==EMP);
  return lcn;
}

int uct_move(int rllouts, Board& B, int s, bool useMiai) {
  Node root; // init
  root.expand(B,s);
  for (int j =0; j< rllouts; j++) {
    Board L = B;
    root.uct_playout(L,s,useMiai);
  }
  int childWins[TotalGBCells];
  int childVisits[TotalGBCells];
  int uctEval[TotalGBCells];
  memset(childWins,0,sizeof(childWins));
  memset(childVisits,0,sizeof(childVisits));
  memset(uctEval,0,sizeof(uctEval));
  for (int j=0; j < root.numChildren; j++) {
    Child& c = root.children[j];
    childWins[c.lcn] = c.node.stat.w;
    childVisits[c.lcn] = c.node.stat.n;
    uctEval[c.lcn] = 1000*root.ucb_eval(c.node);
  }
  printf("childWins  childVisits uctEval\n");
  showYcore(childWins);
  showYcore(childVisits);
  //showYcore(uctEval);
  return root.bestMove();
}

void provenWinAbort(Playout& pl, int turn, int j) {
  pl.colorScore[ndx(turn)] = ROLLOUTS;
  pl.colorScore[ndx(oppnt(turn))] = 0;
  printf(" found winmove sim %d for player %c, abort \n",j,emit(turn));
}

void updateInfo(Playout& pl, int turn, int jw) {
  pl.colorScore[ndx(turn)] ++;
  pl.win_length[ndx(turn)] += jw/2;
  pl.wins[pl.Avail[jw]]++;
  pl.winsBW[ndx(turn)][pl.Avail[jw]]++;
  for (int q = jw; q >= 0; q = q-2) pl.AMAF[ndx(turn)][pl.Avail[q]]++;
  if (jw < pl.minwinlen[ndx(turn)]) // found shorter win
    pl.minwinlen[ndx(turn)] = jw/2;
}

void printInfo(Playout& pl, int s, int r, double winrate) { 
  //assert(ROLLOUTS == pl.colorScore[0]+pl.colorScore[1]);
  int mywins = pl.colorScore[ndx(s)];
  int owins  = pl.colorScore[ndx(oppnt(s))];
  int mysum = pl.win_length[ndx(s)];
  int osum = pl.win_length[ndx(oppnt(s))];
  printf("%c wins %.2f after %d sims ", emit(s), winrate,r);
  printf("len %2.2f (oppt %2.2f) scr %2.2f minlen b %d w %d\n\n",
    ratio(mysum,mywins), ratio(osum,owins), winrate, pl.minwinlen[0], pl.minwinlen[1]);
  //showYcore(pl.wins); 
  showBothYcore(pl.winsBW[ndx(s)],pl.winsBW[ndx(oppnt(s))]);
  showBothYcore(pl.AMAF[ndx(s)],pl.AMAF[ndx(oppnt(s))]);
}

int indexOf(int x, int A[], int n) { int j; // return index of x in A[0]...A[n-1]
  for (j=0; (j<n-1)&&(x!=A[j]);j++) ; // 
  assert(A[j]==x);
  return j;
}

void addtoFrontSublist(int x, int A[], int& sz, int n) { // postcondition: x is in intial sublist of A[0..sz]
  int j = indexOf(x, A, n);
  if (j>sz)  // x is not in sublist, so adjust
    swap(A[j],A[++sz]);
}

void set_MP(Board& B, int s, bool useMiai, int MP[], int& MPsize, bool vrbs) { 
  int T[TotalCells]; int threats = 0; int C[TotalCells]; int Csize; int M[TotalCells]; int Msize;
  if (vrbs) printf("\nsetting mustplay %c \n",emit(s));
  for (int r=0; r<N; r++) {
    for (int c=0; c<N-r; c++) {
      int lcn = fatten(r,c);
      Move mv(s,lcn);
      if ((B.board[lcn]==EMP)&&(is_win(B,mv,useMiai,M,Msize,vrbs))) { // new threat
        T[threats++] = lcn;
        if (threats==1) {// first threat, MP <- carrier
          copyvec(M, Msize, MP, MPsize);
        }
        else  { // not first threat
          myintersect(M, Msize, MP, MPsize, C, Csize);
          copyvec(C, Csize, MP, MPsize);
        }
        //if (vrbs) {printf("  size now %d ",MPsize); for (int j=0; j<MPsize; j++) {prtLcn(MP[j]);printf(" ");} printf("\n");}
      }
    }
  }
  if (vrbs) printf("%d threats found\n",threats);
  assert(threats>0);
}

void myshuffle(Playout& pl, int& end_w, const int& j_w, const bool& acc) {
  if (!acc) 
    shuffle_interval(pl.Avail,0,pl.numAvail-1);
  else { // if accelerate
    if (j_w > end_w) { // add to winners; shuffle remainders
      swap(pl.Avail[++end_w], pl.Avail[j_w]);  // add new winner to end of sublist
      shuffle_interval(pl.Avail,end_w+1,pl.numAvail-1);  // shuffle nonwinners
    }
    shuffle_interval(pl.Avail,0,end_w); //shuffle winners
  }
}

void threatInit(int MP[], int MPsize, int A[], int ep1) {
  int lcn = MP[myrand(0,MPsize)];
  int x = indexOf(lcn, A, ep1);
  swap(A[1], A[x]);
  swap(A[0], A[1]); // mp in psn 0, prev. winner in psn 1
  assert(A[0]==lcn);
}

int sortMPLcn(int MP[], int MPsize, int s, int A[2][TotalGBCells]) { // sort by A[][] val
  //printf("mp size %d s %d",MPsize,s);
  for (int t=0; t< MPsize-1; t++) {
    int ndxMax = t;
    for (int q=t+1; q<MPsize; q++) 
      if (A[ndx(s)][MP[q]] > A[ndx(s)][MP[ndxMax]]) ndxMax = q;
    swap(MP[t], MP[ndxMax]);
  }
  if (A[ndx(s)][MP[0]]==0) { // all A[ndx(s)] vals are 0, so sort by oppnt vals
    for (int t=0; t< MPsize-1; t++) {
      int ndxMax = t;
      for (int q=t+1; q<MPsize; q++) 
        if (A[ndx(s)][MP[q]] > A[ndx(s)][MP[ndxMax]]) ndxMax = q;
      swap(MP[t], MP[ndxMax]);
    }
  }
  return MP[0];
}

double mysmooth(double wr, int sims) {
  if (sims < 70) wr = 0.5*(1.0+ wr);  // if not enough sims, then smooth value
  return wr;
}

ScoreLcn flat_MCS(int rllouts, Board& local, int s, 
  bool useMiai, bool accelerate, bool vrbs, int& MPsize, int MP[]) {  // MP will be MPsize best moves, which defaults to mustplay if there is one
  // accelerate? winners   sublist A[0             .. end_winners]
  //             remainder sublist A[end_winners+1 ..  TotalCells]
  //int MPsize; int MP[7]; // mustplay, restricted to oppnt winner plus neighbours
  ScoreLcn sl;
  bool threat = false;  //bool threat2 = false;
  Playout pl(local); 
  assert(pl.numAvail!=0);
  int end_winners = -1; int just_won = -1;
  for (int j=0; j<1; j++) shuffle_interval(pl.Avail,0,pl.numAvail-1);
  int turn;
  for (int j=0; j< rllouts; j++) { 
    myshuffle(pl, end_winners, just_won, accelerate);
    turn = s;
    if (threat) 
      threatInit(MP, MPsize, pl.Avail, end_winners+1);
    pl.single_playout(turn, just_won, useMiai);
    //if ((j<2)&&(vrbs))  prtPlayoutMsg(j,emit(turn),pl.Avail[just_won],just_won); // data for user
    updateInfo(pl, turn, just_won);
    if ((just_won==1)&&(!useMiai)) { 
      MP[0] = pl.Avail[just_won];
      double wr = mysmooth(wrate(pl.colorScore[ndx(s)],pl.colorScore[ndx(oppnt(s))],j+1),j+1);
      if (vrbs) { printf("not using miai: mustplay 1\n"); printInfo(pl,s,j+1,wr); }
      ScoreLcn sl(0.0, pl.Avail[just_won]); return sl;
    }
    if (just_won ==1) { // using miai, threat detected
      if (!threat) { // first threat
        threat = true; 
        if (vrbs) prtThrtMsg(j,emit(turn),pl.Avail[1],just_won,1);
        set_MP(local, turn, useMiai, MP, MPsize, vrbs);
        if (MPsize==0) { 
          if (vrbs) printf("mustplay empty after %d sims\n",j+1); 
          return ScoreLcn(0.0, MP[0]);
        }
        if (MPsize==1) {  
          double wr = mysmooth(wrate(pl.colorScore[ndx(s)],pl.colorScore[ndx(oppnt(s))],j+1),j+1);
          if (vrbs) { printf("using miai: mustplay 1\n"); printInfo(pl, s, j+1, wr); }
          return ScoreLcn(wr, MP[0]);
        } // MPsize > 1
        for (int q=0; q<MPsize; q++) { 
          addtoFrontSublist(MP[q], pl.Avail, end_winners, TotalCells);
        }
      }
      //else { // not first threat //if (!threat2) { // second threat 
               //threat2 = true; if (vrbs) prtThrtMsg(j,emit(turn),pl.Avail[1],just_won,2); //} //}
    }
    if (just_won == 0) { // found winning move, abort
      ScoreLcn sl(1.0, pl.Avail[just_won]); // no need to set best moves, we have one winning move
      if (vrbs) { 
        double wr = wrate(pl.colorScore[ndx(s)],pl.colorScore[ndx(oppnt(s))],j+1);
        printf("search found winner move "); printInfo(pl, s, j+1, wr); }
      return sl; //provenWinAbort(pl,turn,j);
    }
  }
  int z = index_of_max(pl.AMAF[ndx(s)],0,TotalGBCells); // sims finished without solving
  if (threat && (MPsize>1)) {
    z = sortMPLcn(MP, MPsize, s, pl.AMAF);
    assert(local.board[z]==EMP);
  }
  double wr = wrate(pl.colorScore[ndx(s)],pl.colorScore[ndx(oppnt(s))],rllouts);
  if (MPsize>1) topKLcns(MPsize, MP, pl.AMAF[ndx(s)]);
  sl = ScoreLcn(wr,z);
  if (vrbs) printInfo(pl, s, rllouts, wr);
  if (local.board[z]==EMP) return sl;
  assert(MPsize>1);
  topKLcns(MPsize, MP, pl.AMAF[ndx(oppnt(s))]);
  sl = ScoreLcn(wr,index_of_max(pl.AMAF[ndx(oppnt(s))],0,TotalGBCells));
  return sl;
}
