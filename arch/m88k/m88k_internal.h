#include "libcpu.h"
#include "libcpu_m88k.h"

int arch_m88k_tag_instr(cpu_t *cpu, addr_t pc, tag_t *tag, addr_t *new_pc, addr_t *next_pc);
int arch_m88k_disasm_instr(cpu_t *cpu, addr_t pc, char *line, unsigned int max_line);
Value *arch_m88k_translate_cond(cpu_t *cpu, addr_t pc, BasicBlock *bb);
int arch_m88k_translate_instr(cpu_t *cpu, addr_t pc, BasicBlock *bb);

#define INSTR(a) RAM32(cpu->RAM, a)
