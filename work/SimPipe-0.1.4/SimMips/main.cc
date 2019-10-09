/**********************************************************************
 * SimMips: Simple Computer Simulator of MIPS    Arch Lab. TOKYO TECH *
 **********************************************************************/
#include "define.h"

/**********************************************************************/
int main(int argc, char *argv[])
{
    printf("## %s %s\n", L_NAME, L_VER);

    Board *board = new Board();

    // execute if successfully initialized
    if (board->siminit(argc, argv) == 0)
        board->exec();
    
    DELETE(board);
    return 0;
}

/**********************************************************************/
