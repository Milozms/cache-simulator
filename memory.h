#ifndef CACHE_MEMORY_H_
#define CACHE_MEMORY_H_

#include <stdint.h>
#include "storage.h"

class Memory: public Storage {
 public:
  Memory():Storage() {
      //printf("Memory construct.\n");
      printlog = false;
  }
  ~Memory() {}

  // Main access process
  void HandleRequest(uint64_t addr, int bytes, int read,
                     unsigned char *content, int &hit, int &time);

 private:
  // Memory implement

  DISALLOW_COPY_AND_ASSIGN(Memory);
};

#endif //CACHE_MEMORY_H_ 
