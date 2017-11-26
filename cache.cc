#include <cstring>
#include <cmath>
#include "cache.h"
#include "def.h"
Cache::Cache(int _size, int _ass, int _setnum, int _wt, int _wa, int _hitcyc, Storage *_low): Storage(){
    config_.size = _size;
    config_.associativity = _ass;
    config_.set_num = _setnum;
    config_.write_through = _wt;
    config_.write_allocate = _wa;
    latency_.hit_latency = _hitcyc;
    int linesize = _size/_setnum/_ass;
    config_.linesize = linesize;
    lower_ = _low;
    stats_.miss_num = 0;
    stats_.fetch_num = 0;
    stats_.access_counter = 0;
    stats_.replace_num = 0;
    stats_.access_time = 0;
    stats_.prefetch_num = 0;
    //initialize
    for(int i=0;i<_setnum;++i)
        for(int j=0;j<_ass;++j){
            cache_content[i][j].valid =false;
            cache_content[i][j].tag = 0;
            cache_content[i][j].recent = 0;
            cache_content[i][j].dirty = false;
            memset(cache_content[i][j].block,0,sizeof(cache_content[i][j].block));
        }
    s = b = -1;
    while(_setnum != 0)
    {
        s++;
        _setnum = _setnum >> 1;
    }
    while(linesize != 0)
    {
        b++;
        linesize = linesize >> 1;
    }
    simtime = 0;
    printlog = false;
}
// Main access process
// [in]  addr: access address
// [in]  bytes: target number of bytes
// [in]  read: 0|1 for write|read
//             3|4 for write|read in prefetch
// [i|o] content: in|out data
// [out] hit: 0|1 for miss|hit
// [out] cycle: total access cycle
void Cache::HandleRequest(uint64_t addr, int bytes, int read,
                          unsigned char *content, int &hit, int &time) {
    uint64_t set_index = (addr << (64-s-b)) >> (64-s);
    uint64_t block_offest = (addr << (64-b)) >> (64-b);
    uint64_t tag = addr >> (s+b);
    int lower_hit, lower_time, replace_time;
    if(block_offest + bytes > config_.linesize){
        //bytes too long
        if(printlog) printf("Too long!\n");
        HandleRequest(addr, (int)(config_.linesize - block_offest), read, content, hit, time);
        HandleRequest(addr+config_.linesize, bytes-(int)(config_.linesize - block_offest), read,
                      content+(int)(config_.linesize - block_offest), hit, time);
        return;
    }
    hit = 0;
    time = 0;
    simtime++;
    // Bypass?
    if (!BypassDecision()) {
        PartitionAlgorithm();
        stats_.access_counter++;
        int block_index = ReplaceDecision(addr);
        if (block_index < 0) {
            // Miss, Choose victim
            stats_.miss_num++;
            if(printlog) printf("%llx Miss.\n", addr);
            if(read == 0){//Write
                if(config_.write_allocate == 1){
                    //write allocate, only write cache
                    if(printlog) printf("%llx Write allocated.\n", addr);
                    block_index = ReplaceAlgorithm(addr, replace_time);
                    memcpy(cache_content[set_index][block_index].block+block_offest, content, bytes);
                    cache_content[set_index][block_index].dirty = true;
                    cache_content[set_index][block_index].recent = simtime;
                    time += latency_.hit_latency + replace_time;
                }
                else{
                    //write not allocate, directly write memory
                    stats_.fetch_num++;
                    if(printlog) printf("%llx Write not allocated.\n", addr);
                    lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
                    time += latency_.bus_latency + lower_time;
                    //time will be updated
                }
            }
            else if(read == 1){//Read
                block_index = ReplaceAlgorithm(addr, replace_time);
                memcpy(content, cache_content[set_index][block_index].block+block_offest, bytes);
                time += latency_.hit_latency + replace_time;
                cache_content[set_index][block_index].recent = simtime;
            }
            stats_.access_time += time;
        } else {
            // Hit, return hit & time
            hit = 1;
            if(printlog) printf("%llx Hit.\n", addr);
            if(read == 0){
                //Write
                if(config_.write_through == 1){
                    //Write through: write cache and memory
                    if(printlog)  printf("%llx Write through.\n", addr);
                    //cache
                    memcpy(cache_content[set_index][block_index].block+block_offest, content, bytes);
                    cache_content[set_index][block_index].dirty = false;
                    cache_content[set_index][block_index].recent = simtime;
                    //memory
                    stats_.fetch_num++;
                    lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
                    time += lower_time + latency_.bus_latency;
                    //time will be updated
                }
                else{
                    //Write back: write cache
                    if(printlog) printf("%llx Write back.\n", addr);
                    stats_.fetch_num++;
                    memcpy(cache_content[set_index][block_index].block+block_offest, content, bytes);
                    cache_content[set_index][block_index].dirty = true;
                    cache_content[set_index][block_index].recent = simtime;
                }
            }
            else if(read == 1){
                //Read
                memcpy(content, cache_content[set_index][block_index].block+block_offest, bytes);
                cache_content[set_index][block_index].recent = simtime;
            }
            time += latency_.hit_latency;
            stats_.access_time += time;
            return;
        }
    }
    // Prefetch?
    if (PrefetchDecision()) {
        PrefetchAlgorithm();
    } else {
        // Fetch from lower layer
        int lower_hit, lower_time;
        lower_->HandleRequest(addr, bytes, read, content,
                              lower_hit, lower_time);
        hit = 0;
        time += latency_.bus_latency + lower_time;
        stats_.access_time += latency_.bus_latency;
    }
}

int Cache::BypassDecision() {
  return FALSE;
}

void Cache::PartitionAlgorithm() {
}

//if hit, return block index; else, return -1
int Cache::ReplaceDecision(uint64_t addr) {
    uint64_t set_index = (addr << (64-s-b)) >> (64-s);
    uint64_t tag = addr >> (s+b);
    for(int i=0;i<config_.associativity;++i){
        if(cache_content[set_index][i].valid && cache_content[set_index][i].tag == tag){
            return i;
        }
    }
    return -1;
}

//return the block index replaced
int Cache::ReplaceAlgorithm(uint64_t addr, int& time){
    int empty_block_index = -1;
    int lru = 0;
    int tmp_hit, tmp_time;
    time = 0;
    uint64_t min_recent = 1<<63;
    uint64_t set_index = (addr << (64-s-b)) >> (64-s);
    uint64_t tag = addr >> (s+b);
    for(int i=config_.associativity - 1;i>=0;--i){
        if(cache_content[set_index][i].valid){
            if(cache_content[set_index][i].recent < min_recent){
                min_recent = cache_content[set_index][i].recent;
                lru = i;
            }
        }
        else{
            empty_block_index = i;
        }
    }
    if(empty_block_index < 0){
        //there is no empty block, replace
        if(printlog) printf("Set %d block %d evicted.\n", set_index, lru);
        stats_.replace_num++;
        //if write back or write allocate, update memory
        if(config_.write_through==0 || config_.write_allocate==1){
            if(cache_content[set_index][lru].dirty){
                stats_.fetch_num++;
                if(printlog) printf("Write back %d to the lower storage, addr = %llx.\n", lru, cache_content[set_index][lru].tag<<(b+s)|set_index<<b);
                lower_->HandleRequest(addr, config_.linesize, 0, cache_content[set_index][lru].block, tmp_hit, tmp_time);
                time += tmp_time;
            }
        }
    }
    else{
        lru = empty_block_index;
    }
    stats_.fetch_num++;
    lower_->HandleRequest(addr, config_.linesize, 1, cache_content[set_index][lru].block, tmp_hit, tmp_time);
    time += tmp_time;
    time += latency_.bus_latency;
    cache_content[set_index][lru].valid=true;
    cache_content[set_index][lru].dirty=false;
    cache_content[set_index][lru].recent=simtime;
    cache_content[set_index][lru].tag=tag;
    return lru;

}

void Cache::print_cache_content(){
    for(int i=0;i<config_.set_num;++i)
        for(int j=0;j<config_.associativity;++j){
            if(cache_content[i][j].valid){
                if(printlog) printf("Set %d, valid, tag = %llx, recent = %lld, dirty = %d.\n",
                       i,cache_content[i][j].tag, cache_content[i][j].recent, cache_content[i][j].dirty);
            }
        }
}

int Cache::PrefetchDecision() {
  return FALSE;
}

void Cache::PrefetchAlgorithm() {
}

