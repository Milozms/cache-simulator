#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.h"
#include "def.h"

typedef struct CacheConfig_ {
    int size;
    int associativity;
    int set_num; // Number of cache sets
    int linesize;
    int write_through; // 0|1 for back|through
    int write_allocate; // 0|1 for no-alc|alc
} CacheConfig;

typedef struct Entry_ {
    bool valid;
    uint64_t tag;
    unsigned char block[MAXLINESIZE];
    uint64_t recent;
    bool dirty;
} Entry;

class Cache: public Storage {
public:
    Cache(int _size, int _ass, int _setnum, int _wt, int _wa, int _hitcyc, Storage *_low);

    ~Cache() {}

    // Sets & Gets
    //void SetConfig(CacheConfig cc);

    //void GetConfig(CacheConfig cc);
    void print_cache_content();
    void SetLower(Storage *ll) { lower_ = ll; }

    // Main access process
    void HandleRequest(uint64_t addr, int bytes, int read,
                       unsigned char *content, int &hit, int &time);

private:
    Entry cache_content[MAXSNUM][MAXENUM];
    // Bypassing
    int BypassDecision();

    // Partitioning
    void PartitionAlgorithm();

    // Replacement
    //if hit, return block index; else, return -1
    int ReplaceDecision(uint64_t addr);
    //return the block index replaced
    int ReplaceAlgorithm(uint64_t addr, int& time);

    // Prefetching
    int PrefetchDecision();

    void PrefetchAlgorithm();

    CacheConfig config_;
    int s,b;
    Storage *lower_;
    uint64_t simtime;
    DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif //CACHE_CACHE_H_ 
