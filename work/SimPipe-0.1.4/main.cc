#include  "pipe.h"

int
main(int argc, char** argv)
{
    PipeLine*  p;

    p = new PipeLine();
    if (p->PipeInit(argc, argv) == 0) {
	p->ExecLoop();
    }

    return  0;
}
