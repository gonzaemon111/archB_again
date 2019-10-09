/**********************************************************************
 * SimMips: Simple Computer Simulator of MIPS    Arch Lab. TOKYO TECH *
 **********************************************************************/
#include "define.h"

inline uint032_t exts32(uint032_t x, int y);

/**********************************************************************/
MipsInst::MipsInst()
{
    ir = 0;
    op = UNDEFINED;
    clearmnemonic();
}

/**********************************************************************/
void MipsInst::decode()
{
    opcode = (ir >> 26) &      0x3f;
    rs     = (ir >> 21) &      0x1f;
    rt     = (ir >> 16) &      0x1f;
    rd     = (ir >> 11) &      0x1f;
    shamt  = (ir >>  6) &      0x1f;
    funct  =  ir        &      0x3f;
    imm    =  ir        &    0xffff;
    addr   =  ir        & 0x3ffffff;
    code_l = (ir >>  6) &   0xfffff;
    code_s = (ir >> 16) &     0x3ff;
    sel    =  ir        &       0x7;

    op = UNDEFINED;
    attr = READ_NONE | WRITE_NONE;
    latency = 1;

    switch (opcode) {
    case 0:
        switch (funct) {
        case 0:
            if ((rt | rd | shamt) == 0) {
                op = NOP______;
            } else if (((rt | rd) == 0) && (shamt == 1)) {
                op = SSNOP____;
            } else {
                op = SLL______;
                attr = READ_RT | WRITE_RD;
            }
            break;
        case 2:
            op = SRL______;
            attr = READ_RT | WRITE_RD;
            break;
        case 3:
            op = SRA______;
            attr = READ_RT | WRITE_RD;
            break;
        case 4:
            op = SLLV_____;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 6:
            op = SRLV_____;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 7:
            op = SRAV_____;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 8:
            if (shamt == 0) {
                op = JR_______;
                attr = BRANCH | READ_RS;
            } else if (shamt == 16) {
                op = JR_HB____;
                attr = BRANCH | READ_RS;
            }
            break;
        case 9:
            if (shamt == 0) {
                op = JALR_____;
                attr = BRANCH | READ_RS | WRITE_RD;
            } else if (shamt == 16) {
                op = JALR_HB__;
                attr = BRANCH | READ_RS | WRITE_RD;
            }
            break;
        case 10:
            op = MOVZ_____;
            attr = READ_RS | READ_RT | WRITE_RD_COND;
            break;
        case 11:
            op = MOVN_____;
            attr = READ_RS | READ_RT | WRITE_RD_COND;
            break;
        case 12:
            op = SYSCALL__;
            break;
        case 13:
            op = BREAK____;
            break;
        case 15:
            op = SYNC_____;
            break;
        case 16:
            op = MFHI_____;
            attr = READ_HI | WRITE_RD;
            break;
        case 17:
            op = MTHI_____;
            attr = READ_RS | WRITE_HI;
            break;
        case 18:
            op = MFLO_____;
            attr = READ_LO | WRITE_RD;
            break;
        case 19:
            op = MTLO_____;
            attr = READ_RS | WRITE_LO;
            break;
        case 24:
            op = MULT_____;
            attr = READ_RS | READ_RT | WRITE_HILO;
            break;
        case 25:
            op = MULTU____;
            attr = READ_RS | READ_RT | WRITE_HILO;
            break;
        case 26:
            op = DIV______;
            attr = READ_RS | READ_RT | WRITE_HILO;
            break;
        case 27:
            op = DIVU_____;
            attr = READ_RS | READ_RT | WRITE_HILO;
            break;
        case 32:
            op = ADD______;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 33:
            op = ADDU_____;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 34:
            op = SUB______;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 35:
            op = SUBU_____;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 36:
            op = AND______;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 37:
            op = OR_______;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 38:
            op = XOR______;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 39:
            op = NOR______;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 42:
            op = SLT______;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 43:
            op = SLTU_____;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 48:
            op = TGE______;
            attr = READ_RS | READ_RT;
            break;
        case 49:
            op = TGEU_____;
            attr = READ_RS | READ_RT;
            break;
        case 50:
            op = TLT______;
            attr = READ_RS | READ_RT;
            break;
        case 51:
            op = TLTU_____;
            attr = READ_RS | READ_RT;
            break;
        case 52:
            op = TEQ______;
            attr = READ_RS | READ_RT;
            break;
        case 54:
            op = TNE______;
            attr = READ_RS | READ_RT;
            break;
        }
        break;
    case 1:
        switch (rt) {
        case 0:
            op = BLTZ_____;
            attr = BRANCH | READ_RS;
            break;
        case 1:
            op = BGEZ_____;
            attr = BRANCH | READ_RS;
            break;
        case 2:
            op = BLTZL____;
            attr = BRANCH_LIKELY | READ_RS;
            break;
        case 3:
            op = BGEZL____;
            attr = BRANCH_LIKELY | READ_RS;
            break;
        case 8:
            op = TGEI_____;
            attr = READ_RS;
            break;
        case 9:
            op = TGEIU____;
            attr = READ_RS;
            break;
        case 10:
            op = TLTI_____;
            attr = READ_RS;
            break;
        case 11:
            op = TLTIU____;
            attr = READ_RS;
            break;
        case 12:
            op = TEQI_____;
            attr = READ_RS;
            break;
        case 14:
            op = TNEI_____;
            attr = READ_RS;
            break;
        case 16:
            op = BLTZAL___;
            attr = BRANCH | READ_RS | WRITE_RRA;
            break;
        case 17:
            op = BGEZAL___;
            attr = BRANCH | READ_RS | WRITE_RRA;
            break;
        case 18:
            op = BLTZALL__;
            attr = BRANCH_LIKELY | READ_RS | WRITE_RRA;
            break;
        case 19:
            op = BGEZALL__;
            attr = BRANCH_LIKELY | READ_RS | WRITE_RRA;
            break;
        }
        break;
    case 2:
        op = J________;
        attr = BRANCH;
        break;
    case 3:
        op = JAL______;
        attr = BRANCH | WRITE_RRA;
        break;
    case 4:
        op = BEQ______;
        attr = BRANCH | READ_RS | READ_RT;
        break;
    case 5:
        op = BNE______;
        attr = BRANCH | READ_RS | READ_RT;
        break;
    case 6:
        op = BLEZ_____;
        attr = BRANCH | READ_RS;
        break;
    case 7:
        op = BGTZ_____;
        attr = BRANCH | READ_RS;
        break;
    case 8:
        op = ADDI_____;
        attr = READ_RS | WRITE_RT;
        break;
    case 9:
        op = ADDIU____;
        attr = READ_RS | WRITE_RT;
        break;
    case 10:
        op = SLTI_____;
        attr = READ_RS | WRITE_RT;
        break;
    case 11:
        op = SLTIU____;
        attr = READ_RS | WRITE_RT;
        break;
    case 12:
        op = ANDI_____;
        attr = READ_RS | WRITE_RT;
        break;
    case 13:
        op = ORI______;
        attr = READ_RS | WRITE_RT;
        break;
    case 14:
        op = XORI_____;
        attr = READ_RS | WRITE_RT;
        break;
    case 15:
        op = LUI______;
        attr = WRITE_RT;
        break;
    case 16:
        switch (rs) {
        case 0:
            op = MFC0_____;
            attr = READ_NONE | WRITE_RT;
            break;
        case 2:
            op = CFC0_____;
            attr = READ_NONE | WRITE_RT;
            break;
        case 4:
            op = MTC0_____;
            attr = READ_RT | WRITE_NONE;
            break;
        case 16:
            if (funct == 1)
                op = TLBR_____;
            else if (funct == 2)
                op = TLBWI____;
            else if (funct == 6)
                op = TLBWR____;
            else if (funct == 8)
                op = TLBP_____;
            else if (funct == 24) {
                op = ERET_____;
                attr = BRANCH_ERET;
            } else if (funct == 32) {
                op = WAIT_____;
            }
            break;
        }
        break;
    case 17:
        op = FLOAT_OPS;
        break;
    case 20:
        op = BEQL_____;
        attr = BRANCH_LIKELY | READ_RS | READ_RT;
        break;
    case 21:
        op = BNEL_____;
        attr = BRANCH_LIKELY | READ_RS | READ_RT;
        break;
    case 22:
        op = BLEZL____;
        attr = BRANCH_LIKELY | READ_RS;
        break;
    case 23:
        op = BGTZL____;
        attr = BRANCH_LIKELY | READ_RS;
        break;
    case 28:
        switch (funct) {
        case 0:
            op = MADD_____;
            attr = READ_RS | READ_RT | READ_HILO | WRITE_HILO;
            break;
        case 1:
            op = MADDU____;
            attr = READ_RS | READ_RT | READ_HILO | WRITE_HILO;
            break;
        case 2:
            op = MUL______;
            attr = READ_RS | READ_RT | WRITE_RD;
            break;
        case 4:
            op = MSUB_____;
            attr = READ_RS | READ_RT | READ_HILO | WRITE_HILO;
            break;
        case 5:
            op = MSUBU____;
            attr = READ_RS | READ_RT | READ_HILO | WRITE_HILO;
            break;
        case 32:
            op = CLZ______;
            attr = READ_RS | WRITE_RD;
            break;
        case 33:
            op = CLO______;
            attr = READ_RS | WRITE_RD;
            break;
        }
        break;
    case 32:
        op = LB_______;
        attr = READ_RS | WRITE_RT | LOAD_1B;
        break;
    case 33:
        op = LH_______;
        attr = READ_RS | WRITE_RT | LOAD_2B;
        break;
    case 34:
        op = LWL______;
        attr = READ_RS | READ_RT | WRITE_RT | LOAD_4B_UNALIGN;
        break;
    case 35:
        op = LW_______;
        attr = READ_RS | WRITE_RT | LOAD_4B_ALIGN;
        break;
    case 36:
        op = LBU______;
        attr = READ_RS | WRITE_RT | LOAD_1B;
        break;
    case 37:
        op = LHU______;
        attr = READ_RS | WRITE_RT | LOAD_2B;
        break;
    case 38:
        op = LWR______;
        attr = READ_RS | READ_RT | WRITE_RT | LOAD_4B_UNALIGN;
        break;
    case 40:
        op = SB_______;
        attr = READ_RS | READ_RT | STORE_1B;
        break;
    case 41:
        op = SH_______;
        attr = READ_RS | READ_RT | STORE_2B;
        break;
    case 42:
        op = SWL______;
        attr = READ_RS | READ_RT | STORE_4B_UNALIGN;
        break;
    case 43:
        op = SW_______;
        attr = READ_RS | READ_RT | STORE_4B_ALIGN;
        break;
    case 46:
        op = SWR______;
        attr = READ_RS | READ_RT | STORE_4B_UNALIGN;
        break;
    case 47:
        op = CACHE____;
        attr = READ_RS;
        break;
    case 48:
        op = LL_______;
        attr = READ_RS | WRITE_RT | LOAD_4B_ALIGN;
        break;
    case 51:
        op = PREF_____;
        attr = READ_RS;
        break;
    case 56:
        op = SC_______;
        attr = READ_RS | READ_RT | WRITE_RT | STORE_4B_ALIGN;
        break;
    case 49:
    case 53:
    case 57:
    case 61:
        op = FLOAT_OPS;
        break;
    }
}
/**********************************************************************/
void MipsInst::clearmnemonic()
{
    mnemonic[0] = '\0';
    instname = NULL;
}

/**********************************************************************/
const char *MipsInst::getinstname()
{
    if (op == NOP______)      instname = "nop";
    else if (op == SSNOP____) instname = "ssnop";
    else if (op == SLL______) instname = "sll";
    else if (op == SRL______) instname = "srl";
    else if (op == SRA______) instname = "sra";
    else if (op == SLLV_____) instname = "sllv";
    else if (op == SRLV_____) instname = "srlv";
    else if (op == SRAV_____) instname = "srav";
    else if (op == JR_______) instname = "jr";
    else if (op == JR_HB____) instname = "jr";
    else if (op == JALR_____) instname = "jalr";
    else if (op == JALR_HB__) instname = "jalr";
    else if (op == MOVZ_____) instname = "movz";
    else if (op == MOVN_____) instname = "movn";
    else if (op == SYSCALL__) instname = "syscall";
    else if (op == BREAK____) instname = "break";
    else if (op == SYNC_____) instname = "sync";
    else if (op == MFHI_____) instname = "mfhi";
    else if (op == MTHI_____) instname = "mthi";
    else if (op == MFLO_____) instname = "mflo";
    else if (op == MTLO_____) instname = "mtlo";
    else if (op == MULT_____) instname = "mult";
    else if (op == MULTU____) instname = "multu";
    else if (op == DIV______) instname = "div";
    else if (op == DIVU_____) instname = "divu";
    else if (op == ADD______) instname = "add";
    else if (op == ADDU_____) instname = "addu";
    else if (op == SUB______) instname = "sub";
    else if (op == SUBU_____) instname = "subu";
    else if (op == AND______) instname = "and";
    else if (op == OR_______) instname = "or";
    else if (op == XOR______) instname = "xor";
    else if (op == NOR______) instname = "nor";
    else if (op == SLT______) instname = "slt";
    else if (op == SLTU_____) instname = "sltu";
    else if (op == TGE______) instname = "tge";
    else if (op == TGEU_____) instname = "tgeu";
    else if (op == TLT______) instname = "tlt";
    else if (op == TLTU_____) instname = "tltu";
    else if (op == TEQ______) instname = "teq";
    else if (op == TNE______) instname = "tne";
    else if (op == BLTZ_____) instname = "bltz";
    else if (op == BGEZ_____) instname = "bgez";
    else if (op == BLTZL____) instname = "bltzl";
    else if (op == BGEZL____) instname = "bgezl";
    else if (op == TGEI_____) instname = "tgei";
    else if (op == TGEIU____) instname = "tgeiu";
    else if (op == TLTI_____) instname = "tlti";
    else if (op == TLTIU____) instname = "tltiu";
    else if (op == TEQI_____) instname = "teqi";
    else if (op == TNEI_____) instname = "tnei";
    else if (op == BLTZAL___) instname = "bltzal";
    else if (op == BGEZAL___) instname = "bgezal";
    else if (op == BLTZALL__) instname = "bltzall";
    else if (op == BGEZALL__) instname = "bgezall";
    else if (op == J________) instname = "j";
    else if (op == JAL______) instname = "jal";
    else if (op == BEQ______) instname = "beq";
    else if (op == BNE______) instname = "bne";
    else if (op == BLEZ_____) instname = "blez";
    else if (op == BGTZ_____) instname = "bgtz";
    else if (op == ADDI_____) instname = "addi";
    else if (op == ADDIU____) instname = "addiu";
    else if (op == SLTI_____) instname = "slti";
    else if (op == SLTIU____) instname = "sltiu";
    else if (op == ANDI_____) instname = "andi";
    else if (op == ORI______) instname = "ori";
    else if (op == XORI_____) instname = "xori";
    else if (op == LUI______) instname = "lui";
    else if (op == MFC0_____) instname = "mfc0";
    else if (op == CFC0_____) instname = "cfc0";
    else if (op == MTC0_____) instname = "mtc0";
    else if (op == TLBR_____) instname = "tlbr";
    else if (op == TLBWI____) instname = "tlbwi";
    else if (op == TLBWR____) instname = "tlbwr";
    else if (op == TLBP_____) instname = "tlbp";
    else if (op == ERET_____) instname = "eret";
    else if (op == WAIT_____) instname = "wait";
    else if (op == BEQL_____) instname = "beql";
    else if (op == BNEL_____) instname = "bnel";
    else if (op == BLEZL____) instname = "blezl";
    else if (op == BGTZL____) instname = "bgtzl";
    else if (op == MADD_____) instname = "madd";
    else if (op == MADDU____) instname = "maddu";
    else if (op == MUL______) instname = "mul";
    else if (op == MSUB_____) instname = "msub";
    else if (op == MSUBU____) instname = "msubu";
    else if (op == CLZ______) instname = "clz";
    else if (op == CLO______) instname = "clo";
    else if (op == LB_______) instname = "lb";
    else if (op == LH_______) instname = "lh";
    else if (op == LWL______) instname = "lwl";
    else if (op == LW_______) instname = "lw";
    else if (op == LBU______) instname = "lbu";
    else if (op == LHU______) instname = "lhu";
    else if (op == LWR______) instname = "lwr";
    else if (op == SB_______) instname = "sb";
    else if (op == SH_______) instname = "sh";
    else if (op == SWL______) instname = "swl";
    else if (op == SW_______) instname = "sw";
    else if (op == SWR______) instname = "swr";
    else if (op == CACHE____) instname = "cache";
    else if (op == LL_______) instname = "ll";
    else if (op == PREF_____) instname = "pref";
    else if (op == SC_______) instname = "sc";
    else if (op == FLOAT_OPS) instname = "(FP inst)";
    else                      instname = "";

    return instname;
}

/**********************************************************************/
char *MipsInst::getmnemonic()
{
    if (mnemonic[0])
        return mnemonic;

    char *arg;
    arg = new char[MNEMONIC_BUF_SIZE];
    arg[0] = '\0';
    int size = MNEMONIC_BUF_SIZE;

    getinstname();

    switch (op) {
    case SLL______:
    case SRL______:
    case SRA______:
        snprintf(arg, size, "$%s, $%s, %d",
                 regname[rd], regname[rt], shamt);
        break;
    case MFHI_____:
    case MFLO_____:
        snprintf(arg, size, "$%s", regname[rd]);
        break;
    case JR_______:
    case JR_HB____:
    case MTHI_____:
    case MTLO_____:
        snprintf(arg, size, "$%s", regname[rs]);
        break;
    case MULT_____:
    case MULTU____:
    case DIV______:
    case DIVU_____:
    case TGE______:
    case TGEU_____:
    case TLT______:
    case TLTU_____:
    case TEQ______:
    case TNE______:
    case MADD_____:
    case MADDU____:
    case MSUB_____:
    case MSUBU____:
        snprintf(arg, size, "$%s, $%s", regname[rs], regname[rt]);
        break;
    case JALR_____:
    case JALR_HB__:
    case CLZ______:
    case CLO______:
        snprintf(arg, size, "$%s, $%s", regname[rd], regname[rs]);
        break;
    case SLLV_____:
    case SRLV_____:
    case SRAV_____:
        snprintf(arg, size, "$%s, $%s, $%s",
                 regname[rd], regname[rt], regname[rs]);
        break;
    case MOVZ_____:
    case MOVN_____:
    case ADD______:
    case ADDU_____:
    case SUB______:
    case SUBU_____:
    case AND______:
    case OR_______:
    case XOR______:
    case NOR______:
    case SLT______:
    case SLTU_____:
    case MUL______:
        snprintf(arg, size, "$%s, $%s, $%s", 
                 regname[rd], regname[rs], regname[rt]);
        break;
    case TGEI_____:
    case TGEIU____:
    case TLTI_____:
    case TLTIU____:
    case TEQI_____:
    case TNEI_____:
        snprintf(arg, size, "$%s, %d", regname[rs], exts32(imm, 16));
        break;
    case ADDI_____:
    case ADDIU____:
    case SLTI_____:
    case SLTIU____:
        snprintf(arg, size, "$%s, $%s, %d",
                 regname[rt], regname[rs], exts32(imm, 16));
        break;
    case ANDI_____:
    case ORI______:
    case XORI_____:
        snprintf(arg, size, "$%s, $%s, 0x%x",
                 regname[rt], regname[rs], imm);
        break;
    case LUI______:
        snprintf(arg, size, "$%s, 0x%x", regname[rt], imm);
        break;
    case BLTZ_____:
    case BGEZ_____:
    case BLTZL____:
    case BGEZL____:
    case BLTZAL___:
    case BGEZAL___:
    case BLTZALL__:
    case BGEZALL__:
    case BLEZ_____:
    case BGTZ_____:
    case BLEZL____:
    case BGTZL____:
        snprintf(arg, size, "$%s, %08x", 
                 regname[rs], pc + 4 + (exts32(imm, 16) << 2));
        break;
    case BEQ______:
    case BNE______:
    case BEQL_____:
    case BNEL_____:
        snprintf(arg, size, "$%s, $%s, %08x", regname[rs], regname[rt],
                 pc + 4 + (exts32(imm, 16) << 2));
        break;
    case J________:
    case JAL______:
        snprintf(arg, size, "%08x",
                 ((pc + 4) & 0xf0000000) | (addr << 2));
        break;
    case SYSCALL__:
        snprintf(arg, size, "0x%x", code_l);
        break;
    case BREAK____:
        snprintf(arg, size, "0x%x", code_s);
        break;
    case MFC0_____:
    case CFC0_____:
    case MTC0_____:
        snprintf(arg, size, "$%s, %d.%d", regname[rt], rd, sel);
        break;
    case LB_______:
    case LH_______:
    case LWL______:
    case LW_______:
    case LBU______:
    case LHU______:
    case LWR______:
    case SB_______:
    case SH_______:
    case SWL______:
    case SW_______:
    case SWR______:
    case CACHE____:
    case LL_______:
    case PREF_____:
    case SC_______:
        snprintf(arg, size, "$%s, %d($%s)", 
                 regname[rt], exts32(imm, 16), regname[rs]);
        break;
    }

    snprintf(mnemonic, size, "%-9s%s", instname, arg);
    DELETE_ARRAY(arg);
    return mnemonic;
}

/*********************************************************************/
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
