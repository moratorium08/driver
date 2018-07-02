#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "mem.h"
#include "util.h"

size_t v2p(size_t vaddr);
static virt_addr _virt;
static phys_addr _phys;
static size_t _req_size = 0;
static size_t _act_size = 0;

phys_addr get_phys_ptr() { return _phys; }

virt_addr get_virt_ptr() { return _virt; }

void init_mem(size_t size) {
  _req_size = size;

  if (_req_size > 0) {
    _act_size = 2 * 1024 * 1024;
    assert(_req_size <= _act_size);
    _virt =
      mmap(NULL, _act_size, PROT_READ | PROT_WRITE,
          MAP_PRIVATE | MAP_HUGETLB | MAP_ANONYMOUS | MAP_POPULATE, 0, 0);
    if (_virt == MAP_FAILED) {
      perror("page alloc:");
      panic("");
    }
    _phys = v2p((size_t)_virt);
  } else {
    _virt = 0;
    _phys = 0;
  }
  bzero(get_virt_ptr(), _req_size);
}

size_t v2p(size_t vaddr) {
  FILE *pagemap;
  size_t paddr = 0;
  ssize_t offset = (vaddr / sysconf(_SC_PAGESIZE)) * sizeof(uint64_t);
  uint64_t e;

  // https://www.kernel.org/doc/Documentation/vm/pagemap.txt
  if ((pagemap = fopen("/proc/self/pagemap", "r"))) {
    if (lseek(fileno(pagemap), offset, SEEK_SET) == offset) {
      if (fread(&e, sizeof(uint64_t), 1, pagemap)) {
        if (e & (1ULL << 63)) {            // page present ?
          paddr = e & ((1ULL << 55) - 1);  // pfn mask
          paddr = paddr * sysconf(_SC_PAGESIZE);
          // add offset within page
          paddr = paddr | (vaddr & (sysconf(_SC_PAGESIZE) - 1));
        }
      }
    }
    fclose(pagemap);
  }
  return paddr;
}
