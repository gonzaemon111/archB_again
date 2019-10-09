/**********************************************************************
 * SimMips: Simple Computer Simulator of MIPS    Arch Lab. TOKYO TECH *
 **********************************************************************/
#include "define.h"

enum {
    MCO_READ = 0,
    MCO_WRITE = 1,
};

/************************************************************************/
MainMemory::MainMemory(uint032_t mem_size)
{
    npage = (mem_size + PAGE_SIZE - 1) / PAGE_SIZE;
    mem_size = npage * PAGE_SIZE;
    pagetable = new uint032_t*[npage];
    external = new int[npage];
    for (uint i = 0; i < npage; i++) {
        pagetable[i] = NULL;
        external[i] = 0;
    }
}

/************************************************************************/
MainMemory::~MainMemory()
{
    for (uint i = 0; i < npage; i++)
        if (!external[i]) {
            DELETE_ARRAY(pagetable[i]);
        }
    DELETE_ARRAY(pagetable);
    DELETE_ARRAY(external);
}

/************************************************************************/
uint032_t* MainMemory::setpageentry(uint032_t addr, uint032_t *array)
{
    if (addr >= mem_size)
        return NULL;
    if (!external[addr / PAGE_SIZE])
        DELETE_ARRAY(pagetable[addr / PAGE_SIZE]);
    pagetable[addr / PAGE_SIZE] = array;
    external[addr / PAGE_SIZE] = 1;
    return array;
}

/************************************************************************/
inline uint032_t* MainMemory::newpage(uint032_t addr)
{
    uint032_t *page = new uint032_t[PAGE_SIZE / sizeof(uint032_t)];
    for (uint i = 0; i < PAGE_SIZE / sizeof(uint032_t); i++)
        page[i] = 0;
    pagetable[addr / PAGE_SIZE] = page;
    return page;
}

/************************************************************************/
inline uint032_t* MainMemory::getrealaddr(uint032_t addr)
{
    uint032_t *page = pagetable[addr / PAGE_SIZE];
    if (page == NULL)
        page = newpage(addr);
    page += (addr % PAGE_SIZE) / sizeof(uint032_t);
    return page;
}

/************************************************************************/
void MainMemory::read1b(uint032_t addr, uint008_t *data)
{
    uint032_t temp;
    int offset = (addr & 0x3) * 8;
    read4b(addr, &temp);
    *data = (temp >> offset) & 0xff;
}

/************************************************************************/
void MainMemory::read2b(uint032_t addr, uint016_t *data)
{
    uint032_t temp;
    int offset = (addr & 0x2) * 8;
    read4b(addr, &temp);
    *data = (temp >> offset) & 0xffff;
}

/************************************************************************/
void MainMemory::read4b(uint032_t addr, uint032_t *data)
{
    uint032_t *mem = getrealaddr(addr);
    *data = *mem;
}

/************************************************************************/
void MainMemory::read8b(uint032_t addr, uint064_t *data)
{
    addr = addr & ~0x7;
    uint032_t temp_h, temp_l;
    read4b(addr, &temp_l);
    read4b(addr + 4, &temp_h);
    *data = ((uint064_t) temp_h << 32) | temp_l;
}

/************************************************************************/
void MainMemory::readnb(uint032_t addr, int size, uint008_t *data)
{
    for (int i = 0; i < size; i++, addr++, data++)
        read1b(addr, data);
}

/************************************************************************/
void MainMemory::write1b(uint032_t addr, uint008_t data)
{
    int offset = (addr & 0x3) * 8;
    uint032_t ins = (uint032_t) data << offset;
    uint032_t mask = 0xff << offset;
    uint032_t temp;
    read4b(addr, &temp);
    temp = ins | (temp & ~mask);
    write4b(addr, temp);
}

/************************************************************************/
void MainMemory::write2b(uint032_t addr, uint016_t data)
{
    int offset = (addr & 0x2) * 8;
    uint032_t ins = (uint032_t) data << offset;
    uint032_t mask = 0xffff << offset;
    uint032_t temp;
    read4b(addr, &temp);
    temp = ins | (temp & ~mask);
    write4b(addr, temp);
}

/************************************************************************/
void MainMemory::write4b(uint032_t addr, uint032_t data)
{
    uint032_t *mem = getrealaddr(addr);
    *mem = data;
}

/************************************************************************/
void MainMemory::write8b(uint032_t addr, uint064_t data)
{
    addr = addr & ~0x7;
    write4b(addr, (uint032_t) data);
    write4b(addr + 4, (uint032_t) (data >> 32));
}

/************************************************************************/
void MainMemory::writenb(uint032_t addr, int size, uint008_t *data)
{
    for (int i = 0; i < size; i++, addr++, data++)
        write1b(addr, *data);
}

/************************************************************************/
void MainMemory::print()
{
    uint032_t *page;
    for (uint i = 0; i < npage; i++) {
        page = pagetable[i];
        if (page != NULL) {
            printf("[MEMORY BLOCK: 0x%08x]", i);
            for (uint j = 0; j < PAGE_SIZE / sizeof(uint032_t); j++) {
                if ((j % 4) == 0)
                    printf("\n%08x: ", (uint)
                           (i * PAGE_SIZE + j * sizeof(uint032_t)));
                printf("%02x%02x%02x%02x ",
                       page[j] & 0xff , (page[j] >> 8) & 0xff,
                       (page[j] >> 16) & 0xff , (page[j] >> 24) & 0xff);
            }
            printf("\n\n");
        }
    }
}
/************************************************************************/
MemoryMap::MemoryMap()
{
    addr = size = 0;
    dev = NULL;
    next = NULL;
}

/************************************************************************/
MemoryMap::~MemoryMap()
{
    DELETE(dev);
    DELETE(next);
}

/************************************************************************/
McInst::McInst()
{
    state = MCI_NONE;
    data008 = 0;
    data016 = 0;
    data032 = 0;
    data064 = 0;
}

/************************************************************************/
MemoryController::MemoryController(MemoryMap *mmap, int mode)
{
    this->mmap = mmap;
    this->mode = mode;
    head = tail = 0;
}

/************************************************************************/
int MemoryController::enqueue(uint064_t addr, uint032_t size, void *data)
{
    int ret = head;
    McInst *ih = &inst[head];

    if ((tail - head + NUM_MCINST) % NUM_MCINST == 1)
        return -1;

    ih->state = MCI_PEND;
    ih->op = (data) ? MCO_WRITE : MCO_READ;
    ih->addr = addr;
    ih->size = size;

    if (data) {
        if (size == 1)
            ih->data008 = *(uint008_t *) data;
        else if (size == 2)
            ih->data016 = *(uint016_t *) data;
        else if (size == 4)
            ih->data032 = *(uint032_t *) data;
        else
            return -1;
    }
    
    head = (head == NUM_MCINST - 1) ? 0 : head + 1;
    if (mode == MC_THROUGHMODE)
        step();
    return ret;
}

/************************************************************************/
void MemoryController::step()
{
    McInst *it = &inst[tail];
    MemoryMap *temp;

    if (it->state != MCI_PEND)
        return;

    for (temp = mmap; temp != NULL; temp = temp->next) {
        if (it->addr - temp->addr >= temp->size)
            continue;
        uint032_t addr = (uint032_t) it->addr - temp->addr;
        if (it->op == MCO_READ) {
            // Memory Read
            if (it->size == 1)
                temp->dev->read1b(addr, &it->data008);
            else if (it->size == 2)
                temp->dev->read2b(addr, &it->data016);
            else if (it->size == 4)
                temp->dev->read4b(addr, &it->data032);
            else if (it->size == 8)
                temp->dev->read8b(addr, &it->data064);
            else
                it->state = MCI_FAILURE;
        } else {
            // Memory Write
            if (it->size == 1)
                temp->dev->write1b(addr, it->data008);
            else if (it->size == 2)
                temp->dev->write2b(addr, it->data016);
            else if (it->size == 4)
                temp->dev->write4b(addr, it->data032);
            else if (it->size == 8)
                temp->dev->write8b(addr, it->data064);
            else
                it->state = MCI_FAILURE;
        }
        break;
    }
    if (temp == NULL) {
        it->state = MCI_FAILURE;
        // for debug
        fprintf(stderr, "!! OUT OF MEMORY RANGE: 0x%08llx\n", it->addr);
    }
    if (it->state != MCI_FAILURE)
        it->state = MCI_FINISH;

    tail = (tail == NUM_MCINST - 1) ? 0 : tail + 1;
}

/************************************************************************/
void MemoryController::print()
{
    for (MemoryMap *temp = mmap; temp != NULL; temp = temp->next)
        temp->dev->print();
}

/************************************************************************/
