#include "libcpu.h"
#include "libcpu_m88k.h"

int arch_m88k_tag_instr(uint8_t* RAM, addr_t pc, int *flow_type, addr_t *new_pc);
int arch_m88k_disasm_instr(uint8_t* RAM, addr_t pc, char *line, unsigned int max_line);
int arch_m88k_recompile_instr(uint8_t* RAM, addr_t pc, BasicBlock *bb_dispatch, BasicBlock *bb, BasicBlock *bb_target, BasicBlock *bb_cond, BasicBlock *bb_next);

#define INSTR(a) RAM32(RAM, a)
