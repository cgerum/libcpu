#ifndef _LIBCPU_H_
#define _LIBCPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/PassManager.h"
#include "llvm/CallingConv.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ModuleProvider.h"
#include "llvm/Target/TargetData.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/LinkAllPasses.h"

#include "types.h"

using namespace llvm;

struct cpu;
typedef void        (*fp_init)(struct cpu *cpu);
typedef StructType *(*fp_get_struct_reg)(void);
typedef addr_t      (*fp_get_pc)(void *);
typedef void        (*fp_emit_decode_reg)(BasicBlock *bb);
typedef void        (*fp_spill_reg_state)(BasicBlock *bb);
typedef int         (*fp_tag_instr)(uint8_t* RAM, addr_t pc, int *flow_type, addr_t *new_pc);
typedef int         (*fp_disasm_instr)(uint8_t* RAM, addr_t pc, char *line, unsigned int max_line);
typedef int         (*fp_recompile_instr)(uint8_t* RAM, addr_t pc, BasicBlock *bb_dispatch, BasicBlock *bb);

typedef struct {
	fp_init init;
	fp_get_pc get_pc;
	fp_emit_decode_reg emit_decode_reg;
	fp_spill_reg_state spill_reg_state;
	fp_tag_instr tag_instr;
	fp_disasm_instr disasm_instr;
	fp_recompile_instr recompile_instr;
} arch_func_t;

typedef enum {
	CPU_ARCH_INVALID = 0, //XXX unused
	CPU_ARCH_6502,
	CPU_ARCH_M68K,
	CPU_ARCH_MIPS,
	CPU_ARCH_M88K,
	CPU_ARCH_ARM
} cpu_arch_t;

typedef uint8_t tagging_type_t;

typedef struct cpu {
	cpu_arch_t arch;
	arch_func_t f;
	uint32_t pc_width;
	uint32_t count_regs_i8;
	uint32_t count_regs_i16;
	uint32_t count_regs_i32;
	uint32_t count_regs_i64;
	const char *name;
	addr_t code_start;
	addr_t code_end;
	addr_t code_entry;
	uint64_t flags_optimize;
	uint32_t flags_debug;
	uint32_t flags_arch;
	uint32_t flags;
	tagging_type_t *tagging_type; /* array of flags, one per byte of code */
	Module *mod;
	Function *func_jitmain;
	ExecutionEngine *exec_engine;
	void *fp;
	void *reg;
} cpu_t;

enum {
	JIT_RETURN_NOERR = 0,
	JIT_RETURN_FUNCNOTFOUND
};

//////////////////////////////////////////////////////////////////////
// optimization flags
//////////////////////////////////////////////////////////////////////
#define CPU_OPTIMIZE_NONE 0x0000000000000000LL
#define CPU_OPTIMIZE_ALL  0xFFFFFFFFFFFFFFFFLL

//////////////////////////////////////////////////////////////////////
// debug flags
//////////////////////////////////////////////////////////////////////
#define CPU_DEBUG_NONE 0x00000000
#define CPU_DEBUG_SINGLESTEP			(1<<0)
#define CPU_DEBUG_PRINT_IR				(1<<1)
#define CPU_DEBUG_PRINT_IR_OPTIMIZED	(1<<1)
#define CPU_DEBUG_ALL 0xFFFFFFFF

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

/*
 * type of the debug callback; second parameter is
 * pointer to CPU specific register struct
 */
typedef void (*debug_function_t)(uint8_t*, void*);

//////////////////////////////////////////////////////////////////////

cpu_t *cpu_new(cpu_arch_t arch);
void cpu_set_flags_optimize(cpu_t *cpu, uint64_t f);
void cpu_set_flags_debug(cpu_t *cpu, uint32_t f);
void cpu_tag(cpu_t *cpu, addr_t pc);
int cpu_run(cpu_t *cpu, debug_function_t debug_function);
void cpu_set_flags_arch(cpu_t *cpu, uint32_t f);
void cpu_set_ram(uint8_t *RAM);
void cpu_flush(cpu_t *cpu);
void cpu_init(cpu_t *cpu);

#endif
