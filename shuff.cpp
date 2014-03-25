#include <cstdio>
#include "shuff.h"

//shuffle vector interval indexed a..b
void shuffle_interval(int X[], int a, int b) {
  int j,k;
  for (k = b; k > a; k--) {
    j = myrand(a,k);  //printf("rand %d\n", j);
    swap(X[j],X[k]);
  }
}

