/**********************************************************************
 * SimMips: Simple Computer Simulator of MIPS    Arch Lab. TOKYO TECH *
 **********************************************************************/
#include "define.h"
#ifdef __CYGWIN__
#include <ncurses/ncurses.h>
#else
#include <ncurses.h>
#endif

enum {
    PIC_RD_IRR = 0,
    PIC_RD_ISR = 1,
    PIC_RD_BUFIRR = 2,
    PIC_CONNECTED = 2,

    PIC_PRI_ADDR = 0x20,
    PIC_SEC_ADDR = 0xa0,
    PIC_ADDR_RANGE = 2,

    SIO_RBR = 0,
    SIO_IER = 1,
    SIO_IIR = 2,
    SIO_LCR = 3,
    SIO_MCR = 4,
    SIO_LSR = 5,
    SIO_MSR = 6,
    SIO_SCR = 7,

    SIO_IER_TX = 0x02,
    SIO_IER_RX = 0x01,
    SIO_IIR_FIFO = 0xc0,
    SIO_FIFO_SH = 5,
    SIO_IIR_TX = 0x02,
    SIO_IIR_RX = 0x04,
    SIO_IIR_NP = 0x01,
    SIO_FCR_FIFO = 0x06,
    SIO_LCR_DIV = 0x80,
    SIO_MCR_INT = 0x08,
    SIO_LSR_VALUE = 0x60,
    SIO_MSR_VALUE = 0x0a,
    SIO_CONNECTED = 4,

    SIO_PRI_ADDR = 0x3f8,
    SIO_ADDR_RANGE = 8,

    SIO_POLL_CYCLE = 0x100,

    MIERU_SW = 0x00,
    MIERU_LCD = 0x04,
    MIERU_SEG = 0x08,
    MIERU_CNT = 0x0c,
    MIERU_KB  = 0x10,
};

/* Interrupt Controller, integrating TWO 8259-like PIC                */
/**********************************************************************/
IntController::IntController(MipsCp0 *cp0)
{
    this->cp0 = cp0;
    for (int i = 0; i < 2; i++) {
        imr[i] = 0xff;
        irr[i] = 0;
        isr[i] = 0;
        tobe_read[i] = PIC_RD_BUFIRR;
        init_mode[i] = 0; 
    }
}

/**********************************************************************/
int IntController::checkaddr(uint032_t addr)
{
    uint032_t base = addr & ~0x1;
    if (base == PIC_PRI_ADDR)
        return 0;
    else if (base == PIC_SEC_ADDR)
        return 1;
    else
        return -1;
}
/**********************************************************************/
void IntController::recalcirq()
{
    // Secondary PIC(IRQ 8-15) is connected to IRQ2
    irr[0] = ((irr[0] & ~(1 << 2)) |
              (((irr[1] & ~imr[1]) != 0) << 2));

    for (int i = 0; i < 2; i++) {
        isr[i] = 0;
        for (int j = 0; j < 8; j++) {
            if (irr[i] & ~imr[i] & (1 << j)) {
                isr[i] = 1 << j;
                break;
            }
        }
    }
    if (isr[0])
        cp0->setinterrupt(PIC_CONNECTED);
    else
        cp0->clearinterrupt(PIC_CONNECTED);
}

/**********************************************************************/
void IntController::read1b(uint032_t addr, uint008_t *data)
{
    int ch = checkaddr(addr);
    if (ch < 0)
        return;

    if ((addr & 0x1) == 0) {
        if        (tobe_read[ch] == PIC_RD_IRR) {
            *data = irr[ch];
        } else if (tobe_read[ch] == PIC_RD_ISR) {
            *data = isr[ch];
        } else {                 // PIC_RD_BUFIRR
            *data = 0x00;
            for (int i = 0; i < 8; i++) {
                if (irr[ch] & (1 << i)) {
                    *data = 0x80 + i;
                    break;
                }
            }
        }
    } else {
        *data = imr[ch];
    }
}

/**********************************************************************/
void IntController::write1b(uint032_t addr, uint008_t data)
{
    int ch = checkaddr(addr);
    if (ch < 0)
        return;

    if ((addr & 0x1) == 0) {
        if (data == 0x0a) {
            tobe_read[ch] = PIC_RD_IRR;
        } else if (data == 0x0b) {
            tobe_read[ch] = PIC_RD_ISR;
        } else if (data == 0x0c) {
            tobe_read[ch] = PIC_RD_BUFIRR;
        } else if ((data >= 0x10) && (data <= 0x1f)) {
            init_mode[ch] = 2 + (data & 0x1);
        } else if ((data >= 0x20) && (data <= 0x27)) {
            irr[ch] &= ~isr[ch];
            isr[ch] = 0;
            recalcirq();
        } else if ((data >= 0x60) && (data <= 0x67)) {
            irr[ch] &= ~(1 << (data & 0xf));
            isr[ch] &= ~(1 << (data & 0xf));
            recalcirq();
        } else {
            printf("## IntController: undefined op: 0x%02x\n", data);
        }
    } else {
        if (init_mode[ch]) {
            init_mode[ch]--;
        } else {
            imr[ch] = data;
            recalcirq();
        }
    }
}

/**********************************************************************/
void IntController::setinterrupt(int irq)
{
    irr[irq / 8] |= 1 << (irq % 8);
    recalcirq();
}

/**********************************************************************/
void IntController::clearinterrupt(int irq)
{
    irr[irq / 8] &= ~(1 << (irq % 8));
    recalcirq();
}

/* Serial I/O Controller, simplifying ns16550                         */
/**********************************************************************/
SerialIO::SerialIO(IntController *pic)
{
    this->pic = pic;
    ier = 0;
    iir = SIO_IIR_NP;
    lcr = 0;
    mcr = 0;
    scr = 0;
    counter = 0;
    divisor = 12;
    currentchar = -1;
}

/**********************************************************************/
void SerialIO::step()
{
    if (++counter < SIO_POLL_CYCLE)
        return;

    counter = 0;
    if (charavail()) {
        iir |= SIO_IIR_RX;
        recalcirq();
    }
}

/**********************************************************************/
int SerialIO::charavail()
{
    char buf;
    int ret;

    if (currentchar != -1)
        return 1;
    ret = read(STDIN_FILENO, &buf, sizeof(buf));
    if (ret == 1) {
        currentchar = ((int) buf) & 0xff;
        return 1;
    }
    return 0;
}

/**********************************************************************/
void SerialIO::recalcirq()
{
    int int_pend = 0;
    int_pend = ((ier & SIO_IER_TX) && (iir & SIO_IIR_TX));
    int_pend = int_pend || ((ier & SIO_IER_RX) && (iir & SIO_IIR_RX));
    iir = (iir & ~SIO_IIR_NP) | ((int_pend) ? 0 : SIO_IIR_NP);
    int_pend = int_pend && (mcr & SIO_MCR_INT);
    if (int_pend)
        pic->setinterrupt(SIO_CONNECTED);
    else
        pic->clearinterrupt(SIO_CONNECTED);
}

/**********************************************************************/
void SerialIO::read1b(uint032_t addr, uint008_t *data)
{
    switch(addr) {
    case SIO_RBR:
        if (lcr & SIO_LCR_DIV) {
            *data = divisor & 0xff;
        } else {
            *data = (uint008_t) currentchar;
            currentchar = -1;
            iir &= ~SIO_IIR_RX;
            recalcirq();
        }
        break;
    case SIO_IER:
        *data = (lcr & SIO_LCR_DIV) ? divisor >> 8 : ier;
        break;
    case SIO_IIR:
        *data = iir;
        iir &= ~SIO_IIR_TX;
        recalcirq();
        break;
    case SIO_LCR:
        *data = lcr;
        break;
    case SIO_MCR:
        *data = mcr;
        break;
    case SIO_LSR:
        *data = SIO_LSR_VALUE + ((iir & SIO_IIR_RX) != 0);
        break;
    case SIO_MSR:
        *data = SIO_MSR_VALUE;
        break;
    case SIO_SCR:
        *data = scr;
        break;
    }
}

/**********************************************************************/
void SerialIO::write1b(uint032_t addr, uint008_t data)
{
    switch(addr) {
    case SIO_RBR:
        if (lcr & SIO_LCR_DIV) {
            divisor = (divisor & 0xff00) | data;
        } else {
            putchar((int) data & 0xff);
            fflush(stdout);
            iir |= SIO_IIR_TX;
            recalcirq();
        }
        break;
    case SIO_IER:
        if (lcr & SIO_LCR_DIV) {
            divisor = (divisor & 0x00ff) | ((uint) data << 8);
        } else {
            if (((ier & SIO_IER_TX) == 0) && ((data & SIO_IER_TX) != 0))
                iir |= SIO_IIR_TX;
            ier = data;
            recalcirq();
        }
        break;
    case SIO_IIR:
        iir = ((iir & ~SIO_IIR_FIFO) | 
               ((data & SIO_FCR_FIFO) << SIO_FIFO_SH));
        break;
    case SIO_LCR:
        lcr = data;
        break;
    case SIO_MCR:
        mcr = data;
        recalcirq();
        break;
    case SIO_LSR:
    case SIO_MSR:
        // lsr, msr are read-only
        break;
    case SIO_SCR:
        scr = data;
        break;
    }
}

/* ISA Bus I/O                                                        */
/**********************************************************************/
IsaIO::IsaIO()
{
    pic = NULL;
    sio = NULL;
}

/**********************************************************************/
IsaIO::~IsaIO()
{
    DELETE(pic);
    DELETE(sio);
}

/**********************************************************************/
void IsaIO::init(Board *board)
{
    pic = new IntController(board->chip->cp0);
    sio = new SerialIO(pic);
}

/**********************************************************************/
void IsaIO::step()
{
    sio->step();
}

/**********************************************************************/
void IsaIO::read1b(uint032_t addr, uint008_t *data)
{
    if ((addr - PIC_PRI_ADDR < PIC_ADDR_RANGE) ||
        (addr - PIC_SEC_ADDR < PIC_ADDR_RANGE))
        pic->read1b(addr, data);
    else if (addr - SIO_PRI_ADDR < SIO_ADDR_RANGE)
        sio->read1b(addr - SIO_PRI_ADDR, data);
    else
        *data = 0x00;
}

/**********************************************************************/
void IsaIO::write1b(uint032_t addr, uint008_t data)
{
    if ((addr - PIC_PRI_ADDR < PIC_ADDR_RANGE) ||
        (addr - PIC_SEC_ADDR < PIC_ADDR_RANGE))
        pic->write1b(addr, data);
    else if (addr - SIO_PRI_ADDR < SIO_ADDR_RANGE)
        sio->write1b(addr - SIO_PRI_ADDR, data);
}

/* I/O for MieruPC                                                    */
/**********************************************************************/
void MieruIO::init(Board *board)
{
    this->board = board;
    if (!board->debug_mode)
        lcd_ttyopen();
}

/**********************************************************************/
void MieruIO::fini()
{
    lcd_ttyclose();
}

/**********************************************************************/
void MieruIO::lcd_ttyopen()
{
    initscr();
    start_color();
    
    if (COLORS >= 8) {
        init_pair(1, COLOR_YELLOW, COLOR_BLACK);
        init_pair(2, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(3, COLOR_RED, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
        init_pair(5, COLOR_GREEN, COLOR_BLACK);
        init_pair(6, COLOR_BLUE, COLOR_BLACK);
        init_pair(7, COLOR_BLACK, COLOR_BLACK);
    }

    clear();
    lcd_width = 40;
    lcd_height = 15;
    cursorx = cursory = 0;

    for (int i = 0; i < lcd_height; i++) {
        move(i, lcd_width);
        printw("#");
    }
    move(lcd_height, 0);
    for (int i = 0; i < lcd_width + 1; i++)
        printw("#");
}

/**********************************************************************/
void MieruIO::lcd_ttyclose()
{
    endwin();
}

/**********************************************************************/
void MieruIO::lcd_setcolor(int color)
{
    attrset(COLOR_PAIR(((color & 0x80) >> 5) ^
                       ((color & 0x10) >> 3) ^
                       ((color & 0x02) >> 1) ^ 0x7));
}

/**********************************************************************/
void MieruIO::lcd_cls()
{
    int x, y;
    for (y = 0; y < lcd_height; y++) {
        move(y, 0);
        for (x = 0; x < lcd_width; x++)
            printw(" ");
    }
    refresh();
}

/**********************************************************************/
int MieruIO::lcd_printf(char *fmt, ...)
{
    char buf[256];
    int i, ret;
    va_list arg;
    va_start(arg, fmt);

    move(cursory, cursorx);
    ret = vsnprintf(buf, 256, fmt, arg);
    va_end(arg);

    for (i = 0; i < ret; i++) {
        printw("%c", buf[i]);
        if (++cursorx >= lcd_width) {
            cursorx = 0;
            cursory = (cursory + 1) % lcd_height;
            move(cursory, cursorx);
        }
    }
    refresh();
    return ret;
}

/**********************************************************************/
void MieruIO::lcd_nextline()
{
    cursorx = 0;
    cursory = (cursory + 1) % lcd_height;
}

/**********************************************************************/
void MieruIO::read1b(uint032_t addr, uint008_t *data)
{
    char swbuf;
    if (addr == MIERU_SW) {
        *data = 0;
        if (read(STDIN_FILENO, &swbuf, sizeof(swbuf)) != 0)
            *data = ((swbuf == 'z') ? 4 :
                     (swbuf == 'x') ? 2 :
                     (swbuf == 'c') ? 1 : 0);
    } else if (addr == MIERU_LCD) {
        *data = 1;
    } else if (addr == MIERU_KB) {
        if (read(STDIN_FILENO, &swbuf, sizeof(swbuf)) != 0)
            *data = ((swbuf == 'w') ? 32 :
                     (swbuf == 's') ? 16 :
                     (swbuf == 'a') ? 8 :
                     (swbuf == 'd') ? 4 :
                     (swbuf == 'j') ? 2 :
                     (swbuf == 'k') ? 1 : 0);
    }
}

/**********************************************************************/
void MieruIO::read4b(uint032_t addr, uint032_t *data)
{
    if (addr == MIERU_CNT)
        *data = (uint032_t) (board->gettime() / 10);
}

/**********************************************************************/
void MieruIO::write1b(uint032_t addr, uint008_t data)
{
    if ((addr == MIERU_LCD) && (!board->debug_mode)) {
        lcdbuf[lcdindex] = data;
        if (data == '\r') {
            lcdbuf[lcdindex] = '\0';
            if (strncmp(lcdbuf, "CS", 2) == 0) {
                lcd_setcolor(strtol(lcdbuf + 2, NULL, 16));
            } else if (strncmp(lcdbuf, "ER", 2) == 0) {
                lcd_cls();
            } else if (strncmp(lcdbuf, "HP", 2) == 0) {
                char *endptr;
                cursorx = strtol(lcdbuf + 2, &endptr, 10);
                cursory = strtol(endptr + 1, NULL, 10);
            } else if (strncmp(lcdbuf, "HW", 2) == 0) {
                lcdbuf[lcdindex - 1] = '\0';
                lcd_printf(lcdbuf + 3);
            } else if (strncmp(lcdbuf, "HR", 2) == 0) {
                lcd_nextline();
            }
            lcdindex = 0;
        } else {
            lcdindex += (lcdindex != 99);
        }
    }
}

/**********************************************************************/
void MieruIO::write4b(uint032_t addr, uint032_t data)
{
    int x7seg[24] = {12,13,13,12,11,11,12,14, 7, 8, 8, 7, 6, 6, 7, 9,
                      2, 3, 3, 2, 1, 1, 2, 4}; 
    int y7seg[24] = {18,19,21,22,21,19,20,22,18,19,21,22,21,19,20,22,
                     18,19,21,22,21,19,20,22};

    if ((addr == MIERU_SEG) && (!board->debug_mode)) {
        for (int i = 0; i < 24; i++) {
            move(y7seg[i], x7seg[i]);
            printw((data & 0x1) ? "*" : " ");
            data >>= 1;
        }
    }
}

/**********************************************************************/
