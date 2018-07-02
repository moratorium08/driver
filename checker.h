#define CHECK_LEVEL ((check_level)CHECK_STRICT)

#include <stdint.h>

// Define check level.
// When CHECK_NONE, these functions do nothing.
#define CHECK_STRICT 2
#define CHECK_LOOSE  1
#define CHECK_NONE   0

typedef int check_level;


void check_set_rqueue_head(check_level l, uint32_t val);
void check_set_tqueue_head(check_level l, uint32_t val);
void check_set_rqueue_tail(check_level l, uint32_t val);
void check_set_tqueue_tail(check_level l, uint32_t val);
void check_before_rqueue_enable(check_level l);
void check_before_tqueue_enable(check_level l);
void check_before_set_rdba(check_level l);
void check_before_set_tdba(check_level l);
void check_before_set_rdlen(check_level l);
void check_before_set_tdlen(check_level l);
