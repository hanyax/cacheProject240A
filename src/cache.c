//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"

//
// TODO:Student Information
//
const char *studentName = "Hanyang Xu";
const char *studentID   = "A92068025";
const char *email       = "hax032@ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//
uint32_t* icache;
uint32_t* dcache;
uint32_t* l2cache;

clock_t* icache_history;
clock_t* dcache_history;
clock_t* l2cache_history;

int* icache_valid;
int* dcache_valid;

uint32_t itag_size;
uint32_t dtag_size;
uint32_t l2tag_size;

int blockBit;
//------------------------------------//
//          Cache Functions           //
//------------------------------------//

// Initialize the Cache Hierarchy
//
void
init_cache()
{
  // Initialize cache stats
  icacheRefs        = 0;
  icacheMisses      = 0;
  icachePenalties   = 0;
  dcacheRefs        = 0;
  dcacheMisses      = 0;
  dcachePenalties   = 0;
  l2cacheRefs       = 0;
  l2cacheMisses     = 0;
  l2cachePenalties  = 0;

  blockBit = (int) log2(blocksize);

  //
  //TODO: Initialize Cache Simulator Data Structures
  //
  uint32_t icache_size = icacheSets * icacheAssoc;
  icache = (uint32_t*) malloc (icache_size * sizeof(uint32_t));
  itag_size = 32 - ((int) log2(blocksize * icacheSets));
  icache_history = (clock_t*) malloc (icache_size * sizeof(clock_t));
  icache_valid = (int*) malloc (icache_size * sizeof(int));

  uint32_t dcache_size = dcacheSets * dcacheAssoc;
  dcache = (uint32_t*) malloc (dcache_size * sizeof(uint32_t));
  dtag_size = 32 - ((int) log2(blocksize * dcacheSets));
  dcache_history = (clock_t*) malloc (dcache_size * sizeof(clock_t));
  dcache_valid = (int*) malloc (dcache_size * sizeof(int));

  uint32_t l2cache_size = l2cacheSets * l2cacheAssoc;
  l2cache = (uint32_t*) malloc (l2cache_size * sizeof(uint32_t));
  l2tag_size = 32 - ((int) log2(blocksize * l2cacheSets));
  l2cache_history = (clock_t*) malloc (l2cache_size * sizeof(clock_t));
}

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t icache_access(uint32_t addr) {
  //
  //TODO: Implement I$
  //
  if (icacheSets == 0) {
    return l2cache_access(addr);
  }

  icacheRefs+=1;

  //printf("address is %x \n", addr);
  uint32_t index = ((addr << itag_size) >> ((int) (itag_size + blockBit))) * icacheAssoc;

  //printf("Index is %x \n", index);
  uint32_t tag_in = addr >> (32-itag_size);
  for (int i = 0; i < icacheAssoc; i++) {
    uint32_t block = icache[index+i];
    uint32_t tag_block = block >> (32-itag_size);
    if (tag_in == tag_block) { // Hit
      if (icache_valid[index + i] == 1) {
        icache_history[index + i] = clock();
        return icacheHitTime;
      }
    }
  }

  /*---- Update Cache and LRU history ------*/
  icacheMisses+=1;

  // find LRU index
  uint32_t penalty = l2cache_access(addr);

  clock_t min_age = icache_history[index];
  int lru_index = 0;
  for (int j = 1; j < icacheAssoc; j++) {
    clock_t age = icache_history[index+j];
    if (age < min_age) {
      min_age = age;
      lru_index = j;
    }
  }

  icache_valid[index+lru_index] = 1;
  icache[index+lru_index] = addr;
  icache_history[index+lru_index] = clock();

  icachePenalties += penalty;
  return (penalty + icacheHitTime);
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t dcache_access(uint32_t addr) {
  //
  //TODO: Implement D$
  //
  if (dcacheSets == 0) {
    return l2cache_access(addr);
  }

  dcacheRefs+=1;

  //printf("address is %x \n", addr);
  uint32_t index = ((addr << dtag_size) >> ((int) (dtag_size + blockBit))) * dcacheAssoc;
  //printf("Index is %x \n", index);

  uint32_t tag_in = addr >> (32-dtag_size);
  for (int i = 0; i < dcacheAssoc; i++) {
    uint32_t block = dcache[index+i];
    uint32_t tag_block = block >> (32-dtag_size);
    if (tag_in == tag_block) { // Hit
      if (dcache_valid[index + i] == 1) {
        dcache_history[index + i] = clock();
        return dcacheHitTime;
      }
    }
  }

  /*---- Update Cache and LRU history ------*/
  dcacheMisses+=1;

  // find LRU index
  clock_t min_age = dcache_history[index];
  int lru_index = 0;
  for (int j = 1; j < dcacheAssoc; j++) {
    clock_t age = dcache_history[index+j];
    if (age < min_age) {
      min_age = age;
      lru_index = j;
    }
  }

  uint32_t penalty = l2cache_access(addr);

  dcache_valid[index+lru_index] = 1;
  dcache[index+lru_index] = addr;
  dcache_history[index+lru_index] = clock();

  dcachePenalties += penalty;
  return (penalty + dcacheHitTime);
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t l2cache_access(uint32_t addr) {
  //
  //TODO: Implement L2$
  //
  if (l2cacheSets == 0) {
    return memspeed;
  }

  l2cacheRefs+=1;

  uint32_t index = ((addr << l2tag_size) >> ((int) (l2tag_size + blockBit))) * l2cacheAssoc;

  uint32_t tag_in = addr >> (32-l2tag_size);
  for (int i = 0; i < l2cacheAssoc; i++) {
    uint32_t block = l2cache[index+i];
    uint32_t tag_block = block >> (32-l2tag_size);
    if (tag_in == tag_block) { // Hit
      l2cache_history[index + i] = clock();
      return l2cacheHitTime;
    }
  }

  l2cacheMisses+=1;

  // find LRU index
  clock_t min_age = l2cache_history[index];
  int lru_index = 0;
  for (int j = 1; j < l2cacheAssoc; j++) {
    clock_t age = l2cache_history[index+j];
    if (age < min_age) {
      min_age = age;
      lru_index = j;
    }
  }

  if (inclusive == 1) {
    // set address invalid in L1
    uint32_t i_index = ((l2cache[index+lru_index] << itag_size) >> ((int) (itag_size + blockBit))) * icacheAssoc;
    uint32_t d_index = ((l2cache[index+lru_index] << dtag_size) >> ((int) (dtag_size + blockBit))) * dcacheAssoc;

    uint32_t itag_in = l2cache[index+lru_index] >> (32-itag_size);
    uint32_t dtag_in = l2cache[index+lru_index] >> (32-dtag_size);

    for (int a = 0; a < icacheAssoc; a++) {
      uint32_t iblock = icache[i_index+a];
      uint32_t itag_block = iblock >> (32-itag_size);
      if (itag_in == itag_block) { // Hit
          icache_valid[i_index + a] = 0;
      }
    }

    for (int b = 0; b < dcacheAssoc; b++) {
      uint32_t dblock = dcache[d_index+b];
      uint32_t dtag_block = dblock >> (32-dtag_size);
      if (dtag_in == dtag_block) { // Hit
          dcache_valid[d_index + b] = 0;
      }
    }
  }

  l2cache[index+lru_index] = addr;
  l2cache_history[index+lru_index] = clock();

  uint32_t penalty = memspeed;
  l2cachePenalties += penalty;
  return (penalty + l2cacheHitTime);
}
