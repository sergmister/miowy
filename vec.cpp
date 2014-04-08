#include <cstdio>
#include <cassert>
#include "vec.h"
#include "board.h"

int index_of_max(int A[], int k, int n) { // index of max element in A[k..n-1]
  int x = k; 
  for (int j=k+1; j<n; j++) 
    if (A[j]>A[x]) 
      x = j;
  return x;
}

int mymax(int x, int y) { if (x>y) return x; return y; }

int myMax(int A[], int n) {
  assert(n>=0);
  int x = A[0];
  for (int j=1; j<n; j++) 
    if (x < A[j]) 
      x = A[j];
  return x;
}

void mymerge(int A[], int a, int B[], int b, int C[], int& c) {
  int x=0; int y=0; 
  for (c=0; (x<a) && (y<b); ) {
    if (A[x]<B[y]) 
      C[c++] = A[x++];
    else {
      if (A[x]==B[y]) x++;
      C[c++] = B[y++];
    }
  }
  while (x<a) C[c++] = A[x++]; 
  while (y<b) C[c++] = B[y++];
}

void myintersect(int A[], int a, int B[], int b, int C[], int& c) {
  //showvec(A,a);
  //showvec(B,b);
  c = 0;
  for (int x=0; x<a; x++)
    for (int y=0; y<b; y++)
      if (A[x]==B[y]) C[c++] = A[x];
  //below works only if sorted :(
  //int x=0; int y=0; 
  //for (c=0; (x<a) && (y<b); ) {
    //if (A[x]<B[y]) x++;
    //else {
      //if (A[x]==B[y]) C[c++] = A[x++];
      //y++;
    //}
  //}
  //printf("intersection "); showvec(C,c);
}

void showvec(int A[], int n) {
  for (int j=0; j<n; j++) { prtLcn(A[j]); printf(" "); }
  printf("\n");
}

void copyvec(int A[], int a, int B[], int& b) {
  b = a;
  for (int j=0; j<b; j++) B[j]=A[j];
  //showvec(B,b);
}

void vectest(int tA[], int a, int tB[], int b) {
  int tC[1000]; int c = 1000;
  printf("intersection test\n");
  showvec(tA,a); showvec(tB,b); 
  myintersect(tA,a,tB,b,tC,c); showvec(tC,c);
  myintersect(tB,b,tA,a,tC,c); showvec(tC,c);
}
