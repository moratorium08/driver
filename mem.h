#pragma once

typedef size_t phys_addr;
typedef void *virt_addr;

void init_mem(size_t size);
phys_addr get_phys_ptr();
virt_addr get_virt_ptr();
