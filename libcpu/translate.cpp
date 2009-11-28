#include "libcpu.h"
#include "tag.h"
#include "basicblock.h"
/*
 * returns the basic block where code execution continues, or
 * NULL if the instruction always branches away
 * (The caller needs this to link the basic block)
 */
BasicBlock *
translate_instr(cpu_t *cpu, addr_t pc, tag_t tag,
	BasicBlock *bb_target,
	BasicBlock *bb_next,
	BasicBlock *bb_trap,
	BasicBlock *cur_bb)
{
	BasicBlock *bb_cond;
	BasicBlock *bb_delay;

	/* create internal basic blocks if needed */
	if (tag & TAG_CONDITIONAL)
		bb_cond = create_basicblock(cpu, pc, cpu->func_jitmain, BB_TYPE_COND);
	if ((tag & TAG_DELAY_SLOT) && (tag & TAG_CONDITIONAL))
		bb_delay = create_basicblock(cpu, pc, cpu->func_jitmain, BB_TYPE_DELAY);

	/* special case: delay slot */
	if (tag & TAG_DELAY_SLOT) {
		if (tag & TAG_CONDITIONAL) {
			addr_t delay_pc;
			// cur_bb:  if (cond) goto b_cond; else goto bb_delay;
			Value *c = cpu->f.translate_cond(cpu, pc, cur_bb);
			BranchInst::Create(bb_cond, bb_delay, c, cur_bb);
			// bb_cond: instr; delay; goto bb_target;
			pc += cpu->f.translate_instr(cpu, pc, bb_cond);
			delay_pc = pc;
			cpu->f.translate_instr(cpu, pc, bb_cond);
			BranchInst::Create(bb_target, bb_cond);
			// bb_cond: delay; goto bb_next;
			cpu->f.translate_instr(cpu, delay_pc, bb_delay);
			BranchInst::Create(bb_next, bb_delay);
		} else {
			// cur_bb:  instr; delay; goto bb_target;
			pc += cpu->f.translate_instr(cpu, pc, cur_bb);
			cpu->f.translate_instr(cpu, pc, cur_bb);
			BranchInst::Create(bb_target, cur_bb);
		}
		return NULL; /* don't link */
	}

	/* no delay slot */
	if (tag & TAG_CONDITIONAL) {
		// cur_bb:  if (cond) goto b_cond; else goto bb_next;
		addr_t delay_pc;
		Value *c = cpu->f.translate_cond(cpu, pc, cur_bb);
		BranchInst::Create(bb_cond, bb_next, c, cur_bb);
		cur_bb = bb_cond;
	}

	cpu->f.translate_instr(cpu, pc, cur_bb);

	if (tag & (TAG_BRANCH | TAG_CALL | TAG_RET))
		BranchInst::Create(bb_target, cur_bb);
	else if (tag & TAG_TRAP)
		BranchInst::Create(bb_trap, cur_bb);

	if (tag & TAG_CONTINUE)
		return cur_bb;
	else
		return NULL;
}

