//   play y               RBH 2012 ... tips from Jakub
#include <cstdio>
#include <cstdlib>

#include "board.hpp"
#include "connect.hpp"
#include "interact.hpp"

int main(void) {
    Board myB;  // myB.showBr();display_nearedges();
    interact(myB);
    return 0;
}
