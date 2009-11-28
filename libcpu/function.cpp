/*
 * libcpu: function.cpp
 *
 * Create the master function and fill it with the helper
 * basic blocks
 */
//XXX this is MESSY!
#include "libcpu.h"

static StructType *
get_struct_reg(cpu_t *cpu) {
	std::vector<const Type*>type_struct_reg_t_fields;

	for (uint32_t i = 0; i < cpu->count_regs_i8; i++) /* 8 bit registers */
		type_struct_reg_t_fields.push_back(getIntegerType(8));
	for (uint32_t i = 0; i < cpu->count_regs_i16; i++) /* 16 bit registers */
		type_struct_reg_t_fields.push_back(getIntegerType(16));
	for (uint32_t i = 0; i < cpu->count_regs_i32; i++) /* 32 bit registers */
		type_struct_reg_t_fields.push_back(getIntegerType(32));
	for (uint32_t i = 0; i < cpu->count_regs_i64; i++) /* 64 bit registers */
		type_struct_reg_t_fields.push_back(getIntegerType(64));

	type_struct_reg_t_fields.push_back(getIntegerType(cpu->pc_width)); /* PC */

	return getStructType(type_struct_reg_t_fields, /*isPacked=*/true);
}

static StructType *
get_struct_fp_reg(cpu_t *cpu) {
	std::vector<const Type*>type_struct_fp_reg_t_fields;

	for (uint32_t i = 0; i < cpu->count_regs_f32; i++) /* 32 bit registers */
		type_struct_fp_reg_t_fields.push_back(getFloatType(32));
	for (uint32_t i = 0; i < cpu->count_regs_f64; i++) /* 64 bit registers */
		type_struct_fp_reg_t_fields.push_back(getFloatType(64));
	for (uint32_t i = 0; i < cpu->count_regs_f80; i++) { /* 80 bit registers */
		if (cpu->flags & CPU_FLAG_FP80)
			type_struct_fp_reg_t_fields.push_back(getFloatType(80));
		else {
			/* two 64bits words hold the data */
			type_struct_fp_reg_t_fields.push_back(getIntegerType(64));
			type_struct_fp_reg_t_fields.push_back(getIntegerType(64));
		}
	}
	for (uint32_t i = 0; i < cpu->count_regs_f128; i++) { /* 128 bit registers */
		if (cpu->flags & CPU_FLAG_FP128)
			type_struct_fp_reg_t_fields.push_back(getFloatType(128));
		else {
			/* two 64bits words hold the data */
			type_struct_fp_reg_t_fields.push_back(getIntegerType(64));
			type_struct_fp_reg_t_fields.push_back(getIntegerType(64));
		}
	}

	return getStructType(type_struct_fp_reg_t_fields, /*isPacked=*/true);
}

static Value *
get_struct_member_pointer(Value *s, int index, BasicBlock *bb) {
	ConstantInt* const_0 = ConstantInt::get(getType(Int32Ty), 0);
	ConstantInt* const_index = ConstantInt::get(getType(Int32Ty), index);

	SmallVector<Value*, 2> ptr_11_indices;
	ptr_11_indices.push_back(const_0);
	ptr_11_indices.push_back(const_index);
	return (Value*) GetElementPtrInst::Create(s, ptr_11_indices.begin(), ptr_11_indices.end(), "", bb);
}

static void
emit_decode_reg_helper(cpu_t *cpu, int count, int width, Value **in_ptr_r, Value **ptr_r, BasicBlock *bb) {
#ifdef OPT_LOCAL_REGISTERS
	// decode struct reg and copy the registers into local variables
	for (int i = 0; i < count; i++) {
		char reg_name[16];
		snprintf(reg_name, sizeof(reg_name), "gpr_%u", i);
		in_ptr_r[i] = get_struct_member_pointer(cpu->ptr_reg, i, bb);
		ptr_r[i] = new AllocaInst(getIntegerType(width), reg_name, bb);
		LoadInst* v = new LoadInst(in_ptr_r[i], "", false, bb);
		new StoreInst(v, ptr_r[i], false, bb);
	}
#else
	// just decode struct reg
	for (int i = 0; i < count; i++) 
		ptr_r[i] = get_struct_member_pointer(cpu->ptr_reg, i, bb);
#endif
}

static inline unsigned
fp_alignment(unsigned width) {
	return ((width == 80 ? 128 : width) >> 3);
}

static void
emit_decode_fp_reg_helper(cpu_t *cpu, int count, int width, Value **in_ptr_r,
	Value **ptr_r, BasicBlock *bb)
{
#ifdef OPT_LOCAL_REGISTERS
	// decode struct reg and copy the registers into local variables
	for (int i = 0; i < count; i++) {
		char reg_name[16];
		if ((width == 80 && (cpu->flags & CPU_FLAG_FP80) == 0) ||
			(width == 128 && (cpu->flags & CPU_FLAG_FP128) == 0)) {
			snprintf(reg_name, sizeof(reg_name), "fpr_%u_0", i);

			in_ptr_r[i*2+0] = get_struct_member_pointer(cpu->ptr_fp_reg, i*2+0, bb);
			ptr_r[i*2+0] = new AllocaInst(getIntegerType(64), 0, 0, reg_name, bb);
			LoadInst* v = new LoadInst(in_ptr_r[i*2+0], "", false, 0, bb);
			new StoreInst(v, ptr_r[i*2+0], false, 0, bb);

			snprintf(reg_name, sizeof(reg_name), "fpr_%u_1", i);

			in_ptr_r[i*2+1] = get_struct_member_pointer(cpu->ptr_fp_reg, i*2+1, bb);
			ptr_r[i*2+1] = new AllocaInst(getIntegerType(64), 0, 0, reg_name, bb);
			v = new LoadInst(in_ptr_r[i*2+1], "", false, 0, bb);
			new StoreInst(v, ptr_r[i*2+1], false, 0, bb);
		} else {
			snprintf(reg_name, sizeof(reg_name), "fpr_%u", i);
			in_ptr_r[i] = get_struct_member_pointer(cpu->ptr_fp_reg, i, bb);
			ptr_r[i] = new AllocaInst(getFloatType(width), 0, fp_alignment(width), reg_name, bb);
			LoadInst* v = new LoadInst(in_ptr_r[i], "", false, fp_alignment(width), bb);
			new StoreInst(v, ptr_r[i], false, fp_alignment(width), bb);
		}
	}
#else
	// just decode struct reg
	for (int i = 0; i < count; i++) 
		ptr_r[i] = get_struct_member_pointer(cpu->ptr_fp_reg, i, bb);
#endif
}

static void
emit_decode_reg(cpu_t *cpu, BasicBlock *bb)
{
	emit_decode_reg_helper(cpu, cpu->count_regs_i8,   8, cpu->in_ptr_r8,  cpu->ptr_r8,  bb);
	emit_decode_reg_helper(cpu, cpu->count_regs_i16, 16, cpu->in_ptr_r16, cpu->ptr_r16, bb);
	emit_decode_reg_helper(cpu, cpu->count_regs_i32, 32, cpu->in_ptr_r32, cpu->ptr_r32, bb);
	emit_decode_reg_helper(cpu, cpu->count_regs_i64, 64, cpu->in_ptr_r64, cpu->ptr_r64, bb);

	emit_decode_fp_reg_helper(cpu, cpu->count_regs_f32, 32, cpu->in_ptr_f32, cpu->ptr_f32, bb);
	emit_decode_fp_reg_helper(cpu, cpu->count_regs_f64, 64, cpu->in_ptr_f64, cpu->ptr_f64, bb);
	emit_decode_fp_reg_helper(cpu, cpu->count_regs_f80, 80, cpu->in_ptr_f80, cpu->ptr_f80, bb);
	emit_decode_fp_reg_helper(cpu, cpu->count_regs_f128, 128, cpu->in_ptr_f128, cpu->ptr_f128, bb);

	uint32_t pc_index = 
		cpu->count_regs_i8 +
		cpu->count_regs_i16+
		cpu->count_regs_i32+
		cpu->count_regs_i64;
	cpu->ptr_PC = get_struct_member_pointer(cpu->ptr_reg, pc_index, bb);

	if (cpu->f.emit_decode_reg) /* cpu specific part */
		cpu->f.emit_decode_reg(cpu, bb);
}

static void
spill_reg_state_helper(int count, Value **in_ptr_r, Value **ptr_r, BasicBlock *bb)
{
#ifdef OPT_LOCAL_REGISTERS
	for (int i=0; i<count; i++) {
		LoadInst* v = new LoadInst(ptr_r[i], "", false, bb);
		new StoreInst(v, in_ptr_r[i], false, bb);
	}
#endif
}

static void
spill_fp_reg_state_helper(cpu_t *cpu, int count, int width, Value **in_ptr_r,
	Value **ptr_r, BasicBlock *bb)
{
#ifdef OPT_LOCAL_REGISTERS
	for (int i=0; i<count; i++) {
		if ((width == 80 && (cpu->flags & CPU_FLAG_FP80) == 0) ||
			(width == 128 && (cpu->flags & CPU_FLAG_FP128) == 0)) {
			LoadInst* v = new LoadInst(ptr_r[i*2+0], "", false, 0, bb);
			new StoreInst(v, in_ptr_r[i*2+0], false, 0, bb);

			v = new LoadInst(ptr_r[i*2+1], "", false, 0, bb);
			new StoreInst(v, in_ptr_r[i*2+1], false, 0, bb);
		} else {
			LoadInst* v = new LoadInst(ptr_r[i], "", false, fp_alignment(width), bb);
			new StoreInst(v, in_ptr_r[i], false, fp_alignment(width), bb);
		}
	}
#endif
}

static void
spill_reg_state(cpu_t *cpu, BasicBlock *bb)
{
	if (cpu->f.spill_reg_state) /* cpu specific part */
		cpu->f.spill_reg_state(cpu, bb);

	spill_reg_state_helper(cpu->count_regs_i8,  cpu->in_ptr_r8,  cpu->ptr_r8,  bb);
	spill_reg_state_helper(cpu->count_regs_i16, cpu->in_ptr_r16, cpu->ptr_r16, bb);
	spill_reg_state_helper(cpu->count_regs_i32, cpu->in_ptr_r32, cpu->ptr_r32, bb);
	spill_reg_state_helper(cpu->count_regs_i64, cpu->in_ptr_r64, cpu->ptr_r64, bb);

	spill_fp_reg_state_helper(cpu, cpu->count_regs_f32, 32, cpu->in_ptr_f32, cpu->ptr_f32, bb);
	spill_fp_reg_state_helper(cpu, cpu->count_regs_f64, 64, cpu->in_ptr_f64, cpu->ptr_f64, bb);
	spill_fp_reg_state_helper(cpu, cpu->count_regs_f80, 80, cpu->in_ptr_f80, cpu->ptr_f80, bb);
	spill_fp_reg_state_helper(cpu, cpu->count_regs_f128, 128, cpu->in_ptr_f128, cpu->ptr_f128, bb);
}

Function*
cpu_create_function(cpu_t *cpu, const char *name,
	BasicBlock **p_bb_ret,
	BasicBlock **p_bb_trap,
	BasicBlock **p_label_entry)
{
	Function *func;

	// Type Definitions
	// - struct reg
	StructType *type_struct_reg_t = get_struct_reg(cpu);
	cpu->mod->addTypeName("struct.reg_t", type_struct_reg_t);
	// - struct reg *
	PointerType *type_pstruct_reg_t = PointerType::get(type_struct_reg_t, 0);
	// - struct fp_reg
	StructType *type_struct_fp_reg_t = get_struct_fp_reg(cpu);
	cpu->mod->addTypeName("struct.fp_reg_t", type_struct_fp_reg_t);
	// - struct fp_reg *
	PointerType *type_pstruct_fp_reg_t = PointerType::get(type_struct_fp_reg_t, 0);
	// - uint8_t *
	PointerType *type_pi8 = PointerType::get(getIntegerType(8), 0);
	// - intptr *
	PointerType *type_intptr = PointerType::get(cpu->exec_engine->getTargetData()->getIntPtrType(_CTX()), 0);
	// - (*f)(cpu_t *) [debug_function() function pointer]
	std::vector<const Type*>type_func_callout_args;
	type_func_callout_args.push_back(type_intptr);	/* intptr *cpu */
	FunctionType *type_func_callout = FunctionType::get(
		getType(VoidTy),	/* Result */
		type_func_callout_args,	/* Params */
		false);		      	/* isVarArg */
	cpu->type_pfunc_callout = PointerType::get(type_func_callout, 0);

	// - (*f)(uint8_t *, reg_t *, fp_reg_t *, (*)(...)) [jitmain() function pointer)
	std::vector<const Type*>type_func_args;
	type_func_args.push_back(type_pi8);				/* uint8_t *RAM */
	type_func_args.push_back(type_pstruct_reg_t);	/* reg_t *reg */
	type_func_args.push_back(type_pstruct_fp_reg_t);	/* fp_reg_t *fp_reg */
	type_func_args.push_back(cpu->type_pfunc_callout);	/* (*debug)(...) */
	FunctionType* type_func = FunctionType::get(
		getIntegerType(32),		/* Result */
		type_func_args,		/* Params */
		false);						/* isVarArg */

	// Function Declarations
	func = Function::Create(
		type_func,				/* Type */
		GlobalValue::ExternalLinkage,	/* Linkage */
		name, cpu->mod);				/* Name */
	func->setCallingConv(CallingConv::C);
	AttrListPtr func_PAL;
	{
		SmallVector<AttributeWithIndex, 4> Attrs;
		AttributeWithIndex PAWI;
		PAWI.Index = 1U; PAWI.Attrs = 0  | Attribute::NoCapture;
		Attrs.push_back(PAWI);
		PAWI.Index = 4294967295U; PAWI.Attrs = 0  | Attribute::NoUnwind;
		Attrs.push_back(PAWI);
		func_PAL = AttrListPtr::get(Attrs.begin(), Attrs.end());
	}
	func->setAttributes(func_PAL);

	// args
	Function::arg_iterator args = func->arg_begin();
	cpu->ptr_RAM = args++;
	cpu->ptr_RAM->setName("RAM");
	cpu->ptr_reg = args++;
	cpu->ptr_reg->setName("reg");	
	cpu->ptr_fp_reg = args++;
	cpu->ptr_fp_reg->setName("fp_reg");	
	cpu->ptr_func_debug = args++;
	cpu->ptr_func_debug->setName("debug");

	// entry basicblock
	BasicBlock *label_entry = BasicBlock::Create(_CTX(), "entry", func, 0);
	emit_decode_reg(cpu, label_entry);

	// create exit code
	Value *exit_code = new AllocaInst(getIntegerType(32), "exit_code", label_entry);
	// assume JIT_RETURN_FUNCNOTFOUND or JIT_RETURN_SINGLESTEP if in in single step.
	new StoreInst(ConstantInt::get(getType(Int32Ty),
					(cpu->flags_debug & CPU_DEBUG_SINGLESTEP) ? JIT_RETURN_SINGLESTEP :
					JIT_RETURN_FUNCNOTFOUND), exit_code, false, 0, label_entry);

#if 0 // bad for debugging, minimal speedup
	/* make the RAM pointer a constant */
	PointerType* type_pi8 = PointerType::get(IntegerType::get(8), 0);
	cpu->ptr_RAM = ConstantExpr::getCast(Instruction::IntToPtr, ConstantInt::get(Type::Int64Ty, (uint64_t)(long)cpu->RAM), type_pi8);
#endif

	// create ret basicblock
	BasicBlock *bb_ret = BasicBlock::Create(_CTX(), "ret", func, 0);  
	spill_reg_state(cpu, bb_ret);
	ReturnInst::Create(_CTX(), new LoadInst(exit_code, "", false, 0, bb_ret), bb_ret);
	// create trap return basicblock
	BasicBlock *bb_trap = BasicBlock::Create(_CTX(), "trap", func, 0);  
	new StoreInst(ConstantInt::get(getType(Int32Ty), JIT_RETURN_TRAP), exit_code, false, 0, bb_trap);
	// return
	BranchInst::Create(bb_ret, bb_trap);

	*p_bb_ret = bb_ret;
	*p_bb_trap = bb_trap;
	*p_label_entry = label_entry;
	return func;
}

