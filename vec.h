#ifndef VEC_H
#define VEC_H

void mymerge(int A[], int a, int B[], int b, int C[], int& c);
void myintersect(int A[], int a, int B[], int b, int C[], int& c);
void showvec(int A[], int n) ;
void copyvec(int A[], int a, int B[], int& b) ;
int myMax(int A[], int n) ;
int index_of_max(int A[], int k, int n) ; // index of max element in A[k..n-1]
int mymax(int x, int y) ;
#endif
