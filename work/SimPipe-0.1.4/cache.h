/* -*-c++-*-
 * SimPipe: a MIPS Pipeline Simulator using SimMips
 *   Keiji Kimura
 */

#ifndef  CACHE_H
#define  CACHE_H

#ifndef  L_NAME
#include  "define.h"
#endif

#include  <set>

class Cache {
public:
    enum { CACHE_READ, CACHE_WRITE };

    Cache(uint032_t size, uint032_t way, uint032_t line, int penalty,
	  bool writeback);
    ~Cache();

    int  Access(uint064_t address, int rwtype);

    void PutStatistics();

private:
    bool is_hit(uint064_t address, uint064_t& tag,
		uint032_t& index, uint032_t& line, uint032_t& offset);

    bool get_empty_line(uint032_t index, uint032_t& line);

    uint032_t get_write_back_line(uint032_t index);

    void  update_lru_count(uint032_t index, uint032_t line);

    enum { VALID = 1, MODIFIED = 2 };
    uint032_t  size;
    uint032_t  way;
    uint032_t  line;
    int  penalty;
    bool  writeback;

    uint064_t  tag_mask;
    uint064_t  index_mask;
    uint064_t  offset_mask;
    uint064_t  offset_mask_bits;
    uint032_t  number_of_lines;

    int*  state;
    uint064_t*  tagarray;
    int*  lru_count;

    int  hit_count;
    int  access_count;
    int  compulsory_count;
    int  capacity_count;
    int  conflict_count;
    int  remained_lines;
    std::set<uint064_t>* block_hist;
};

#endif	// CACHE_H
