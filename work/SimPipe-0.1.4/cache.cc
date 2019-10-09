/* -*-c++-*-
 * SimPipe: a MIPS Pipeline Simulator using SimMips
 *   Keiji Kimura
 */

#include  <cassert>
#include  "cache.h"

#undef  DEBUG_CACHE

bool
exp2p(uint032_t number)
{
    uint032_t j = 1;
    for (unsigned int i = 0; i < 32; i++) {
	if (number == j) {
	    return  true;
	} else {
	    j <<= 1;
	}
    }
    return  false;
}

uint032_t
calc_mask_bits(uint032_t mask)
{
    uint032_t  i = 0;
    uint032_t  cmp = 0;

    for (; i < 32; i++) {
	if (cmp == mask) {
	    break;
	} else {
	    cmp <<= 1;
	    cmp |= 1;
	}
    }
    return i;
}

Cache::Cache(uint032_t size, uint032_t way, uint032_t line, int penalty,
	  bool writeback)
    : size(size), way(way), line(line), penalty(penalty), writeback(writeback),
      hit_count(0), access_count(0), compulsory_count(0), capacity_count(0), conflict_count(0)
{
    if (!exp2p(size)) {
	printf("Cache size must be 2^n. (now %d)\n", size);
	abort();
    }
    if (!exp2p(way)) {
	printf("The number of way in a cache must be 2^n. (now %d)\n", way);
	abort();
    }
    if (!exp2p(line)) {
	printf("Cache line size must be 2^n. (now %d)\n", line);
	abort();
    }
    if (penalty < 1) {
	printf("Cache miss penalty must be greater then 0. (now %d)\n",
	       penalty);
	abort();
    }
    if (size % line != 0) {
	printf("Cache size must be divisible by line size. (now size %d, line %d))\n", size, line);
	abort();
    }
    if (size % (line*way) != 0) {
	printf("Cache size must be divisibal by line-size * num-of-way. (now size %d, line %d, way %d)\n", size, line, way);
	abort();
    }

    number_of_lines = size/line;
    remained_lines = number_of_lines;
    offset_mask = (uint064_t)(line - 1);
    offset_mask_bits = calc_mask_bits(offset_mask);
    index_mask  = (uint064_t)((number_of_lines/way-1)<<offset_mask_bits);
    tag_mask = (uint064_t)(~(index_mask|offset_mask));

    state = new int[number_of_lines];
    tagarray = new uint064_t[number_of_lines];
    lru_count = new int[number_of_lines];

    for (uint032_t i = 0; i < number_of_lines; i++) {
	state[i] = 0;
	lru_count[i] = 0;
    }

    block_hist = new std::set<uint064_t>();
}

Cache::~Cache()
{
    delete[]  state;
    delete[]  tagarray;
    delete[]  lru_count;
}

void
Cache::PutStatistics()
{
    printf("\n*** Cache Statistics\n");
    printf("*** access count: %d\n", access_count);
    printf("*** hit count: %d\n", hit_count);
    printf("*** hit ratio: %f\n", (double)hit_count/access_count);
    printf("***  compulsory miss: %d (ratio %f)\n",
	   compulsory_count, (double)compulsory_count/access_count);
    printf("***  capacity miss: %d (ratio %f)\n",
	   capacity_count, (double)capacity_count/access_count);
    printf("***  conflict miss: %d (ratio %f)\n",
	   conflict_count, (double)conflict_count/access_count);
}

int
Cache::Access(uint064_t address, int rwtype)
{
    uint064_t  tag;
    uint032_t  index;
    uint032_t  line;
    uint032_t  offset;
    int  latency = 0;

    access_count++;
#ifdef  DEBUG_CACHE
    fprintf(stderr, "Access for %llx\n", address);
#endif
    if (is_hit(address, tag, index, line, offset)) {
#ifdef  DEBUG_CACHE
	fprintf(stderr, "-- Hit --> Tag: %llx, Index: %x, Line %x\n",
		tag, index, line);
#endif
	hit_count++;
	update_lru_count(index, line);
	if (!writeback && rwtype == CACHE_WRITE) {
	    latency = penalty;
	} else {
	    latency = 1;
	}
	if (rwtype == CACHE_WRITE) {
	    state[line] |= MODIFIED;
	}
    } else if (!writeback && rwtype == CACHE_WRITE) {
	/* Non write allocate in the case of write-through */
#ifdef  DEBUG_CACHE
	fprintf(stderr, "-- MissHit (write for write-through)\n");
#endif
	latency = penalty;
	access_count--; /* Not a valid cache access */
    } else {
	uint064_t  block_no = address >> offset_mask_bits;
	std::set<uint064_t>::iterator sit = block_hist->find(block_no);
	if (sit == block_hist->end()) {
	    compulsory_count++;
	    block_hist->insert(block_no);
	} else {
	    if (remained_lines > 0) {
		conflict_count++;
	    } else {
		capacity_count++;
	    }
	}

	if (get_empty_line(index, line)) {
#ifdef  DEBUG_CACHE
	    fprintf(stderr, "-- MissHit, Load into a Line %x\n", line);
#endif
	    /* Just read a corresponding line. */
	    latency = penalty;
	    remained_lines--;
	} else {
	    line = get_write_back_line(index);
#ifdef  DEBUG_CACHE
	    fprintf(stderr, "-- MissHit, Replace a Line %x\n", line);
#endif
	    if (writeback && (state[line] & MODIFIED)) {
		/* Write-back and read a corresponding line. */
		latency = 2*penalty;
	    } else {
		/* Just read a corresponding line. */
		latency = penalty;
	    }
	}
	tagarray[line] = tag;
	state[line] = VALID;
	if (rwtype == CACHE_WRITE) {
	    state[line] |= MODIFIED;
	}
	update_lru_count(index, line);
    }

    return latency;
}

bool
Cache::is_hit(uint064_t address, uint064_t& tag,
	      uint032_t& index, uint032_t& line, uint032_t& offset)
{
    tag = address & tag_mask;
    index = (uint032_t)((address & index_mask) >> offset_mask_bits);
    offset = (uint032_t)(address & offset_mask);

    for (line = index*way; line < (index+1)*way; line++) {
	if (tagarray[line] == tag && (state[line] & VALID)) {
	    return  true;
	}
    }
    return  false;
}

bool
Cache::get_empty_line(uint032_t index, uint032_t& line)
{
    for (line = index*way; line < (index+1)*way; line++) {
	if (!(state[line] & VALID)) {
	    return  true;
	}
    }
    return  false;
}

uint032_t
Cache::get_write_back_line(uint032_t index)
{
    uint032_t  line = index*way;
    uint032_t  return_line = line;

    for (; line < (index+1)*way; line++) {
	if (lru_count[line] == 0) {
	    return_line = line;
	}
    }

    return return_line;
}

void
Cache::update_lru_count(uint032_t index, uint032_t line)
{
    int count = lru_count[line];

    for (uint032_t l = index*way; l < (index+1)*way; l++) {
	if (lru_count[l] > count) {
	    lru_count[l]--;
	}
    }
    lru_count[line] = way-1;
}
