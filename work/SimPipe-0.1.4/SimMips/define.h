/**********************************************************************
 * SimMips: Simple Computer Simulator of MIPS    Arch Lab. TOKYO TECH *
 **********************************************************************/
#define L_NAME "SimMips: Simple Computer Simulator of MIPS"
#define L_VER "Version 0.5.3 2009-02-18"

/**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#if __APPLE__
#include "elf.h"
#else
#include <elf.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>

/**********************************************************************/
typedef unsigned int uint;
typedef signed long long llint;
typedef unsigned long long ullint;

typedef signed char        int008_t;
typedef signed short       int016_t;
typedef signed int         int032_t;
typedef signed long long   int064_t;
typedef unsigned char      uint008_t;
typedef unsigned short     uint016_t;
typedef unsigned int       uint032_t;
typedef unsigned long long uint064_t;

/**********************************************************************/
#define DELETE(obj) {delete obj; obj = NULL;}
#define DELETE_ARRAY(arr) {delete[] arr; arr = NULL;}

/**********************************************************************/
enum {
    CPU_STOP = 0,
    CPU_START = 1,
    CPU_IF = 1,
    CPU_ID = 2,
    CPU_RF = 3,
    CPU_EX = 4,
    CPU_MS = 5,
    CPU_MR = 6,
    CPU_WB = 7,
    CPU_WAIT = 100,
    CPU_ERROR = -1,

    MAX_CYCLE_DEF = 0x7fffffffffffffffull,
    MAX_DEBUG_MODE = 4,
    DEB_RESULT = 1,
    DEB_INST   = 2,
    DEB_REG    = 3,
    DEB_EXCTLB = 4,

    NREG = 32,
    NCREG = 256,
    TLB_ENTRY = 16,
    MNEMONIC_BUF_SIZE = 128,
    INST_CODE_NUM = 107,

    MEM_SIZE_DEF = 0x08000000, // 128MiB
    PAGE_SIZE =  0x00001000,   //   4KiB
    KSEG0_MIN =  0x80000000,
    KSEG1_MIN =  0xa0000000,
    KSEG2_MIN =  0xc0000000,
    UNMAP_MASK = 0x1fffffff,
};

/* board.cc ***********************************************************/
class Chip;
class Mips;
class MipsCp0;
class MainMemory;
class MemoryController;
class MemoryMap;

/**********************************************************************/
class ttyControl {
 private:
    struct termios oldterm;
    int oldfflg;
    int valid;
 public:
    ttyControl();
    ~ttyControl();
};

/**********************************************************************/
class Board {
 private:
    ullint maxcycle;
    char *binfile, *memfile;
    ttyControl *ttyc;

    void usage();
    ullint atoi_postfix(const char *);
    void checkarg(int, char**);
    int loadrawfile(char *, uint032_t);
    char *getlinehead(char *, FILE *);
    FILE *openmemfile();
    void setdefaultmap();
    int setmemorymap();
    int setinitialdata();
    void printresult();
    
 public:
    int debug_mode, imix_mode, multicycle, use_cp0, use_ttyc;
    Chip *chip;
    MemoryMap *mmap;

    Board();
    ~Board();
    ullint gettime();
    int siminit(char *);
    int siminit(int, char **);
    void exec();
};

/**********************************************************************/
class Chip {
 private:
    Board *board;
    MemoryMap *mmap;

 public:
    ullint cycle, maxcycle;
    int ready;

    Mips *mips;
    MipsCp0 *cp0;
    MemoryController *mc;

    Chip(Board *, int, int);
    ~Chip();

    int step_funct();
    int step_multi();
    int getstate();
};

/* mipsinst.cc ********************************************************/
class MipsInst {
 private:
    char mnemonic[MNEMONIC_BUF_SIZE];
    const char *instname;

 public:
    uint032_t ir, op, pc;
    uint attr;
    uint opcode, rs, rt, rd, shamt, funct, imm, addr;
    uint code_l, code_s, sel;
    uint latency;
    
    MipsInst();
    void decode();
    const char *getinstname();
    char *getmnemonic();
    void clearmnemonic();
};

/* mips.cc ************************************************************/
class MipsArchstate {
 public:
    uint032_t pc, delay_npc;
    uint032_t r[NREG];
    uint032_t hi, lo;

    MipsArchstate();
    void print();
};

/**********************************************************************/
class MipsSimstate {
 public:
    ullint inst_count;
    ullint imix[INST_CODE_NUM];

    MipsSimstate();
    void print();
};

/**********************************************************************/
class Mips {
 private:
    Board *board;
    Chip *chip;
    MemoryController *mc;
    MipsCp0 *cp0;
 
    uint032_t rrs, rrt, rrd, rhi, rlo;
    uint032_t npc, vaddr;
    uint064_t paddr;
    int cond;
    int mcid;
    int exc_occur, exc_code;
    uint wait_cycle;

    void fetch();
    void decode();
    void regfetch();
    void execute();
    void memsend();
    void memreceive();
    void writeback();
    void setnpc();
    void exception(int);
    void proceedstate();
    void syscall();

 public:
    MipsArchstate *as;
    MipsSimstate *ss;
    MipsInst *inst;
    int state;
    
    Mips(Board *, Chip *);
    ~Mips();

    int step_funct();
    int step_multi();
    int running();
    inline uint064_t get_paddr() const { return paddr; }
};

/* cp0.cc *************************************************************/
class MipsTlbEntry {
 public:
    uint vpn2;
    uint asid;
    uint pagemask;
    uint pageshift;
    uint global;
    uint pfn[2];
    uint valid[2];
    uint dirty[2];
    uint cache[2];

    MipsTlbEntry();
    void print();
};

/**********************************************************************/
class MipsCp0 {
 private:
    Board *board;
    Chip *chip;
    MipsTlbEntry tlb[TLB_ENTRY];
    uint032_t r[NCREG];
    int counter, divisor;
    int gettlbentry(uint032_t);
    void regprint();
    void tlbprint();

 public:
    MipsCp0(Board *, Chip *, int);
    void step();
    void tlbread();
    void tlbwrite(int);
    void tlblookup();
    uint032_t readreg(int);
    void writereg(int, uint032_t);
    void modifyreg(int, uint032_t, uint032_t);
    int getphaddr(uint032_t, uint064_t *, int);
    uint032_t doexception(int, uint032_t, uint032_t, int);
    void setinterrupt(int);
    void clearinterrupt(int);
    int checkinterrupt();
    void print();
};

/* device.cc **********************************************************/
class MMDevice {
 public:
    virtual ~MMDevice() {}

    virtual void init(Board *) {}
    virtual void fini() {}
    virtual void step() {}
    virtual void read1b(const uint032_t, uint008_t*) {}
    virtual void read2b(const uint032_t, uint016_t*) {}
    virtual void read4b(const uint032_t, uint032_t*) {}
    virtual void read8b(const uint032_t, uint064_t*) {}
    virtual void write1b(const uint032_t, const uint008_t) {}
    virtual void write2b(const uint032_t, const uint016_t) {}
    virtual void write4b(const uint032_t, const uint032_t) {}
    virtual void write8b(const uint032_t, const uint064_t) {}
    virtual void print() {}
};

/**********************************************************************/
class IntController {
 private:
    MipsCp0 *cp0;
    uint008_t imr[2], irr[2], isr[2];
    int tobe_read[2], init_mode[2];

    int checkaddr(uint032_t);
    void recalcirq();

 public:
    IntController(MipsCp0 *);

    void read1b(const uint032_t, uint008_t *);
    void write1b(const uint032_t, const uint008_t);
    void setinterrupt(int);
    void clearinterrupt(int);
};

/**********************************************************************/
class SerialIO {
 private:
    IntController *pic;
    uint008_t ier, iir, lcr, mcr, scr;
    int currentchar;
    uint divisor;
    int counter;

    int charavail();
    void recalcirq();

 public:
    SerialIO(IntController *);

    void step();
    void read1b(const uint032_t, uint008_t *);
    void write1b(const uint032_t, const uint008_t);
};

/**********************************************************************/
class IsaIO : public MMDevice {
 private:
    IntController *pic;
    SerialIO *sio;
 public:
    IsaIO();
    ~IsaIO();
    
    void init(Board *);
    void step();
    void read1b(const uint032_t, uint008_t *);
    void write1b(const uint032_t, const uint008_t);
};

/**********************************************************************/
class MieruIO : public MMDevice {
 private:
    Board *board;
    int lcd_width, lcd_height, cursorx, cursory;
    char lcdbuf[100];
    int lcdindex;

    void lcd_ttyopen();
    void lcd_ttyclose();
    void lcd_setcolor(int);
    void lcd_cls();
    int lcd_printf(char *, ...);
    void lcd_nextline();

 public:
    void init(Board *);
    void fini();
    void read1b(const uint032_t, uint008_t *);
    void read4b(const uint032_t, uint032_t *);
    void write1b(const uint032_t, const uint008_t);
    void write4b(const uint032_t, const uint032_t);
};

/* memory.cc **********************************************************/
class MainMemory : public MMDevice {
 private:
    uint032_t mem_size, npage;
    uint032_t **pagetable;
    int *external;
    uint032_t *newpage(const uint032_t);
    uint032_t *getrealaddr(const uint032_t);
    
 public:
    MainMemory(uint032_t);
    ~MainMemory();
    void read1b(const uint032_t, uint008_t*);
    void read2b(const uint032_t, uint016_t*);
    void read4b(const uint032_t, uint032_t*);
    void read8b(const uint032_t, uint064_t*);
    void readnb(const uint032_t, int, uint008_t*);
    void write1b(const uint032_t, const uint008_t);
    void write2b(const uint032_t, const uint016_t);
    void write4b(const uint032_t, const uint032_t);
    void write8b(const uint032_t, const uint064_t);
    void writenb(const uint032_t, int, uint008_t*);
    uint032_t *setpageentry(const uint032_t, uint032_t*);
    void print();
};

/**********************************************************************/
enum {
    NUM_MCINST = 2,
    MC_BUFFERMODE = 0,
    MC_THROUGHMODE = 1,

    MCI_FAILURE = -1,
    MCI_NONE = 0,
    MCI_PEND = 1,
    MCI_FINISH = 2,
};
/**********************************************************************/
class MemoryMap {
 public:
    uint032_t addr;
    uint032_t size;
    MMDevice *dev;
    MemoryMap *next;

    MemoryMap();
    ~MemoryMap(); 
};

/**********************************************************************/
class McInst {
 public:
    int state;
    int op;
    uint064_t addr;
    uint032_t size;
    uint008_t data008;
    uint016_t data016;
    uint032_t data032;
    uint064_t data064;

    McInst();
};

/**********************************************************************/
class MemoryController {
 private:
    MemoryMap *mmap;
    int mode;
    int head, tail;
    
 public:
    McInst inst[NUM_MCINST];

    MemoryController(MemoryMap *, int);
    int enqueue(uint064_t, uint032_t, void *);
    void step();
    void print();
};

/* simloader.cc *******************************************************/
typedef struct {
    uint032_t addr;
    uint008_t data;
} memtab_t;

/**********************************************************************/
typedef enum {
    ST_NOTYPE,
    ST_FUNC,
    ST_OBJECT,
} symtype_t;

/**********************************************************************/
typedef struct {
    uint032_t addr;
    symtype_t type;
    char *name;
} symtab_t;

/**********************************************************************/
class SimLoader {
 public:
    memtab_t *memtab;
    symtab_t *symtab;
    
    int fileident;
    int filetype;
    int endian;
    int archtype;
    int dynamic;
    uint032_t entry;
    uint032_t stackptr;
    int memtabnum;
    int symtabnum;
    
    SimLoader();
    ~SimLoader();
    int loadfile(char*);
    int checkfile(const char *);
    int loadelf32(const char *);
};

/* Exception Code Definition ******************************************/
/**********************************************************************/
enum {
    EXC_INT____ = 0,  // Interupt
    EXC_MOD____ = 1,  // Modifying read-only page
    EXC_TLBL___ = 2,  // TLB miss load
    EXC_TLBS___ = 3,  // TLB miss store
    EXC_ADEL___ = 4,  // Address error load
    EXC_ADES___ = 5,  // Address error store
    EXC_IBE____ = 6,  // Bus error inst
    EXC_DBE____ = 7,  // Bus error data
    EXC_SYSCALL = 8,  // System call
    EXC_BP_____ = 9,  // Breakpoint
    EXC_RI_____ = 10, // Illegal instruction
    EXC_CPU____ = 11, // Coprocessor unavailable
    EXC_OV_____ = 12, // Overflow
    EXC_TRAP___ = 13, // Trap instruction
    // # Codes below are NOT architectually defined
    EXC_TLBREFL = 0x100, // TLB Refill flag (for TLBL, TLBS)
    EXC_CPU1___ = 0x200, // CP1 unavailable? (for CPU)
};

/* MIPS Register Name Definition **************************************/
/**********************************************************************/
enum {
    REG_V0 = 2,
    REG_A0 = 4,
    REG_A1 = 5,
    REG_A2 = 6,
    REG_A3 = 7,
    REG_T9 = 25,
    REG_GP = 28,
    REG_SP = 29,
    REG_RA = 31,
};

/**********************************************************************/
const char regname[NREG + 1][3] =
    {"zr", "at", "v0", "v1", "a0", "a1", "a2", "a3", 
     "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", 
     "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", 
     "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra", "pc"};

/* CP0 Register Definition ********************************************/
/**********************************************************************/
enum {
    CP0_INDEX___ = 0,
    CP0_RANDOM__ = 1,
    CP0_ENTRYLO0 = 2,
    CP0_ENTRYLO1 = 3,
    CP0_CONTEXT_ = 4,
    CP0_PAGEMASK = 5,
    CP0_WIRED___ = 6,
    CP0_BADVADDR = 8,
    CP0_COUNT___ = 9,
    CP0_ENTRYHI_ = 10,
    CP0_COMPARE_ = 11,
    CP0_SR______ = 12,
    CP0_CAUSE___ = 13,
    CP0_EPC_____ = 14,
    CP0_PRID____ = 15,
    CP0_CONFIG__ = 16,
    CP0_CONFIG1_ = 48,

    SR_EXL_SH = 1,
    SR_KSU_SH = 3,
    SR_BEV_SH = 22,
    SR_EXL_MASK = 0x1,
    SR_ERLEXL_MASK = 0x3,
    SR_KSU_MASK = 0x3,
    SR_BEV_MASK = 0x1,
    SR_IE_MASK = 0x1,

    CAUSE_EXC_SH = 2,
    CAUSE_IP_SH = 8,
    CAUSE_IV_SH = 23,
    CAUSE_CE_SH = 28,
    CAUSE_BD_SH = 31,
    CAUSE_EXC_MASK = 0x1f,
    CAUSE_IP_MASK = 0xff,
    CAUSE_IV_MASK = 0x1,
    CAUSE_CE_MASK = 0x3,
    CAUSE_BD_MASK = 0x1,

    TLB_VPAGE_SH = 13,
    TLB_VPAGE_LOWER = 0x1fff,
    TLB_VPAGE_MASK = 0x7ffff,
    TLB_PPAGE_SH = 12,
    CONT_BADV_SH = 4,
    CONT_BADV_MASK = TLB_VPAGE_MASK,
};


/* System Call Definition *********************************************/
/**********************************************************************/
enum {
    SYS_EXIT  = 4001,
    SYS_WRITE = 4004,
    SYS_IOCTL = 4054,
};

/* Instruction Definition *********************************************/
/**********************************************************************/
enum {
    NOP______,
    SSNOP____,
    SLL______,
    SRL______,
    SRA______,
    SLLV_____,
    SRLV_____,
    SRAV_____,
    JR_______,
    JR_HB____,
    JALR_____,
    JALR_HB__,
    MOVZ_____,
    MOVN_____,
    SYSCALL__,
    BREAK____,
    SYNC_____,
    MFHI_____,
    MTHI_____,
    MFLO_____,
    MTLO_____,
    MULT_____,
    MULTU____,
    DIV______,
    DIVU_____,
    ADD______,
    ADDU_____,
    SUB______,
    SUBU_____,
    AND______,
    OR_______,
    XOR______,
    NOR______,
    SLT______,
    SLTU_____,
    TGE______,
    TGEU_____,
    TLT______,
    TLTU_____,
    TEQ______,
    TNE______,
    BLTZ_____,
    BGEZ_____,
    BLTZL____,
    BGEZL____,
    TGEI_____,
    TGEIU____,
    TLTI_____,
    TLTIU____,
    TEQI_____,
    TNEI_____,
    BLTZAL___,
    BGEZAL___,
    BLTZALL__,
    BGEZALL__,
    J________,
    JAL______,
    BEQ______,
    BNE______,
    BLEZ_____,
    BGTZ_____,
    ADDI_____,
    ADDIU____,
    SLTI_____,
    SLTIU____,
    ANDI_____,
    ORI______,
    XORI_____,
    LUI______,
    MFC0_____,
    CFC0_____,
    MTC0_____,
    TLBR_____,
    TLBWI____,
    TLBWR____,
    TLBP_____,
    ERET_____,
    WAIT_____,
    BEQL_____,
    BNEL_____,
    BLEZL____,
    BGTZL____,
    MADD_____,
    MADDU____,
    MUL______,
    MSUB_____,
    MSUBU____,
    CLZ______,
    CLO______,
    LB_______,
    LH_______,
    LWL______,
    LW_______,
    LBU______,
    LHU______,
    LWR______,
    SB_______,
    SH_______,
    SWL______,
    SW_______,
    SWR______,
    CACHE____,
    LL_______,
    PREF_____,
    SC_______,
    FLOAT_OPS,
    UNDEFINED,
};

/**********************************************************************/
enum {
    READ_NONE = 0x0,
    READ_RS = 0x1,
    READ_RT = 0x2,
    READ_RD = 0x4,
    READ_HI = 0x8,
    READ_LO = 0x10,
    READ_HILO = 0x18,
    WRITE_NONE = 0x0,
    WRITE_RS = 0x100,
    WRITE_RT = 0x200,
    WRITE_RD = 0x400,
    WRITE_HI = 0x800,
    WRITE_LO = 0x1000,
    WRITE_HILO = 0x1800,
    WRITE_RD_COND = 0x4000,
    WRITE_RRA = 0x8000,
    LOAD_1B = 0x10000,
    LOAD_2B = 0x20000,
    LOAD_4B_ALIGN = 0x40000,
    LOAD_4B_UNALIGN = 0x80000,
    LOAD_4B = 0xc0000,
    LOAD_ANY = 0xf0000,
    STORE_1B = 0x100000,
    STORE_2B = 0x200000,
    STORE_4B_ALIGN = 0x400000,
    STORE_4B_UNALIGN = 0x800000,
    STORE_4B = 0xc00000,
    STORE_ANY = 0xf00000,
    LOADSTORE = 0xff0000,
    LOADSTORE_4B_UNALIGN = 0x880000,
    BRANCH = 0x1000000,
    BRANCH_LIKELY = 0x2000000,
    BRANCH_ERET = 0x4000000, // BRANCH_NODELAY (eret only has)
};

/**********************************************************************/
