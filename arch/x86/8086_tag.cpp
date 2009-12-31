#include "libcpu.h"
#include "8086_decode.h"
#include "8086_isa.h"
#include "tag.h"

int
arch_8086_tag_instr(cpu_t *cpu, addr_t pc, tag_t *tag, addr_t *new_pc, addr_t *next_pc)
{
	struct x86_instr instr;
	int len;

	if (arch_8086_decode_instr(&instr, cpu->RAM, pc) != 0)
		return -1;

	len = arch_8086_instr_length(&instr);

	*tag = TAG_CONTINUE;

	*next_pc = pc + len;

	return len;
}
