#include <cassert>
#include <cstdio>
#include <cstring>
#include "connect.h"
#include "genmove.h"
#include "move.h"
#include "shuff.h"
#include "node.h"
#include "vec.h"

int mymin(int x, int y) { return (x<y) ? x : y ; }

void topKLcns(int topKL[], int& k, int val[]) { // lcns of top k vals, sorted by val[]
  int local[TotalGBCells]; int dummy; copyvec(val, TotalGBCells, local, dummy);
  int j;
  for (j = 0; j<k; j++) {
    int x = index_of_max(local, j, TotalGBCells);
    //printf("%d ",val[x]); 
    //if (local[x]==0) break; 
    swap(local[x], local[j]);
    topKL[j] = x;
  }
  k = j;
}

ScoreLcn negIfNec(bool rtClr, ScoreLcn sl) {
  if (rtClr) return sl;
  return ScoreLcn(negScr(sl.scr), sl.lcn);  // negate score
}

ScoreLcn ngmx_MCS (int r, Board& Bd, int s, bool rtClr,
                   int d, int w, int alf, int bet, bool v) {
  bool uzM = true; bool acc = true; int Children[TotalCells]; int numCh;
  Board B = Bd; Playout pl(B); 

  ScoreLcn rootsl = flat_MCS(r, B, pl, s, uzM, acc, v);
  if (d==0||rootsl.scr==MAXSCORE||rootsl.scr==0) 
    return negIfNec(rtClr, rootsl); 
  assert(pl.mpsz>0);
  if (pl.mpsz==1) { 
    Children[0] = rootsl.lcn; numCh = 1;}
  else {
    numCh = w; if (pl.mpsz<pl.numAvail) numCh = pl.mpsz;
    printf("w %d pl.mpsz %d min %d\n",w, pl.mpsz, numCh);
    topKLcns(Children, numCh, pl.AMAF[nx(s)]);  // flat_MCS erased non-mp vals
  }
  printf("top %d moves: ",numCh);
  for (int j = 0; j<numCh; j++) { prtLcn(Children[j]); printf(" "); } printf("\n");
  ScoreLcn bstSL(-1, -1);
  for (int j=0; j<numCh; j++) {
    Board chB = B; 
    int chMiai; int chbdst = BRDR_NIL; Move chMv(s,Children[j]); 
    chMiai = chB.move(chMv, uzM, chbdst); chB.show();
    ScoreLcn chSL = ngmx_MCS(r, chB, opt(s), !rtClr, d-1, w, negScr(bet), negScr(alf), v); 
    chSL.scr = negScr(chSL.scr);
    printf("tried move %c ",emit(opt(s))); prtLcn(chSL.lcn); printf(" %.2f\n",scrToFlt(chSL.scr));
    if (chSL.scr > bstSL.scr) { bstSL.scr = chSL.scr; bstSL.lcn = Children[j]; }
    alf = mymax(alf, chSL.scr);
    if (alf >= bet) break;
  }
  return bstSL;
}

void prtPlayoutMsg(int sim, char c, int lcn, int moves) {
  printf("sim %d: %c wins ",sim,c); prtLcn(lcn); printf(" move %d\n",moves);
}

void prtThrtMsg(int sim, char c, int lcn, int moves, int threatn) {
  printf("  threat %d: ",threatn); 
  prtPlayoutMsg(sim,c,lcn,moves);
}

void init_to_zero(int A[], int n) { int j;
  for (j=0; j<n; j++) A[j] = 0;
}

double ratio(int n, int d) {
  return (d==0) ? double(INFNTY): (double)n/(double)d;
}

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
  pl.wins[nx(turn)] = ROLLOUTS;
  pl.wins[nx(opt(turn))] = 0;
  printf(" found winmove sim %d for player %c, abort \n",j,emit(turn));
}

void updateInfo(Playout& pl, int turn, int jw) {
  pl.wins[nx(turn)] ++;
  pl.win_length[nx(turn)] += jw/2;
  //pl.wins[pl.Avail[jw]]++;
  pl.cellWins[nx(turn)][pl.Avail[jw]]++;
  for (int q = jw; q >= 0; q = q-2) pl.AMAF[nx(turn)][pl.Avail[q]]++;
  if (jw < pl.minwinlen[nx(turn)]) // found shorter win
    pl.minwinlen[nx(turn)] = jw/2;
}

void printInfo(Playout& pl, int s, int r) { 
  //assert(ROLLOUTS == pl.wins[0]+pl.wins[1]);
  int mywins = pl.wins[nx(s)];
  int owins  = pl.wins[nx(opt(s))];
  int mysum = pl.win_length[nx(s)];
  int osum = pl.win_length[nx(opt(s))];
  printf("%c wins %.2f after %d sims ", emit(s), float(mywins)/float(r), r);
  printf("len %2.2f (oppt %2.2f) minlen b %d w %d\n\n",
    ratio(mysum,mywins), ratio(osum,owins), pl.minwinlen[0], pl.minwinlen[1]);
  //showYcore(pl.wins); 
  showBothYcore(pl.cellWins[nx(s)],pl.cellWins[nx(opt(s))]);
  showBothYcore(pl.AMAF[nx(s)],pl.AMAF[nx(opt(s))]);
}

int indexOf(int x, int A[], int n) { int j; // return index of x in A[0]...A[n-1]
  for (j=0; (j<n-1)&&(x!=A[j]);j++) ; // 
  assert(A[j]==x);
  return j;
}

void addtoFrontSublist(int x, int A[], int& sz, int n) { 
// postcondition: x is in intial sublist of A[0..sz]
  int j = indexOf(x, A, n);
  if (j>sz)  // x is not in sublist, so adjust
    swap(A[j],A[++sz]);
}

void set_MP(Board& B, Playout& pl, int s, bool useMiai, bool vrbs) { 
//   * try every possible move as a killer
//   * if yes, its carrier is itself plus all immediate nbrs in miai
//   * final mustplay is intersection of all such carriers
//   * each move in final mustplay has no killer
  int T[TotalCells]; int threats = 0; 
  int C[TotalCells]; int Csz; 
  int M[TotalCells]; int Msz;
  if (vrbs) printf("\nsetting mustplay %c \n",emit(s));
  for (int r=0; r<N; r++) { // check every possible move for win, if yes, refine MP
    for (int c=0; c<N-r; c++) {
      int lcn = fatten(r,c);
      Move mv(s,lcn);
      if ((B.board[lcn]==EMP)&&(is_win(B,mv,useMiai,M,Msz,vrbs))) { // new threat
        T[threats++] = lcn;
        if (threats==1) {// first threat, MP <- carrier
          copyvec(M, Msz, pl.MP, pl.mpsz);
        }
        else  { // not first threat
          myintersect(M, Msz, pl.MP, pl.mpsz, C, Csz);
          copyvec(C, Csz, pl.MP, pl.mpsz);
        }
      }
    }
  }
  if (vrbs) printf("%d threats, mustplay %d found\n",threats,pl.mpsz);
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

void threatInit(Playout& pl, int& k) {  // cycle through mustplay as move[0]
  assert(pl.mpsz > 0);
  assert((k >= 0)&&(k < pl.mpsz));
  int ndx = indexOf(pl.MP[k], pl.Avail, TotalCells);
  swap(pl.Avail[0], pl.Avail[ndx]); // next mustplay element
  k++; if (k==pl.mpsz) k=0;         // increment k, cyclically
}

int sortMPLcn(int MP[], int MPsize, int s, int A[2][TotalGBCells]) { // sort by A[][] val
  //printf("mp size %d s %d",MPsize,s);
  for (int t=0; t< MPsize-1; t++) {
    int ndxMax = t;
    for (int q=t+1; q<MPsize; q++) 
      if (A[nx(s)][MP[q]] > A[nx(s)][MP[ndxMax]]) ndxMax = q;
    swap(MP[t], MP[ndxMax]);
  }
  if (A[nx(s)][MP[0]]==0) { // all A[nx(s)] vals are 0, so sort by opt vals
    for (int t=0; t< MPsize-1; t++) {
      int ndxMax = t;
      for (int q=t+1; q<MPsize; q++) 
        if (A[nx(s)][MP[q]] > A[nx(s)][MP[ndxMax]]) ndxMax = q;
      swap(MP[t], MP[ndxMax]);
    }
  }
  return MP[0];
}

int wrate(int wins, int opt_wins, int sims, int maxSims) {
  assert(wins+opt_wins==sims);
  int wr = fltToScr(float(wins)/float(sims));
  if (sims < maxSims/10) // too few sims, smooth
    return fltToScr(0.5*(1.0 + scrToFlt(wr))); 
  return wr;
}
// TODO: more sophisticated ? maybe use length?
//double score(int wins, int opt_wins, int sum_lengths, int opt_sum_lengths) {
  //return -.5 + (double)wins/(double)ROLLOUTS+ //monte carlo win prob
         //(double)opt_sum_lengths/(double)opt_wins- 
	 //(double)sum_lengths/(double)wins;

void erase(int& x) { // make negative, used to erase non-mustplay cell scores
  if (x>0) x *=-1; else x--;
}

ScoreLcn goodMove(Playout& pl, int s, int sims, int maxSims, bool v) { ScoreLcn sl;
  if (pl.mpsz==0) {  if (v) printf("mustplay 0 after %d sims\n", sims); 
    sl.scr = 0; 
    sl.lcn = pl.MP[0]; 
  }
  else if (pl.mpsz==1) { if (v) printf("mustplay 1 after %d sims\n", sims); 
    sl.scr = wrate(pl.wins[nx(s)], pl.wins[nx(opt(s))], sims, maxSims);
    if (sl.scr==0) sl.scr = 1; // not a proven loss
    sl.lcn = pl.MP[0];
  }
  else { // pl.msz > 1, sims finished without solving
    assert(sims == maxSims);
    if (pl.mpsz < pl.numAvail) { // nontrivial mustplay, zero out other scores
      bool inMP[TotalGBCells];
      for (int x = 0; x < N; x ++) for (int y=0; y< N-x; y++) inMP[fatten(x,y)] = false;
      for (int m=0; m<pl.mpsz; m++) inMP[pl.MP[m]] = true;
      for (int x = 0; x < N; x ++) for (int y=0; y< N-x; y++) {
        int psn = fatten(x,y);
        if (!inMP[psn]) { erase(pl.AMAF[nx(s)][psn]); erase(pl.AMAF[nx(opt(s))][psn]); }
      }
    }
    if (pl.wins[nx(s)]>0) {
      sl.scr = wrate(pl.wins[nx(s)], pl.wins[nx(opt(s))], sims, maxSims);
      sl.lcn = index_of_max(pl.AMAF[nx(s)], 0, TotalGBCells); 
    } 
    else {
      printf(" no wins but not proven loss ???? \n");
      sl.scr = 1; // not a proven loss yet...
      sl.lcn = index_of_max(pl.AMAF[nx(opt(s))], 0, TotalGBCells);
    }
  }
  if (v) printInfo(pl, s, sims);
  return sl;
}

ScoreLcn flat_MCS(int& r, Board& B, Playout& pl, int s, bool uzMi, bool acc, bool v) {  // mustplay left at front of Avail
  // accelerate? winners   sublist A[0             .. end_winners]
  //             remainder sublist A[end_winners+1 ..  TotalCells]
  int maxr = r; // kick out early if win/loss
  bool threat = false;  //bool threat2 = false;
  assert(pl.numAvail!=0);
  int end_winners = -1; int just_won = -1;
  //for (int j=0; j<1; j++) shuffle_interval(pl.Avail,0,pl.numAvail-1);
  int turn; int MPndx = 0;
  for (int j=0; j< r; j++) { 
    myshuffle(pl, end_winners, just_won, acc);
    turn = s;
    if (threat) threatInit(pl, MPndx); 
    pl.single_playout(turn, just_won, uzMi);
    //if ((j<2)&&(v))  prtPlayoutMsg(j,emit(turn),pl.Avail[just_won],just_won); // data for user
    updateInfo(pl, turn, just_won);
    if ((just_won==1)&&(!uzMi)) { // found mp singleton, update pl and return
      pl.mpsz = 1; r = j+1; return goodMove(pl, s, r, maxr, v);
    }
    if (just_won == 1) { // using miai, threat detected
      if (!threat) { // first threat
        threat = true; 
        if (v) prtThrtMsg(j,emit(turn),pl.Avail[1],just_won,1);
        set_MP(B, pl, turn, uzMi, v); // set mustplay
        if (pl.mpsz==0||pl.mpsz==1) { r = j+1; return goodMove(pl, s, r, maxr, v); }
        //  pl.mpsz > 1 ... insert MP into winners sublist at front of Avail
        for (int q=0; q < pl.mpsz; q++) { 
          addtoFrontSublist(pl.MP[q], pl.Avail, end_winners, TotalCells);
        }
      }
    }
    if (just_won == 0) { // found win
      r = j+1;
      if (v) { printf("search found win "); printInfo(pl, s, r); }
      return ScoreLcn(MAXSCORE, pl.Avail[just_won]);
    }
  } // no win after r sims
  return goodMove(pl, s, r, maxr, v);
}
