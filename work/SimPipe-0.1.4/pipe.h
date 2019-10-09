/* -*-c++-*-
 * SimPipe: a MIPS Pipeline Simulator using SimMips
 *   Keiji Kimura
 */

#ifndef  PIPE_H
#define  PIPE_H

#include  <cstdio>
#ifndef  L_NAME
#include  "define.h"
#endif
#include  "cache.h"

#define  PIPELOGNAME  "pipe.log"

struct Latch {
    Latch() { contain = false; }
    bool  contain;
    MipsInst  inst;
    uint064_t  paddr;
};

struct RegBoard {
    RegBoard() { locked = 0; ex_fw = false; ex2_fw = false;
	load0_fw = false; load_fw = false; }
    int   locked;
    bool  ex_fw;
    bool  ex2_fw;
    bool  load0_fw;
    bool  load_fw;
};

class PipeLine {
public:
    PipeLine();
    ~PipeLine();

    int PipeInit(int argc, char** argv);

    void ExecLoop();
    void StepPipe();

private:
    char** CheckOpt(int argc, char** argv, int* bargc);
    void   help();

    void Fetch();
    void Decode();
    void Exec();
    void Mem();
    void WriteBack();

    void PutPipeLog();
    void PutConfig();

    inline void ShiftStage(int stageid);
    inline bool RegAvailable(int reg, bool branch);
    inline void WriteBackReg(int reg);

    enum { PIPE_DEPTH = 5 };
    enum { GEN_REG = 32 };
    enum { PIPE_REG_HI = 32, PIPE_REG_LO = 33 };
    enum { STAGE_IDLE,
	   STAGE_BUSY, /* waiting for its execution */
	   STAGE_STALL}; /* waiting for output-latch's ready state */
    enum { SFETCH = 0,
	   SDECODE,
	   SEXEC,
	   SMEM,
	   SWB };

    enum { DEFAULT_DCACHE_SIZE    = 1024,
	   DEFAULT_DCACHE_WAY     = 1,
	   DEFAULT_DCACHE_LINE    = 16,
	   DEFAULT_DCACHE_PENALTY = 10,
	   DEFAULT_DCACHE_WRITEBACK = 1 };

    Board*  board;
    Mips*   mips;
    int    stage_state[PIPE_DEPTH];
    int    stage_wait_cycle[PIPE_DEPTH];
    Latch  latches[PIPE_DEPTH];
    RegBoard  reg_state[GEN_REG+2]; /* GPR(32)+Hi+Lo */

    Cache*  dcache;

    unsigned long long  cycle;
    bool  forwarding;
    bool  pipelog;

    bool dcache_enable;
    uint032_t  dcache_size;
    uint032_t  dcache_way;
    uint032_t  dcache_line;
    int  dcache_penalty;
    bool dcache_writeback;

    FILE*  logfd;
};

#endif	// PIPE_H
