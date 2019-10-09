/**********************************************************************
 * SimMips: Simple Computer Simulator of MIPS    Arch Lab. TOKYO TECH *
 **********************************************************************/
#include "define.h"

#define MEM_HEADER "SimMips_Machine_Setting"

enum {
    SR_DEF       = 0x10000000,
    PAGEMASK_DEF = 0x00001fff,
    PRID_DEF     = 0x00018001,
    CONFIG_DEF   = 0x80000082,
    CONFIG1_DEF  = 0x1ed96c80,

    RUNNING = 0,
    HALT_CYCLE = 1,
    HALT_MIPS = 2,
    HALT_INT = 3,

    HEAD_SIZE = 128,
};

/**********************************************************************/
volatile sig_atomic_t recieve_int = 0;

void sigint_handler(int signum)
{
    recieve_int = 1;
}

/**********************************************************************/
ttyControl::ttyControl()
{
    struct termios newterm;
    int newfflg;

    tcgetattr(STDIN_FILENO, &oldterm);
    oldfflg = fcntl(STDIN_FILENO, F_GETFL);

    if ((oldterm.c_lflag & (ICANON | ECHO)) == 0) {
        valid = 0;
        return;
    }

    newterm = oldterm;
    newterm.c_lflag &= ~(ICANON | ECHO);
    newterm.c_cc[VMIN] = 0;
    newterm.c_cc[VTIME] = 0;
    newfflg = oldfflg | O_NONBLOCK;

    tcsetattr(STDIN_FILENO, TCSANOW, &newterm);
    fcntl(STDIN_FILENO, F_SETFL, newfflg);
    valid = 1;
}

/**********************************************************************/
ttyControl::~ttyControl()
{
    if (valid) {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldterm);
        fcntl(STDIN_FILENO, F_SETFL, oldfflg);
    }
}

/**********************************************************************/
Board::Board()
{
    debug_mode = multicycle = imix_mode = use_cp0 = use_ttyc = 0;
    maxcycle = MAX_CYCLE_DEF;
    binfile = memfile = NULL;
    ttyc = NULL;
    mmap = NULL;
}

/**********************************************************************/
Board::~Board()
{
    DELETE(ttyc);
    DELETE(chip);
    DELETE(mmap);
}

/**********************************************************************/
void Board::usage()
{
    char usagemessage[] = "\
  -e[num][kmg]: stop simulation after num cycles executed\n\
  -d[level]: debug mode\n\
  -i: put instruction mix after simulation\n\
  -m: use multi-cycle execution model\n\
  -M [filename]: specify machine setting file\n\
\n";

    printf("Usage: simmips [-options] object_file_name\n");
    printf("%s", usagemessage);
}

/**********************************************************************/
ullint Board::atoi_postfix(const char *nptr)
{
    char *endptr;
    ullint result = strtoll(nptr, &endptr, 10);
    if ((*endptr == 'k') || (*endptr == 'K'))
        result *= 1000;
    else if ((*endptr == 'm') || (*endptr == 'M'))
        result *= 1000000;
    else if ((*endptr == 'g') || (*endptr == 'G'))
        result *= 1000000000;
    return result;
}

/**********************************************************************/
void Board::checkarg(int argc, char **argv)
{
    int bin_index = -1;
    int num;
    char *opt;

    for (int i = 1; (opt = argv[i]) != NULL; i++) {
        if (opt[0] != '-') {
            if (bin_index != -1) {
                fprintf(stderr, "## multiple binary files(%s, %s)\n",
                        argv[bin_index], opt);
                usage();
                return;
            }
            bin_index = i;
            continue;
        }
        switch (opt[1]) {
        case 'd':
            num = atoi(&opt[2]);
            if (num > MAX_DEBUG_MODE) {
                fprintf(stderr, "## debug mode %d not available\n", num);
                return;
            }
            debug_mode = num;
            break;
        case 'e':
            maxcycle = atoi_postfix(&opt[2]);
            if (!maxcycle)
                maxcycle = MAX_CYCLE_DEF;
            break;
        case 'i':
            imix_mode = 1;
            break;
        case 'm':
            multicycle = 1;
            break;
        case 'M':
            if (memfile) {
                fprintf(stderr, "## multiple -M options\n");
                return;
            }
            if ((memfile = argv[++i]) == NULL) {
                fprintf(stderr, "## -M option: no file specified\n");
                return;
            }
            break;
        default:
            fprintf(stderr, "## -%c: invalid option\n", opt[1]);
            usage();
            return;
        }
    }
    if (bin_index == -1) {
        usage();
        return;
    }

    if (debug_mode)
        printf("## debug mode %d.\n", debug_mode);

    binfile = argv[bin_index];
}

/**********************************************************************/
int Board::loadrawfile(char *filename, uint032_t addr)
{
    FILE *fp;
    uint008_t temp;

    if ((fp = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "## can't open file: %s\n", filename);
        return 1;
    }
    while (fread(&temp, sizeof(temp), 1, fp)) {
        chip->mc->enqueue(addr, 1, &temp);
        chip->mc->step();
        addr++;
    }
    if (!feof(fp)) {
        fprintf(stderr, "## can't load file: %s\n", filename);
        return 1;
    }
    fclose(fp);
    return 0;
}

/**********************************************************************/
char* Board::getlinehead(char *dest, FILE *fp)
{
    memset(dest, '\0', HEAD_SIZE + 1);
    char *result = fgets(dest, HEAD_SIZE, fp);
    int len = strlen(dest);
    if (dest[len - 1] != '\n') {
        if (dest[len - 1] == '\r')
            dest[len - 1] = '\0';
        char poi[HEAD_SIZE];
        do
            if (fgets(poi, HEAD_SIZE, fp) == NULL)
                break;
        while (poi[strlen(poi) - 1] != '\n');
    } else {
        dest[len - 1] = '\0';
        if (dest[len - 2] == '\r')
            dest[len - 2] = '\0';
    }
    return result;
}

/**********************************************************************/
FILE *Board::openmemfile()
{
    FILE *fp;
    char line[HEAD_SIZE + 1];
    line[HEAD_SIZE] = '\0';

    if ((fp = fopen(memfile, "r")) == NULL) {
        fprintf(stderr, "## can't open file: %s\n", memfile);
        return NULL;
    }
    getlinehead(line, fp);
    if (strncmp(line, MEM_HEADER, strlen(MEM_HEADER)) != 0) {
        fprintf(stderr, "## not a machine setting file: %s\n", memfile);
        fclose(fp);
        return NULL;
    }
    return fp;
}

/**********************************************************************/
void Board::setdefaultmap()
{
    mmap = new MemoryMap();
    mmap->addr = 0;
    mmap->size = MEM_SIZE_DEF;
    mmap->dev = new MainMemory(MEM_SIZE_DEF);
}

/**********************************************************************/
int Board::setmemorymap()
{
    FILE *fp;    
    char line[HEAD_SIZE + 1];
    uint032_t addr, size;
    int linecnt = 1;
    char *endptr;
    MemoryMap *now = NULL;
    line[HEAD_SIZE] = '\0';

    if (!memfile) {
        setdefaultmap();
        return 0;
    }
    if ((fp = openmemfile()) == NULL)
        return 1;

    while (getlinehead(line, fp) != NULL) {
        linecnt++;
        if (strncmp(line, "@map", 4))
            continue;

        addr = strtoul(&line[5], &endptr, 16);
        size = strtoul(&endptr[1], &endptr, 16);
        if (!size) {
            fprintf(stderr, "## %s:%d: invalid syntax. skip.\n",
                    memfile, linecnt);
            continue;
        }
        if (!now)
            now = (mmap = new MemoryMap());
        else
            now = (now->next = new MemoryMap());
        now->addr = addr;
        now->size = size;

        if (strcmp(&endptr[1], "MAIN_MEMORY") == 0) {
            now->dev = new MainMemory(size);
        } else if (strcmp(&endptr[1], "ISA_IO") == 0) {
            now->dev = new IsaIO();
            use_cp0 = use_ttyc = 1;
        } else if (strcmp(&endptr[1], "ISA_BUS") == 0) {
            now->dev = new MMDevice();
        } else if (strcmp(&endptr[1], "MIERU_IO") == 0) {
            now->dev = new MieruIO();
            use_ttyc = 1;
        }
    }
    if (!now)
        setdefaultmap();
    fclose(fp);
    return 0;
}

/**********************************************************************/
int Board::setinitialdata()
{
    FILE *fp;
    char line[HEAD_SIZE + 1];
    int linecnt = 1, regnum;
    uint032_t addr, regval;
    char *endptr;
    line[HEAD_SIZE] = '\0';

    if (!memfile)
        return 0;

    if ((fp = openmemfile()) == NULL)
        return 1;

    while (getlinehead(line, fp) != NULL) {
        linecnt++;
        if (line[0] != '@')
            continue;

        if (strncmp(&line[1], "reg", 3) == 0) {
            regnum = strtol(&line[6], &endptr, 10);
            if (!regnum)
                for (int i = 1; i < NREG + 1; i++)
                    if (strncmp(&line[6], regname[i], 
                                strlen(regname[i])) == 0) {
                        regnum = i;
                        endptr = &line[6 + strlen(regname[i])];
                        break;
                    }
            if (endptr[0] != '=') {
                fprintf(stderr, "## %s:%d: invalid syntax. skip.\n",
                        memfile, linecnt);
                continue;
            } else {
                regval = strtoul(&endptr[1], NULL, 0);
            }   
            if ((!regnum) && (regnum >= NREG + 1)) {
                fprintf(stderr, "## %s:%d: invalid register. skip.\n",
                        memfile, linecnt);
            } else if (regnum != NREG) {
                chip->mips->as->r[regnum] = regval;
            } else {
                chip->mips->as->pc = regval;
            }
        } else if (strncmp(&line[1], "mem", 3) == 0) {
            addr = strtoul(&line[5], &endptr, 16);
            loadrawfile(&endptr[1], addr);
        } else if (strncmp(&line[1], "map", 3) != 0) {
            fprintf(stderr, "## %s:%d: unknown command. skip.\n",
                    memfile, linecnt);
        }
    }
    fclose(fp);
    return 0;
}

/**********************************************************************/
ullint Board::gettime()
{
    static ullint start = 0;
    ullint tm;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    tm = tv.tv_sec * 1000000ull + tv.tv_usec;
    if (start == 0)
        start = tm;
    return tm - start;
}

/**********************************************************************/
int Board::siminit(char *arg)
{
    if (arg == NULL)
        return 1;
    if (arg[0] == '\0')
        return 1;

    int num, argc = 0;
    int arglen = strlen(arg);
    char *head = new char[arglen + 1];
    memset(head, 0, arglen + 1);
    char *temp = new char[arglen + 1];
    strcpy(temp, arg);
    char flag = ' ';

    for (int i = 0; i < arglen; i++) {
        if ((temp[i] == '"') || (temp[i] == '\'')) {
            if (flag == temp[i]) {
                temp[i] = '\0';
                flag = ' ';
            } else if ((flag != '"') && (flag != '\'')) {
                head[i + 1] = 1;
                argc++;
                flag = temp[i];
            }
        } else if ((temp[i] == ' ') && (flag == '\0')) {
            temp[i] = '\0';
            flag = ' ';
        } else if ((temp[i] != ' ') && (flag == ' ')) {
            head[i] = 1;
            argc++;
            flag = '\0';
        }
    }
    if ((flag == '"') || (flag == '\'')) {
        fprintf(stderr, "## checkarg(): syntax error\n");
        return 1;
    }
    char **argv = new char*[argc + 2];
    num = 1;
    for (int i = 0; i < arglen; i++) {
        if (head[i]) {
            argv[num] = &temp[i];
            num++;
        }
    }
    argv[0] = argv[argc + 1] = NULL;

    int ret = siminit(argc, argv);

    DELETE_ARRAY(head);
    DELETE_ARRAY(temp);
    DELETE_ARRAY(argv);
    return ret;
}

/**********************************************************************/
int Board::siminit(int argc, char **argv)
{
    struct sigaction sa;

    checkarg(argc, argv);
    if (!binfile)
        return 1;
    if (setmemorymap())
        return 1;
    chip = new Chip(this, use_cp0, multicycle);

    // load ELF image
    SimLoader *ld = new SimLoader();
    if (ld->loadfile(binfile))
        return 1;
    if ((ld->archtype != EM_MIPS) ||
        (ld->filetype != ET_EXEC)) { // || (ld->dynamic)) {
        printf("## ERROR: inproper binary: %s\n", binfile);
        return 1;
    }

    // set registers' default value
    chip->mips->as->pc = ld->entry;
    chip->mips->as->r[REG_SP] = ((ld->stackptr) ? ld->stackptr :
                                 MEM_SIZE_DEF - 0x100);
    if (!use_cp0)
        chip->mips->as->r[REG_T9] = chip->mips->as->pc;
    for (int i = 0; i < ld->symtabnum; i++)
        if (strcmp(ld->symtab[i].name, "_gp") == 0) {
            chip->mips->as->r[REG_GP] = ld->symtab[i].addr;
            break;
        }
    if (use_cp0) {
        chip->cp0->writereg(CP0_SR______, SR_DEF);
        chip->cp0->writereg(CP0_PAGEMASK, PAGEMASK_DEF);
        chip->cp0->writereg(CP0_PRID____, PRID_DEF);
        chip->cp0->writereg(CP0_CONFIG__, CONFIG_DEF);
        chip->cp0->writereg(CP0_CONFIG1_, CONFIG1_DEF);
    }

    if (setinitialdata())
        return 1;

    // write ELF image to memory
    if (use_cp0) {
        for (int i = 0; i < ld->memtabnum; i++) {
            if ((ld->memtab[i].addr <  KSEG0_MIN) ||
                (ld->memtab[i].addr >= KSEG2_MIN)) {
                printf("## ERROR: load to unmapped segment: 0x%08x\n",
                       ld->memtab[i].addr);
                return 1;
            } else {
                ld->memtab[i].addr &= UNMAP_MASK;
            }
        }
    }
    for (int i = 0; i < ld->memtabnum; i++) {
        chip->mc->enqueue(ld->memtab[i].addr, 1, &ld->memtab[i].data);
        chip->mc->step();
    }
    DELETE(ld);

    // set up console and signal
    if (use_ttyc)
        ttyc = new ttyControl();
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    for (MemoryMap *temp = mmap; temp != NULL; temp = temp->next)
        temp->dev->init(this);

    // now, the chip is ready for execution
    chip->maxcycle = maxcycle;
    chip->mips->state = CPU_START;
    chip->ready = 1;
    return 0;
}

/**********************************************************************/
void Board::exec()
{
    gettime();
    if (multicycle) {
        while (chip->getstate() == RUNNING)
            chip->step_multi();
    } else {
        while (chip->getstate() == RUNNING)
            chip->step_funct();
    }
    printresult();
}

/**********************************************************************/
void Board::printresult() {
    double simtime = (double) gettime() / 1000000.0;

    for (MemoryMap *temp = mmap; temp != NULL; temp = temp->next)
        temp->dev->fini();

    if      (chip->getstate() == HALT_CYCLE)
        printf("\n## cycle count reaches the limit\n");
    else if (chip->getstate() == HALT_MIPS)
        printf("\n## cpu stopped\n");
    else // (recieve_int)
        printf("\n## interrupt\n");
    printf("## cycle count: %llu\n", chip->cycle);
    printf("## inst count: %llu\n", chip->mips->ss->inst_count);
    printf("## simulation time: %8.3f\n", simtime);
    printf("## mips: %4.3f\n\n", 
           chip->mips->ss->inst_count / (simtime * 1000000.0));

    if (debug_mode == DEB_RESULT) {
        chip->mips->as->print();
        if (chip->cp0)
            chip->cp0->print();
        chip->mc->print();
    }
    if (imix_mode)
        chip->mips->ss->print();
}

/**********************************************************************/
Chip::Chip(Board *board, int use_cp0, int multi)
{
    cycle = 0;
    ready = 0;

    this->board = board;
    this->mmap = board->mmap;
    if (use_cp0)
        cp0 = new MipsCp0(board, this, (multi) ? 3 : 1);
    else
        cp0 = NULL;
    mc = new MemoryController(mmap,
                              (multi) ? MC_BUFFERMODE : MC_THROUGHMODE);
    mips = new Mips(board, this);
}

/**********************************************************************/
Chip::~Chip()
{
    DELETE(mips);
    DELETE(mc);
    DELETE(cp0);
}

/**********************************************************************/
int Chip::step_funct()
{
    cycle++;
    int ret = mips->step_funct();
    if (cp0)
        cp0->step();
    for (MemoryMap *temp = mmap; temp != NULL; temp = temp->next)
        temp->dev->step();
    return ret;
}

/**********************************************************************/
int Chip::step_multi()
{
    cycle++;
    int ret = mips->step_multi();
    if (cp0)
        cp0->step();
    for (MemoryMap *temp = mmap; temp != NULL; temp = temp->next)
        temp->dev->step();
    mc->step();
    return ret;
}

/**********************************************************************/
int Chip::getstate()
{
    if (cycle >= maxcycle)
        return HALT_CYCLE;
    else if (!mips->running())
        return HALT_MIPS;
    else if (recieve_int)
        return HALT_INT;
    else
        return RUNNING;
}

/**********************************************************************/
