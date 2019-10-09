/**********************************************************************
 * SimMips: Simple Computer Simulator of MIPS    Arch Lab. TOKYO TECH *
 **********************************************************************/
#include "define.h"

#define SGN(x) ((x) & 0x80000000)
inline uint032_t exts32(uint032_t x, int y);

/**********************************************************************/
Mips::Mips(Board *board, Chip *chip)
{
    this->board = board;
    this->chip = chip;
    this->mc = chip->mc;
    this->cp0 = chip->cp0;
    as = new MipsArchstate();
    ss = new MipsSimstate();
    inst = new MipsInst();
    state = CPU_STOP;
    exc_occur = 0;
    wait_cycle = 0;
}

/**********************************************************************/
Mips::~Mips()
{
    DELETE(as);
    DELETE(inst);
}

/**********************************************************************/
int Mips::step_funct()
{
    if (!running())
        return (state != CPU_ERROR) ? 0 : -1;
    if (wait_cycle) {
        wait_cycle--;
        return 0;
    }

    uint064_t inst_last = ss->inst_count;
    if (cp0)
        if (cp0->checkinterrupt())
            exception(EXC_INT____);

    if (state == CPU_WAIT)
        return 0;

    fetch();
    decode();
    regfetch();
    execute();
    if (inst->attr & LOADSTORE) {
        memsend();
        memreceive();
    }
    writeback();
    setnpc();
    return (state != CPU_ERROR) ? 
        (int) (ss->inst_count - inst_last) : -1;
}

/**********************************************************************/
int Mips::step_multi()
{
    if (!running())
        return (state != CPU_ERROR) ? 0 : -1;
    if (wait_cycle) {
        wait_cycle--;
        return 0;
    }

    uint064_t inst_last = ss->inst_count;
    if (cp0)
        if (cp0->checkinterrupt())
            exception(EXC_INT____);

    if        (state == CPU_IF) {
        fetch();
    } else if (state == CPU_ID) {
        decode();
    } else if (state == CPU_RF) {
        regfetch();
    } else if (state == CPU_EX) {
        execute();
    } else if (state == CPU_MS) {
        memsend();
    } else if (state == CPU_MR) {
        memreceive();
    } else if (state == CPU_WB) {
        writeback();
        setnpc();
    }
    proceedstate();
    return (state != CPU_ERROR) ? 
        (int) (ss->inst_count - inst_last) : -1;
}

/**********************************************************************/
inline void Mips::fetch()
{
    if ((exc_occur) || (!running()))
        return;

    uint064_t addr = (uint064_t) as->pc;
    int ret;

    ss->inst_count++;
    inst->pc = as->pc;
    inst->clearmnemonic();
    if (cp0) {
        if ((ret = cp0->getphaddr(inst->pc, &addr, 0)) != 0) {
            vaddr = inst->pc;
            exception(ret);
            return;
        }
    }
    mcid = mc->enqueue(addr, 4, NULL);
    if (mcid < 0) {
        printf("## fetch failure 0x%08x\n", inst->pc);
        state = CPU_ERROR;
    }
}

/**********************************************************************/
inline void Mips::decode()
{
    if ((exc_occur) || (!running()))
        return;

    if (mc->inst[mcid].state == MCI_FAILURE) {
        printf("## fetch failure 0x%08x\n", inst->pc);
        state = CPU_ERROR;
        return;
    }
    inst->ir = mc->inst[mcid].data032;
    inst->decode();

    if ((board->debug_mode == DEB_INST) || 
        (board->debug_mode == DEB_REG))
        printf("[%10lld] %08x: %s\n",
               ss->inst_count, inst->pc, inst->getmnemonic());

    if (board->imix_mode)
        ss->imix[inst->op]++;
}

/**********************************************************************/
inline void Mips::regfetch()
{
    if ((exc_occur) || (!running()))
        return;

    // NOTE: semantic 'if's are omitted for speedup
//  if (inst->attr & READ_RS)
        rrs = as->r[inst->rs];
//  if (inst->attr & READ_RT)
        rrt = as->r[inst->rt];
//  if (inst->attr & READ_RD)
        rrd = as->r[inst->rd];
//  if (inst->attr & READ_HI)
        rhi = as->hi;
//  if (inst->attr & READ_LO)
        rlo = as->lo;

    if (board->debug_mode == DEB_REG) {
        printf("              ");
        if ((inst->attr & READ_RS) && (inst->rs))
            printf("$%s>%08x ", regname[inst->rs], rrs);
        if ((inst->attr & READ_RT) && (inst->rt))
            printf("$%s>%08x ", regname[inst->rt], rrt);
        if ((inst->attr & READ_RD) && (inst->rd))
            printf("$%s>%08x ", regname[inst->rd], rrd);
        if (inst->attr & READ_HI)
            printf("$hi>%08x ", rhi);
        if (inst->attr & READ_LO)
            printf("$lo>%08x ", rlo);
    }
}

/**********************************************************************/
inline void Mips::execute()
{
    if ((exc_occur) || (!running()))
        return;
    int ret;

    switch (inst->op) {
    case NOP______:
    case SSNOP____:
    case WAIT_____:
        // do nothing
        break;
    case SLL______:
        rrd = rrt << inst->shamt;
        break;
    case SRL______:
        rrd = rrt >> inst->shamt;
        break;
    case SRA______:
        rrd = exts32(rrt >> inst->shamt, 32 - inst->shamt);
        break;
    case SLLV_____:
        rrd = rrt << (rrs % 32);
        break;
    case SRLV_____:
        rrd = rrt >> (rrs % 32);
        break;
    case SRAV_____:
        rrd = exts32(rrt >> (rrs % 32), 32 - (rrs % 32));
        break;
    case JR_______:
    case JR_HB____:
        npc = rrs;
        cond = 1;
        break;
    case JALR_____:
    case JALR_HB__:
        rrd = inst->pc + 8;
        npc = rrs;
        cond = 1;
        break;
    case MOVZ_____:
        rrd = rrs;
        cond = (rrt == 0);
        break;
    case MOVN_____:
        rrd = rrs;
        cond = (rrt != 0);
        break;
    case SYSCALL__:
        if (cp0)
            exception(EXC_SYSCALL);
        else
            syscall();
        break;
    case BREAK____:
        exception(EXC_BP_____);
        break;
    case SYNC_____:
        // LD/ST barrier, nothing to do for functional simulator
        break;
    case MFHI_____:
        rrd = rhi;
        break;
    case MTHI_____:
        rhi = rrs;
        break;
    case MFLO_____:
        rrd = rlo;
        break;
    case MTLO_____:
        rlo = rrs;
        break;
    case MULT_____:
    {
        int064_t temp = (int064_t)(int032_t) rrs * (int032_t) rrt;
        rhi = (uint032_t) (temp >> 32);
        rlo = (uint032_t) (temp & 0xffffffff);
        break;
    }
    case MULTU____:
    {
        uint064_t temp = (uint064_t) rrs * (uint064_t) rrt;
        rhi = (uint032_t) (temp >> 32);
        rlo = (uint032_t) (temp & 0xffffffff);
        break;
    }
    case DIV______:
        if ((rrt != 0) && ((rrs != 0x80000000) || (rrt != 0xffffffff))) {
            rlo = (uint032_t) ((int032_t) rrs / (int032_t) rrt);
            rhi = (uint032_t) ((int032_t) rrs % (int032_t) rrt);
        } else {
            rlo = rhi = 0;
        }
        break;
    case DIVU_____:
        if (rrt != 0) {
            rlo = rrs / rrt;
            rhi = rrs % rrt;
        } else {
            rlo = rhi = 0;
        }
        break;
    case ADD______:
        rrd = rrs + rrt;
        if ((SGN(rrs) == SGN(rrt)) && (SGN(rrs) != SGN(rrd)))
            exception(EXC_OV_____);
        break;
    case ADDU_____:
        rrd = rrs + rrt;
        break;
    case SUB______:
        rrd = rrs - rrt;
        if ((SGN(rrs) != SGN(rrt)) && (SGN(rrs) != SGN(rrd)))
            exception(EXC_OV_____);
        break;        
    case SUBU_____:
        rrd = rrs - rrt;
        break;
    case AND______:
        rrd = rrs & rrt;
        break;
    case OR_______:
        rrd = rrs | rrt;
        break;
    case XOR______:
        rrd = rrs ^ rrt;
        break;
    case NOR______:
        rrd = ~(rrs | rrt);
        break;
    case SLT______:
        rrd = ((int032_t) rrs < (int032_t) rrt) ? 1 : 0;
        break;
    case SLTU_____:
        rrd = (rrs < rrt) ? 1 : 0;
        break;
    case TGE______:
        if ((int032_t) rrs >= (int032_t) rrt)
            exception(EXC_TRAP___);
        break;
    case TGEU_____:
        if (rrs >= rrt)
            exception(EXC_TRAP___);
        break;
    case TLT______:
        if ((int032_t) rrs < (int032_t) rrt)
            exception(EXC_TRAP___);
        break;
    case TLTU_____:
        if (rrs < rrt)
            exception(EXC_TRAP___);
        break;
    case TEQ______:
        if (rrs == rrt)
            exception(EXC_TRAP___);        
        break;
    case TNE______:
        if (rrs != rrt)
            exception(EXC_TRAP___);        
        break;
    case BLTZ_____:
    case BLTZL____:
    case BLTZAL___:
    case BLTZALL__:
        npc = inst->pc + (exts32(inst->imm, 16) << 2) + 4;
        cond = ((int032_t) rrs < 0);
        break;
    case BGEZ_____:
    case BGEZL____:
    case BGEZAL___:
    case BGEZALL__:
        npc = inst->pc + (exts32(inst->imm, 16) << 2) + 4;
        cond = ((int032_t) rrs >= 0);
        break;
    case TGEI_____:
        if ((int032_t) rrs >= (int032_t) exts32(inst->imm, 16))
            exception(EXC_TRAP___);
        break;
    case TGEIU____:
        if (rrs >= exts32(inst->imm, 16))
            exception(EXC_TRAP___);
        break;
    case TLTI_____:
        if ((int032_t) rrs < (int032_t) exts32(inst->imm, 16))
            exception(EXC_TRAP___);
        break;
    case TLTIU____:
        if (rrs < exts32(inst->imm, 16))
            exception(EXC_TRAP___);
        break;
    case TEQI_____:
        if (rrs == exts32(inst->imm, 16))
            exception(EXC_TRAP___);        
        break;
    case TNEI_____:
        if (rrs != exts32(inst->imm, 16))
            exception(EXC_TRAP___);        
        break;
    case J________:
    case JAL______:
        npc = (inst->pc & 0xf0000000) | (inst->addr << 2);
        cond = 1;
        break;
    case BEQ______:
    case BEQL_____:
        npc = inst->pc + (exts32(inst->imm, 16) << 2) + 4;
        cond = (rrs == rrt);
        break;
    case BNE______:
    case BNEL_____:
        npc = inst->pc + (exts32(inst->imm, 16) << 2) + 4;
        cond = (rrs != rrt);
        break;
    case BLEZ_____:
    case BLEZL____:
        npc = inst->pc + (exts32(inst->imm, 16) << 2) + 4;
        cond = ((int032_t) rrs <= 0);
        break;
    case BGTZ_____:
    case BGTZL____:
        npc = inst->pc + (exts32(inst->imm, 16) << 2) + 4;
        cond = ((int032_t) rrs > 0);
        break;
    case ADDI_____:
        rrt = rrs + exts32(inst->imm, 16);
        if ((SGN(rrs) == SGN(exts32(inst->imm, 16))) &&
            (SGN(rrt) != SGN(exts32(inst->imm, 16))))
            exception(EXC_OV_____);
        break;
    case ADDIU____:
        rrt = rrs + exts32(inst->imm, 16);
        break;
    case SLTI_____:
        rrt = ((int032_t) rrs < (int032_t) exts32(inst->imm, 16)) ? 1 : 0;
        break;
    case SLTIU____:
        rrt = (rrs < exts32(inst->imm, 16)) ? 1 : 0;
        break;
    case ANDI_____:
        rrt = rrs & inst->imm;
        break;
    case ORI______:
        rrt = rrs | inst->imm;
        break;
    case XORI_____:
        rrt = rrs ^ inst->imm;
        break;
    case LUI______:
        rrt = inst->imm << 16;
        break;
    case MFC0_____:
        if (cp0)
            rrt = cp0->readreg(inst->rd + inst->sel * 32);
        else
            rrt = 0;
        break;
    case CFC0_____:
        // cfc0 is defined but not used(really?)
        rrt = 0;
        break;
    case MTC0_____:
        if (cp0)
            cp0->writereg(inst->rd + inst->sel * 32, rrt);
        break;
    case TLBR_____:
        if (cp0)
        cp0->tlbread();
        break;
    case TLBWI____:
        if (cp0)
            cp0->tlbwrite(0);
        break;
    case TLBWR____:
        if (cp0)
            cp0->tlbwrite(1);
        break;
    case TLBP_____:
        if (cp0)
            cp0->tlblookup();
        break;
    case ERET_____:
        if (cp0) {
            npc = cp0->readreg(CP0_EPC_____);
            cond = 1;
        } else {
            cond = 0;
        }
        break;
    case MADD_____:
    {
        int064_t temp = ((int064_t) rhi << 32) | rlo;
        temp += (int064_t) rrs * (int064_t) rrt;
        rhi = (uint032_t) (temp >> 32);
        rlo = (uint032_t) (temp & 0xffffffff);
        break;
    }
    case MADDU____:
    {
        int064_t temp = ((int064_t) rhi << 32) | rlo;
        temp += (int064_t) rrs * (int064_t) rrt;
        rhi = (uint032_t) (temp >> 32);
        rlo = (uint032_t) (temp & 0xffffffff);
        break;
    }
    case MUL______:
    {
        int064_t temp = (int064_t) rrs * (int064_t) rrt;
        rrd = (uint032_t) (temp & 0xffffffff);
        break;
    }
    case MSUB_____:
    {
        int064_t temp = ((int064_t) rhi << 32) | rlo;
        temp -= (int064_t) rrs * (int064_t) rrt;
        rhi = (uint032_t) (temp >> 32);
        rlo = (uint032_t) (temp & 0xffffffff);
        break;
    }
    case MSUBU____:
    {
        int064_t temp = ((int064_t) rhi << 32) | rlo;
        temp -= (int064_t) rrs * (int064_t) rrt;
        rhi = (uint032_t) (temp >> 32);
        rlo = (uint032_t) (temp & 0xffffffff);
        break;
    }
    case CLZ______:
        for (rrd = 0; rrd < 32; rrd++)
            if (rrs & (0x80000000u >> rrd))
                break;
        break;
    case CLO______:
        for (rrd = 0; rrd < 32; rrd++)
            if (!(rrs & (0x80000000u >> rrd)))
                break;
        break;
    case LB_______:
    case LBU______:
    case LWL______:
    case LWR______:
        vaddr = rrs + exts32(inst->imm, 16);
        if (!cp0)
            paddr = (uint064_t) vaddr;
        else if ((ret = cp0->getphaddr(vaddr, &paddr, 0)) != 0)
            exception(ret);
        break;
    case SB_______:
    case SWL______:
    case SWR______:
        vaddr = rrs + exts32(inst->imm, 16);
        if (!cp0)
            paddr = (uint064_t) vaddr;
        else if ((ret = cp0->getphaddr(vaddr, &paddr, 1)) != 0)
            exception(ret);
        break;
    case LH_______:
    case LHU______:
        vaddr = rrs + exts32(inst->imm, 16);
        if (!cp0)
            paddr = (uint064_t) vaddr;
        else if (vaddr & 0x1)
            exception(EXC_ADEL___);
        else if ((ret = cp0->getphaddr(vaddr, &paddr, 0)) != 0)
            exception(ret);
        break;
    case SH_______:
        vaddr = rrs + exts32(inst->imm, 16);
        if (!cp0)
            paddr = (uint064_t) vaddr;
        else if (vaddr & 0x1)
            exception(EXC_ADEL___);
        else if ((ret = cp0->getphaddr(vaddr, &paddr, 1)) != 0)
            exception(ret);
        break;
    case LW_______:
    case LL_______:
        vaddr = rrs + exts32(inst->imm, 16);
        if (!cp0)
            paddr = (uint064_t) vaddr;
        else if (vaddr & 0x3)
            exception(EXC_ADEL___);
        else if ((ret = cp0->getphaddr(vaddr, &paddr, 0)) != 0)
            exception(ret);
        break;
    case SW_______:
    case SC_______:
        vaddr = rrs + exts32(inst->imm, 16);
        if (!cp0)
            paddr = (uint064_t) vaddr;
        else if (vaddr & 0x3)
            exception(EXC_ADEL___);
        else if ((ret = cp0->getphaddr(vaddr, &paddr, 1)) != 0)
            exception(ret);
        break;
    case CACHE____:
        // Nothing to do so far
        break;
    case PREF_____:
        // Prefetch instruction, not impremented
        break;
    case FLOAT_OPS:
        // floating operation, trapped if cp0 is enabled
        if (cp0)
            exception(EXC_CPU____ | EXC_CPU1___);
        else {
            printf("## Floating Instruction ! %08x: %08x\n",
                   inst->pc, inst->ir);
            state = CPU_ERROR;
        }
        break;
    default:
        printf("## Undefined Opcode ! %08x: %08x\n",
                inst->pc, inst->ir);
        state = CPU_ERROR;
        break;
    }

    wait_cycle = inst->latency - 1;
}

/**********************************************************************/
inline void Mips::memsend()
{
    if ((exc_occur) || (!running()))
        return;

    if (inst->attr & LOAD_1B) {
        mcid = mc->enqueue(paddr, 1, NULL);
    } else if (inst->attr & LOAD_2B) {
        mcid = mc->enqueue(paddr, 2, NULL);
    } else if (inst->attr & LOAD_4B_ALIGN) {
        mcid = mc->enqueue(paddr, 4, NULL);
    } else if (inst->attr & STORE_1B) {
        uint008_t temp = (uint008_t) rrt;
        mcid = mc->enqueue(paddr, 1, &temp);
    } else if (inst->attr & STORE_2B) {
        uint016_t temp = (uint016_t) rrt;
        mcid = mc->enqueue(paddr, 2, &temp);
    } else if (inst->attr & STORE_4B_ALIGN) {
        mcid = mc->enqueue(paddr, 4, &rrt);
    } else if (inst->attr & LOADSTORE_4B_UNALIGN) {
        mcid = mc->enqueue(paddr & ~0x3, 4, NULL);
    }
    if (mcid < 0)
        exception(EXC_DBE____);
}

/**********************************************************************/
inline void Mips::memreceive()
{
    if ((exc_occur) || (!running()))
        return;

    if (mc->inst[mcid].state == MCI_FAILURE) {
        exception(EXC_DBE____);
        return;
    }
    McInst *im = &mc->inst[mcid];

    if (inst->attr & LOAD_1B) {
        rrt = ((inst->op == LBU______) ? 
               im->data008 : exts32(im->data008, 8));
    } else if (inst->attr & LOAD_2B) {
        rrt = ((inst->op == LHU______) ? 
               im->data016 : exts32(im->data016, 16));
    } else if (inst->attr & LOAD_4B_ALIGN) {
        rrt = im->data032;
    } else if (inst->attr & LOAD_4B_UNALIGN) {
        if (inst->op == LWR______) {
            int shamt = (vaddr & 0x3) * 8;
            uint032_t mask = 0xffffffff >> shamt;
            rrt = ((im->data032 >> shamt) & mask) | (rrt & ~mask);
        } else {      // LWL______
            int shamt = 24 - (vaddr & 0x3) * 8;
            uint032_t mask = 0xffffffff << shamt;
            rrt = ((im->data032 << shamt) & mask) | (rrt & ~mask);
        }
    } else if (inst->attr & STORE_4B_UNALIGN) {
        // TODO: failure in store cannot be trapped 
        if (inst->op == SWR______) {
            int shamt = (vaddr & 0x3) * 8;
            uint032_t mask = 0xffffffff << shamt;
            uint032_t temp = (((rrt << shamt) & mask) |
                              (im->data032 & ~mask));
            mc->enqueue(paddr & ~0x3, 4, &temp);
        } else {     // SWL______
            int shamt = 24 - (vaddr & 0x3) * 8;
            uint032_t mask = 0xffffffff >> shamt;
            uint032_t temp = (((rrt >> shamt) & mask) |
                              (im->data032 & ~mask));
            mc->enqueue(paddr & ~0x3, 4, &temp);
        }
    } else if (inst->op == SC_______) {
        rrt = 1;
    }   
}

/**********************************************************************/
inline void Mips::writeback()
{
    if ((exc_occur) || (!running()))
        return;
    if (inst->attr & WRITE_RS)
        as->r[inst->rs] = rrs;
    if (inst->attr & WRITE_RT)
        as->r[inst->rt] = rrt;
    if (inst->attr & WRITE_RD)
        as->r[inst->rd] = rrd;
    if (inst->attr & WRITE_HI)
        as->hi = rhi;
    if (inst->attr & WRITE_LO)
        as->lo = rlo;
    if ((inst->attr & WRITE_RD_COND) && (cond))
        as->r[inst->rd] = rrd;
    if (inst->attr & WRITE_RRA)
        as->r[REG_RA] = inst->pc + 8;

    if (board->debug_mode == DEB_REG) {
        if ((inst->attr & WRITE_RS) && (inst->rs))
            printf("$%s<%08x ", regname[inst->rs], rrs);
        if ((inst->attr & WRITE_RT) && (inst->rt))
            printf("$%s<%08x ", regname[inst->rt], rrt);
        if ((inst->attr & WRITE_RD) && (inst->rd))
            printf("$%s<%08x ", regname[inst->rd], rrd);
        if (inst->attr & WRITE_HI)
            printf("$hi<%08x ", rhi);
        if (inst->attr & WRITE_LO)
            printf("$lo<%08x ", rlo);
        if ((inst->attr & WRITE_RD_COND) && (inst->rd) && (cond))
            printf("$%s<%08x ", regname[inst->rd], rrd);
        if (inst->attr & WRITE_RRA)
            printf("$ra<%08x ", inst->pc + 8);
        printf("\n");
    }

    as->r[0] = 0; // yes, register $zero is always zero
}

/**********************************************************************/
inline void Mips::setnpc()
{
    if (!running())
        return;

    if (exc_occur) {
        as->pc = cp0->doexception(exc_code, as->pc, 
                                  vaddr, (as->delay_npc) ? 1 : 0);
        as->delay_npc = 0;
    } else if (as->delay_npc) {
        as->pc = as->delay_npc;
        as->delay_npc = 0;
    } else if (((inst->attr & BRANCH) || (inst->attr & BRANCH_LIKELY))
               && (cond)) {
        as->pc += 4;
        as->delay_npc = npc;
        if (!npc) {
            printf("## Branch to zero. stop.\n");
            state = CPU_ERROR;
        }
    } else if ((inst->attr & BRANCH_ERET) && (cond)) {
        cp0->modifyreg(CP0_SR______, 0, 0x2);
        as->pc = npc;
        if (!npc) {
            printf("## Branch to zero. stop.\n");
            state = CPU_ERROR;
        }
    } else if ((inst->attr & BRANCH_LIKELY) && (!cond)) {
        as->pc += 8;
    } else {
        as->pc += 4;
    }

    if ((state != CPU_ERROR) && (inst->op == WAIT_____) && (!exc_occur))
        state = CPU_WAIT;

    exc_occur = 0;
}

/**********************************************************************/
void Mips::exception(int code)
{
    if (!cp0)
        return;
    if (exc_occur)
        return;

    exc_occur = 1;
    exc_code = code;
    if (state == CPU_WAIT)
        state = CPU_WB;
    if (board->debug_mode == DEB_EXCTLB)
        printf("## exception #%d (PC=0x%08x vaddr=0x%08x)\n",
               code, as->pc, vaddr);
}

/**********************************************************************/
void Mips::syscall()
{
    switch (as->r[REG_V0]) {
    case SYS_EXIT:
        state = CPU_STOP;
        break;
    case SYS_WRITE:
        if (as->r[REG_A0] == STDOUT_FILENO) {
            for (uint i = 0; i < as->r[REG_A2]; i++) {
                int mcid = mc->enqueue(as->r[REG_A1] + i, 1, NULL);
                if (mcid < 0)
                    break;
                mc->step();
                if (mc->inst[mcid].state == MCI_FINISH)
                    putchar((int) mc->inst[mcid].data008);
            }
        }
        as->r[REG_V0] = as->r[REG_A2];
        as->r[REG_A3] = 0;
        break;
    case SYS_IOCTL:
        as->r[REG_V0] = as->r[REG_A3] = 0;
        break;
    default:
        printf("## unknown syscall #%d (see asm/unistd.h). Skip.\n",
               as->r[REG_V0]);
        as->r[REG_V0] = as->r[REG_A3] = 0;
        break;
    }
}

/**********************************************************************/
void Mips::proceedstate()
{
    if (!running())
        return;
    else if (state == CPU_EX)
        state = (inst->attr & LOADSTORE) ? CPU_MS : CPU_WB;
    else if (state == CPU_WB)
        state = CPU_IF;
    else if (state != CPU_WAIT)
        state++;
}

/**********************************************************************/
int Mips::running()
{
    return (state > 0);
}

/**********************************************************************/
MipsArchstate::MipsArchstate()
{
    for (int i = 0; i < NREG; i++)
        r[i] = 0;
    hi = lo = 0;
    pc = delay_npc = 0;
}

/**********************************************************************/
void MipsArchstate::print()
{
    for (int i = 0; i < NREG / 8; i++) {
        for (int j = 0; j < 8; j++)
            printf("$%s       ", regname[i * 8 + j]);
        printf("\n");
        for (int j = 0; j < 8; j++)
            printf("%08x  ", r[i * 8 + j]);
        printf("\n");
    }
    printf("pc        hi        lo\n");
    printf("%08x  %08x  %08x\n\n", pc, hi, lo);
}

/*********************************************************************/
MipsSimstate::MipsSimstate()
{
    inst_count = 0;
    for (int i = 0; i < INST_CODE_NUM; i++)
        imix[i] = 0;
}

/*********************************************************************/
void MipsSimstate::print()
{
    ullint temp[INST_CODE_NUM];
    ullint max, lastmax;
    MipsInst *inst = new MipsInst();
    int num = 0, index = 0;

    memcpy(temp, imix, sizeof(imix));
    printf("[[Instruction Statistics]]\n");
    lastmax = 0;
    for (int i = 0; i < INST_CODE_NUM; i++) {
        max = 0;
        for (int j = 0; j < INST_CODE_NUM; j++)
            if (max < temp[j]) {
                max = temp[j];
                index = j;
            }
        if (max == 0)
            break;
        temp[index] = 0;
        if (max != lastmax) {
            lastmax = max;
            num = i + 1;
        }
        inst->op = index;
        
        printf("[%3d] %-9s%11lld (%7.3f%%)\n",
               num, inst->getinstname(), max,
               (double) max / inst_count * 100.0);
    }
    printf("[---] Total    %11lld\n\n", inst_count);
}

/**********************************************************************/
inline uint032_t exts32(uint032_t x, int y)
{
    if (y == 32)
        return x;
    uint032_t temp = 0xffffffff << y;
    if (x & (1 << (y - 1)))
        return (temp | (x & ~temp));
    else
        return (x & ~temp);
}

/*********************************************************************/
