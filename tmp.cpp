
//
// Created by zms on 2017/11/25.
//

#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "cache.h"
#include "memory.h"


int main0(int argc, char* argv[]) {
    Memory *m = new Memory();
    Cache *l1 = new Cache(64 * 64 * 2048, 64, 64, 0, 0, 3, m);
    StorageStats s;
    s.access_time = 0;
    s.access_counter = 0;
    s.fetch_num = 0;
    s.prefetch_num = 0;
    s.replace_num = 0;
    s.miss_num = 0;
    m->SetStats(s);
    l1->SetStats(s);

    StorageLatency ml;
    ml.bus_latency = 6;
    ml.hit_latency = 100;
    m->SetLatency(ml);

    StorageLatency ll;
    ll.bus_latency = 3;
    ll.hit_latency = 10;
    l1->SetLatency(ll);

    int hit, time;
    unsigned char content[64];
    if (argc > 1) {
        char *filename = argv[1];
        freopen(filename, "r", stdin);

        char action[2];
        unsigned long long addr;
        unsigned char _content_[8];
        memset(_content_, 0, sizeof(_content_));

        int cycle = 0;
        int hit = 0;
        while (scanf("%s %lld", action, &addr) != EOF) {
            //printf("%s %lld\n", action, addr);
            if (action[0] == 'r')
                l1->HandleRequest(addr, 0, 1, _content_, hit, cycle);
            else if (action[0] == 'w')
                l1->HandleRequest(addr, 0, 0, _content_, hit, cycle);
            else {
                printf("action is %s, addr is %lld\n", action, addr);
                printf("Illegal action. EXIT!\n");
                exit(0);
            }
            //l1.print_cache_content();
        }

        StorageStats result;
        l1->GetStats(result);

        //printf("Total access:\t%d\n", result.access_counter);
        //printf("Total miss:\t%d\n", result.miss_num);
        printf("Miss rate:\t%f\n", (float) (result.miss_num) / result.access_counter);
        //printf("Total replacement:\t%d\n", result.replace_num);
        //printf("Lower storage:\t%d\n", result.fetch_num);
        //printf("Total cycle:\t%d\n", cycle);
    } else {
        l1->HandleRequest(0, 0, 1, content, hit, time);
        printf("Request access time: %dns\n", time);
        l1->HandleRequest(1024, 0, 1, content, hit, time);
        printf("Request access time: %dns\n", time);
    }
    l1->GetStats(s);
    printf("Total L1 access time: %dns\n", s.access_time);
    m->GetStats(s);
    printf("Total Memory access time: %dns\n", s.access_time);
    delete m;
    delete l1;
    return 0;
}


