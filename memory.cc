#include "memory.h"

void Memory::HandleRequest(uint64_t addr, int bytes, int read,
                          unsigned char *content, int &hit, int &time) {
  hit = 1;
  time = latency_.hit_latency + latency_.bus_latency;
  stats_.access_time += time;
}

