/*
 * libcpu: optimize.cpp
 *
 * Tell LLVM to run optimizers over the IR.
 */

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ModuleProvider.h"
#include "llvm/PassManager.h"
#include "llvm/Support/StandardPasses.h"
#include "llvm/Target/TargetData.h"

#include "libcpu.h"

void
optimize(cpu_t *cpu)
{
	FunctionPassManager pm = FunctionPassManager(cpu->mp);

	std::string data_layout = cpu->exec_engine->getTargetData()->getStringRepresentation();
	TargetData *TD = new TargetData(data_layout);
	pm.add(TD);
	pm.add(createPromoteMemoryToRegisterPass());
	pm.add(createInstructionCombiningPass());
	pm.add(createConstantPropagationPass());
	pm.add(createDeadCodeEliminationPass());
	pm.run(*cpu->cur_func);
}

