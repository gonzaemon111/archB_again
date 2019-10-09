/* -*-c++-*-
 * SimPipe: a MIPS Pipeline Simulator using SimMips
 *   Keiji Kimura
 */

#include  <cassert>
#include  <cstdlib>
#include  <cstring>
#include  "pipe.h"

#undef  DEBUG_PIPELINE

/* from SimMips/board.cc */
extern volatile sig_atomic_t recieve_int;

PipeLine::PipeLine()
    : cycle(0), forwarding(true), pipelog(false),
      dcache_enable(false),
      dcache_size(DEFAULT_DCACHE_SIZE),
      dcache_way(DEFAULT_DCACHE_WAY),
      dcache_line(DEFAULT_DCACHE_LINE),
      dcache_penalty(DEFAULT_DCACHE_PENALTY),
      dcache_writeback(DEFAULT_DCACHE_WRITEBACK)
{
    for (int i = 0; i < PIPE_DEPTH; i++) {
	stage_state[i] = STAGE_IDLE;
	stage_wait_cycle[i] = 0;
    }
}

PipeLine::~PipeLine()
{
    delete  board;
    if (dcache_enable) {
	delete  dcache;
    }
}

int
PipeLine::PipeInit(int argc, char** argv)
{
    int bargc;
    char** bargv;
    bargv = CheckOpt(argc, argv, &bargc);
    PutConfig();

    board = new Board();
    int ret = board->siminit(bargc, bargv);
    if (ret > 0) {
	return ret;
    }
    mips = board->chip->mips;
    for (int i = 0; i < PIPE_DEPTH; i++) {
	latches[i].inst.op = 0;
    }
    if (pipelog) {
	if ((logfd = fopen(PIPELOGNAME, "w")) == NULL) {
	    fprintf(stderr, "Can't open pipe-log file.\n");
	    ret = 1;
	}
	fprintf(logfd, "        |        |   F   |   D   |   E   |   M   |   W   |\n");
    }

    dcache = new Cache(dcache_size, dcache_way, dcache_line,
		       dcache_penalty, dcache_writeback);


    return  ret;
}

char**
PipeLine::CheckOpt(int argc, char** argv, int* bargc)
{
    char**  bargv;
    *bargc = 0;

    bargv = new char*[argc+1];
    memset(bargv, 0, sizeof(char*)*(argc+1));

    bargv[(*bargc)++] = argv[0];
    char*  opt;
    for (int i = 1; (opt = argv[i]) != NULL; i++) {
	if (opt[0] != '-') {
	    bargv[(*bargc)++] = argv[i];
	    continue;
	}
	switch (opt[1]) {
	case  'd':
	    if (strcmp(opt+2, "cache-size") == 0) {
		dcache_enable = true;
		dcache_size = atoi(argv[++i]) * 1024;
	    } else if (strcmp(opt+2, "cache-way") == 0) {
		dcache_enable = true;
		dcache_way = atoi(argv[++i]);
	    } else if (strcmp(opt+2, "cache-line") == 0) {
		dcache_enable = true;
		dcache_line = atoi(argv[++i]);
	    } else if (strcmp(opt+2, "cache-penalty") == 0) {
		dcache_enable = true;
		dcache_penalty = atoi(argv[++i]);
	    } else if (strcmp(opt+2, "cache-writeback") == 0) {
		dcache_enable = true;
		dcache_writeback = atoi(argv[++i]);
	    } else {
		fprintf(stderr, "Invalid data cache parameter %s\n", opt);
	    }
	    break;
	case  'f':
	    if (opt[2] == '0') {
		forwarding = false;
	    } else if (opt[2] == '1') {
		forwarding = true;
	    } else {
		fprintf(stderr, "Invalid forwarding parameter %c\n", opt[2]);
	    }
	    break;
	case  'h':
	    help();
	    bargv[(*bargc)++] = argv[i];
	    break;
	case  'l':
	    pipelog = true;
	    break;
	default:
	    bargv[(*bargc)++] = argv[i];
	}
    }

    return  bargv;
}

void
PipeLine::PutConfig()
{
    printf("* Pipeline Configuration *\n");
    printf("  Forwarding: %s\n", forwarding ? "Yes" : "No");
    if (dcache_enable) {
	printf("  DataCache Enabled\n");
	printf("   Size:      %d KB\n", dcache_size/1024);
	printf("   Way:       %d\n", dcache_way);
	printf("   Line:      %d\n", dcache_line);
	printf("   WriteBack: %s\n", dcache_writeback ? "Yes" : "No");
	printf("   Penalty:   %d cycles\n", dcache_penalty);
    } else {
	printf("  DataCache Disabled\n");
    }
    printf("\n");
}

void
PipeLine::help()
{
    printf("Options for pipeline simulator\n"
	   " -dcache-size [num]: Data cache size in KByte\n"
	   " -dcache-way [num]: Number of ways for data cache\n"
	   " -dcache-line [num] : Line size for data cache in byte\n"
	   " -dcache-penalty [num]: Data cache miss penalty cycles\n"
	   " -dcache-writeback [01]: Data cache write-back [1] or write-through [1]\n"
	   " -f[01]: Disable forwarding [0] or Enable forwarding [1]\n"
	   " -l : Output pipeline log file\n");
}

void
PipeLine::ExecLoop()
{
    board->gettime();
    while (mips->running() && !recieve_int) {
	StepPipe();
    }

    if (recieve_int) {
	printf("\n** Interrupted! **\n");
    }
    double simtime = (double)board->gettime()/1000000.0;
    printf("\n####################\n");
    printf("## cycle count: %lld\n", cycle);
    printf("## inst count: %lld\n", mips->ss->inst_count);
    printf("## IPC: %f\n", (double)mips->ss->inst_count/cycle);
    printf("## simulation time: %8.3f\n", simtime);
    if (dcache_enable) {
	dcache->PutStatistics();
    }
}

void
PipeLine::StepPipe()
{
    cycle++;
    WriteBack();
    Mem();
    Exec();
    Decode();
    Fetch();

    if (pipelog) {
	PutPipeLog();
    }

    latches[SWB].inst.op = 0;
    for (int i = PIPE_DEPTH-2; i >= 0; i--) {
	if (stage_state[i] == STAGE_STALL && !latches[i+1].contain) {
	    ShiftStage(i);
	}
    }
}

void
PipeLine::PutPipeLog()
{
    fprintf(logfd, "%6lld: |%8x|%7s|%7s|%7s|%7s|%7s|\n",
	    cycle,
	    latches[SFETCH].inst.pc,
	    latches[SFETCH].inst.getinstname(),
	    latches[SDECODE].inst.getinstname(),
	    latches[SEXEC].inst.getinstname(),
	    latches[SMEM].inst.getinstname(),
	    latches[SWB].inst.getinstname());
}

inline void
PipeLine::ShiftStage(int stageid)
{
    memcpy(&latches[stageid+1].inst, &latches[stageid].inst, sizeof(MipsInst));
    latches[stageid+1].paddr = latches[stageid].paddr;
    latches[stageid+1].contain = true;
    latches[stageid  ].contain = false;
    latches[stageid  ].inst.op = 0;
    stage_state[stageid] = STAGE_IDLE;
}

inline bool
PipeLine::RegAvailable(int reg, bool branch)
{
    if (reg == 0) {
	return true;
    }
    if (reg_state[reg].locked > 0) {
	if (!forwarding) {
	    return false;
	}
	if (branch) {
	    if (reg_state[reg].ex2_fw) {
		return true;
	    }
	} else {
	    if (reg_state[reg].ex_fw
		|| (!reg_state[reg].load0_fw && reg_state[reg].ex2_fw)
		|| reg_state[reg].load_fw) {
		return true;
	    }
	}
	return false;
    }
    return true;
}

void
PipeLine::Fetch()
{
    switch (stage_state[SFETCH]) {
    case  STAGE_IDLE: {
#ifdef  DEBUG_PIPELINE
	fprintf(stderr, "Fetch Address: %x\n", mips->as->pc);
#endif

	board->chip->step_funct();
	memcpy(&latches[SFETCH].inst, mips->inst, sizeof(MipsInst));
	if (mips->inst->attr & LOADSTORE) {
	    latches[SFETCH].paddr = mips->get_paddr();
	}
	stage_state[SFETCH] = STAGE_STALL;
    } break;
    case  STAGE_STALL:
	/* Nothing to do */
	break;
    default:
	fprintf(stderr, "Invalid State for Fetch Stage!\n");
	abort();
    }
}

void
PipeLine::Decode()
{
    switch (stage_state[SDECODE]) {
    case  STAGE_IDLE: {
	if (latches[SDECODE].contain) {
	    MipsInst*  inst = &latches[SDECODE].inst;
#ifdef  DEBUG_PIPELINE
	    fprintf(stderr, "Decode: %s\n", inst->getinstname());
#endif
	    bool  ready_rs = true;
	    bool  ready_rt = true;
	    bool  ready_hi = true;
	    bool  ready_lo = true;
	    bool  branch   = (inst->attr & BRANCH);
	    if (inst->attr & READ_RS) {
		ready_rs = RegAvailable(inst->rs, branch);
	    }
	    if (inst->attr & READ_RT) {
		ready_rt = RegAvailable(inst->rt, branch);
	    }
	    if (inst->attr & READ_HI) {
		ready_hi = RegAvailable(PIPE_REG_HI, branch);
	    }
	    if (inst->attr & READ_LO) {
		ready_lo = RegAvailable(PIPE_REG_LO, branch);
	    }
	    if (!ready_rs || !ready_rt || !ready_hi || !ready_lo) {
		break; /* Retry at next cycle. */
	    }
	    if (inst->attr & WRITE_RS) {
		reg_state[inst->rs].locked++;
	    }
	    if (inst->attr & WRITE_RT) {
		reg_state[inst->rt].locked++;
	    }
	    if (inst->attr & (WRITE_RD | WRITE_RD_COND)) {
		reg_state[inst->rd].locked++;
	    }
	    if (inst->attr & WRITE_RRA) {
		reg_state[REG_RA].locked++;
	    }
	    if (inst->attr & WRITE_HI) {
		reg_state[PIPE_REG_HI].locked++;
	    }
	    if (inst->attr & WRITE_LO) {
		reg_state[PIPE_REG_LO].locked++;
	    }
	    reg_state[0].locked = 0;
	    stage_state[SDECODE] = STAGE_STALL;
	}
    } break;
    case  STAGE_STALL:
	/* Nothing to do */
	break;
    default:
	fprintf(stderr, "Invalid State for Decode Stage!\n");
	abort();
    }
}

void
PipeLine::Exec()
{
    /* All instruction can be executed within one cycle. */
    switch (stage_state[SEXEC]) {
    case  STAGE_IDLE: {
	if (latches[SEXEC].contain) {
	    MipsInst*  inst = &latches[SEXEC].inst;
	    if (forwarding) {
		if (!(inst->attr & LOADSTORE)) {
		    if (inst->attr & WRITE_RS) {
			reg_state[inst->rs].ex_fw = true;
		    }
		    if (inst->attr & WRITE_RT) {
			reg_state[inst->rt].ex_fw = true;
		    }
		    if (inst->attr & (WRITE_RD | WRITE_RD_COND)) {
			reg_state[inst->rd].ex_fw = true;
		    }
		    if (inst->attr & WRITE_HI) {
			reg_state[PIPE_REG_HI].ex_fw = true;
		    }
		    if (inst->attr & WRITE_LO) {
			reg_state[PIPE_REG_LO].ex_fw = true;
		    }
		    if (inst->attr & WRITE_RRA) {
			reg_state[REG_RA].ex_fw = true;
		    }
		} else if ((inst->attr & LOAD_ANY)
			   && inst->rt != 0) {
		    // cancel forwarded data from a previous insn.
		    reg_state[inst->rt].load0_fw = true;
		}
		reg_state[0].ex_fw = 0;
	    }
	    stage_state[SEXEC] = STAGE_STALL;
	}
    } break;
    case  STAGE_STALL:
	/* Nothing to do */
	break;
    default:
	fprintf(stderr, "Invalid State for Execution Stage!\n");
	abort();
    }
}

void
PipeLine::Mem()
{
    switch (stage_state[SMEM]) {
    case  STAGE_IDLE: {
	if (latches[SMEM].contain) {
	    int  wait = 0;
	    MipsInst*  inst = &latches[SMEM].inst;
	    if (inst->attr & LOADSTORE) {
		if (dcache_enable) {
		    int rwtype = (inst->attr&LOAD_ANY)
			? Cache::CACHE_READ : Cache::CACHE_WRITE;
		    wait = dcache->Access(latches[SMEM].paddr, rwtype)-1;
		} else {
		    wait = 0;
		}
		if (inst->attr & WRITE_RT) {
		    if (forwarding && inst->rt != 0) {
			reg_state[inst->rt].load0_fw = false;
			reg_state[inst->rt].load_fw  = true;
		    }
		    reg_state[0].load_fw = false;
		}
	    } else { /* exec instructions */
		if (forwarding) {
		    if (inst->attr & WRITE_RS) {
			reg_state[inst->rs].ex_fw  = false;
			reg_state[inst->rs].ex2_fw = true;
		    }
		    if (inst->attr & WRITE_RT) {
			reg_state[inst->rt].ex_fw  = false;
			reg_state[inst->rt].ex2_fw = true;
		    }
		    if (inst->attr & (WRITE_RD | WRITE_RD_COND)) {
			reg_state[inst->rd].ex_fw  = false;
			reg_state[inst->rd].ex2_fw = true;
		    }
		    if (inst->attr & WRITE_HI) {
			reg_state[PIPE_REG_HI].ex_fw  = false;
			reg_state[PIPE_REG_HI].ex2_fw = true;
		    }
		    if (inst->attr & WRITE_LO) {
			reg_state[PIPE_REG_LO].ex_fw  = false;
			reg_state[PIPE_REG_LO].ex2_fw = true;
		    }
		    if (inst->attr & WRITE_RRA) {
			reg_state[REG_RA].ex_fw =  false;
			reg_state[REG_RA].ex2_fw = true;
		    }
		}
	    }
	    stage_wait_cycle[SMEM] = wait;
	    stage_state[SMEM] = (wait > 0) ? STAGE_BUSY : STAGE_STALL;
	}
    } break;
    case  STAGE_BUSY: {
	stage_wait_cycle[SMEM]--;
	if (stage_wait_cycle[SMEM] <= 0) {
	    stage_state[SMEM] = STAGE_STALL;
	}
    } break;
    case  STAGE_STALL:
	/* Nothing to do */
	break;
    default:
	fprintf(stderr, "Invalid State for Memory Stage!\n");
	abort();
    }
}

inline void
PipeLine::WriteBackReg(int reg)
{
#ifdef  DEBUG_PIPELINE
    fprintf(stderr, "WriteBack: %d\n", reg);
#endif
    if (reg == 0) {
	return;
    }
    reg_state[reg].locked--;
    assert(reg_state[reg].locked >= 0);
    if (forwarding) {
	reg_state[reg].ex2_fw  = false;
	reg_state[reg].load_fw = false;
    }
}

void
PipeLine::WriteBack()
{
    assert(stage_state[SWB] == STAGE_IDLE);
    if (latches[SWB].contain) {
	MipsInst*  inst = &latches[SWB].inst;
	latches[SWB].contain = false;

	if (inst->attr & WRITE_RS) {
	    WriteBackReg(inst->rs);
	}
	if (inst->attr & WRITE_RT) {
	    WriteBackReg(inst->rt);
	}
	if (inst->attr & (WRITE_RD | WRITE_RD_COND)) {
	    WriteBackReg(inst->rd);
	}
	if (inst->attr & WRITE_HI) {
	    WriteBackReg(PIPE_REG_HI);
	}
	if (inst->attr & WRITE_LO) {
	    WriteBackReg(PIPE_REG_LO);
	}
	if (inst->attr & WRITE_RRA) {
	    WriteBackReg(REG_RA);
	}
    }
}
