#include "libcpu.h"
#include "tag_generic.h"
#include "tag.h"

static void
init_tagging(cpu_t *cpu)
{
	addr_t tagging_size, i;

	tagging_size = cpu->code_end - cpu->code_start;
	cpu->tagging_type = (tagging_type_t*)malloc(tagging_size);
	for (i = 0; i < tagging_size; i++)
		cpu->tagging_type[i] = TAG_TYPE_UNKNOWN;
}

static bool
is_inside_code_area(cpu_t *cpu, addr_t a)
{
	return a >= cpu->code_start && a < cpu->code_end;
}

static void
or_tagging_type(cpu_t *cpu, addr_t a, tagging_type_t t)
{
	if (is_inside_code_area(cpu, a))
		cpu->tagging_type[a - cpu->code_start] |= t;
}

/* access functions */
tagging_type_t
get_tagging_type(cpu_t *cpu, addr_t a)
{
	if (is_inside_code_area(cpu, a))
		return cpu->tagging_type[a - cpu->code_start];
	else
		return TAG_TYPE_UNKNOWN;
}

bool
is_code(cpu_t *cpu, addr_t a)
{
	return !!(get_tagging_type(cpu, a) & TAG_TYPE_CODE);
}

static void
tag_recursive(cpu_t *cpu, addr_t pc, int level)
{
	int bytes;
	int flow_type;
	addr_t new_pc;

	for(;;) {
		if (!is_inside_code_area(cpu, pc))
			return;
		if (is_code(cpu, pc))	/* we have already been here, ignore */
			return;

#ifdef VERBOSE
		for (int i=0; i<level; i++) printf(" ");
//		disasm_instr(cpu, pc);
#endif

		or_tagging_type(cpu, pc, TAG_TYPE_CODE);

		bytes = cpu->f.tag_instr(cpu, pc, &flow_type, &new_pc);
		
		switch (flow_type) {
			case FLOW_TYPE_ERR:
			case FLOW_TYPE_RET:
				/* execution ends here, the follwing location is not reached */
				return;
			case FLOW_TYPE_JUMP:
				/* continue tagging at target of jump */
				or_tagging_type(cpu, new_pc, TAG_TYPE_BRANCH_TARGET);
				pc = new_pc;
				continue;
			case FLOW_TYPE_BRANCH:
				/* tag target of branch, then continue with next instruction */
				or_tagging_type(cpu, new_pc, TAG_TYPE_BRANCH_TARGET);
				or_tagging_type(cpu, pc+bytes, TAG_TYPE_AFTER_BRANCH);
				tag_recursive(cpu, new_pc, level+1);
				break;
			case FLOW_TYPE_CALL:
				/* tag subroutine, then continue with next instruction */
				or_tagging_type(cpu, new_pc, TAG_TYPE_SUBROUTINE);
				or_tagging_type(cpu, pc+bytes, TAG_TYPE_AFTER_CALL);
				tag_recursive(cpu, new_pc, level+1);
				break;
			case FLOW_TYPE_CONTINUE:
				break; /* continue with next instruction */
		}
		pc += bytes;
	}
}

void
cpu_tag(cpu_t *cpu, addr_t pc)
{
	/* for singlestep, we don't need this */
	if (cpu->flags_debug & CPU_DEBUG_SINGLESTEP)
		return;

	/* initialize data structure on demand */
	if (!cpu->tagging_type)
		init_tagging(cpu);

#if VERBOSE
	printf("starting tagging at $%02llx\n", (unsigned long long)pc);
#endif

	or_tagging_type(cpu, pc, TAG_TYPE_ENTRY); /* client wants to enter the guest code here */
	tag_recursive(cpu, pc, 0);
}

