#include "libcpu.h"
#include "types.h"
#include "6502_isa.h"
#include "cpu_generic.h"
#include "libcpu_6502.h"

using namespace llvm;

//XXX put into cpu_t
Value* ptr_C;
Value* ptr_Z;
Value* ptr_I;
Value* ptr_D;
Value* ptr_V;
Value* ptr_N;

#define A 0
#define X 1
#define Y 2
#define S 3
#define P 4
#define ptr_A cpu->ptr_r8[A]
#define ptr_X cpu->ptr_r8[X]
#define ptr_Y cpu->ptr_r8[Y]
#define ptr_S cpu->ptr_r8[S]
#define ptr_P cpu->ptr_r8[P]

#define OPCODE cpu->RAM[pc]
#define OPERAND_8 cpu->RAM[(pc+1)&0xFFFF]
#define OPERAND_16 ((cpu->RAM[pc+1]&0xFFFF) | (cpu->RAM[pc+2]&0xFFFF)<<8)

#define SET_NZ(a) arch_6502_set_nz(a, bb)
#define OPERAND arch_6502_get_operand_rvalue(cpu, pc, bb)
#define LOPERAND arch_6502_get_operand_lvalue(cpu, pc, bb)
#define LET1(a,b) new StoreInst(b, a, false, bb)

static inline Value *
arch_6502_get_op8(cpu_t *cpu, uint16_t pc, BasicBlock *bb) {
	return ConstantInt::get(getType(Int32Ty), OPERAND_8);
}

static inline Value *
arch_6502_get_op16(cpu_t *cpu, uint16_t pc, BasicBlock *bb) {
	return ConstantInt::get(getType(Int32Ty), OPERAND_16);
}

static inline Value *
arch_6502_get_x(cpu_t *cpu, BasicBlock *bb) {
	return new LoadInst(ptr_X, "", false, bb);
}

static inline Value *
arch_6502_get_y(cpu_t *cpu, BasicBlock *bb) {
	return new LoadInst(ptr_Y, "", false, bb);
}

static inline Value *
arch_6502_load_ram_8(cpu_t *cpu, Value *addr, BasicBlock *bb) {
	Value* ptr = GetElementPtrInst::Create(cpu->ptr_RAM, addr, "", bb);
	return new LoadInst(ptr, "", false, bb);
}

static Value *
arch_6502_load_ram_16(cpu_t *cpu, int is_32, Value *addr, BasicBlock *bb) {
	ConstantInt* const_int32_0001 = ConstantInt::get(getType(Int32Ty), 0x0001);
	ConstantInt* const_int32_0008 = ConstantInt::get(is_32? getType(Int32Ty) : getType(Int16Ty), 0x0008);

	/* get lo */
	Value *lo = arch_6502_load_ram_8(cpu, addr, bb);
	Value *lo32 = new ZExtInst(lo, getIntegerType(is_32? 32:16), "", bb);

	/* get hi */
	addr = BinaryOperator::Create(Instruction::Add, addr, const_int32_0001, "", bb);
	Value *hi = arch_6502_load_ram_8(cpu, addr, bb);
	Value *hi32 = new ZExtInst(hi, getIntegerType(is_32? 32:16), "", bb);

	/* combine */
	BinaryOperator* hi32shifted = BinaryOperator::Create(Instruction::Shl, hi32, const_int32_0008, "", bb);
	return BinaryOperator::Create(Instruction::Add, lo32, hi32shifted, "", bb);
}

static Value *
arch_6502_add_index(Value *ea, Value *index_register, BasicBlock *bb) {
	/* load index register, extend to 32 bit */
	Value *index = new LoadInst(index_register, "", false, bb);
	CastInst* index32 = new ZExtInst(index, getIntegerType(32), "", bb);

	/* add base and index */
	return BinaryOperator::Create(Instruction::Add, index32, ea, "", bb);
}

static Value *
arch_6502_get_operand_lvalue(cpu_t *cpu, addr_t pc, BasicBlock* bb) {
	int am = instraddmode[OPCODE].addmode;
	Value *index_register_before;
	Value *index_register_after;
	bool is_indirect;
	bool is_8bit;

	switch (am) {
		case ADDMODE_ACC:
			return ptr_A;
		case ADDMODE_BRA:
		case ADDMODE_IMM:
		case ADDMODE_IMPL:
			return NULL;
	}

	is_indirect = ((am == ADDMODE_IND) || (am == ADDMODE_INDX) || (am == ADDMODE_INDY));
	is_8bit = !((am == ADDMODE_ABS) || (am == ADDMODE_ABSX) || (am == ADDMODE_ABSY));
	index_register_before = NULL;
	if ((am == ADDMODE_ABSX) || (am == ADDMODE_INDX) || (am == ADDMODE_ZPX))
		index_register_before = ptr_X;
	if ((am == ADDMODE_ABSY) || (am == ADDMODE_ZPY))
		index_register_before = ptr_Y;
	index_register_after = (am == ADDMODE_INDY)? ptr_Y : NULL;

#if 0
	printf("pc = %x\n", pc);
	printf("index_register_before = %x\n", index_register_before);
	printf("index_register_after = %x\n", index_register_after);
	printf("is_indirect = %x\n", is_indirect);
	printf("is_8bit = %x\n", is_8bit);
#endif
	ConstantInt* const_int32_FFFF = ConstantInt::get(getType(Int32Ty), 0xFFFF);
	ConstantInt* const_int32_00FF = ConstantInt::get(getType(Int32Ty), 0x00FF);

	/* create base constant */
	uint16_t base = is_8bit? (OPERAND_8):(OPERAND_16);
	Value *ea = ConstantInt::get(getType(Int32Ty), base);

	if (index_register_before)
		ea = arch_6502_add_index(ea, index_register_before, bb);

	/* wrap around in zero page */
	if (is_8bit)
		ea = BinaryOperator::Create(Instruction::And, ea, const_int32_00FF, "", bb);
	else if (base >= 0xFF00) /* wrap around in memory */
		ea = BinaryOperator::Create(Instruction::And, ea, const_int32_FFFF, "", bb);

	if (is_indirect)
		ea = arch_6502_load_ram_16(cpu, true, ea, bb);

	if (index_register_after)
		ea = arch_6502_add_index(ea, index_register_after, bb);

	return GetElementPtrInst::Create(cpu->ptr_RAM, ea, "", bb);
}

static Value *
arch_6502_get_operand_rvalue(cpu_t *cpu, addr_t pc, BasicBlock* bb)
{
	switch (instraddmode[OPCODE].addmode) {
		case ADDMODE_IMM:
			return ConstantInt::get(getType(Int8Ty), OPERAND_8);
		default:
			Value *lvalue = arch_6502_get_operand_lvalue(cpu, pc, bb);
			return new LoadInst(lvalue, "", false, bb);
	}
}

static void
arch_6502_set_nz(Value *data, BasicBlock *bb)
{
	ConstantInt* const_int8_00 = ConstantInt::get(getType(Int8Ty), 0x00);
	ICmpInst* z = new ICmpInst(*bb, ICmpInst::ICMP_EQ, data, const_int8_00);
	ICmpInst* n = new ICmpInst(*bb, ICmpInst::ICMP_SLT, data, const_int8_00);
	new StoreInst(z, ptr_Z, bb);
	new StoreInst(n, ptr_N, bb);
}

static void
arch_6502_copy_reg(Value *src, Value *dst, BasicBlock *bb)
{
	Value *v = new LoadInst(src, "", false, bb);
	new StoreInst(v, dst, bb);
	arch_6502_set_nz(v, bb);
}

static void
arch_6502_store_reg(cpu_t *cpu, addr_t pc, Value *src, BasicBlock *bb)
{
	Value *lvalue = arch_6502_get_operand_lvalue(cpu, pc, bb);
	Value *v = new LoadInst(src, "", false, bb);
	new StoreInst(v, lvalue, bb);
}

static void
arch_6502_load_reg(cpu_t *cpu, addr_t pc, Value *dst, BasicBlock *bb)
{
	Value *rvalue = arch_6502_get_operand_rvalue(cpu, pc, bb);
	new StoreInst(rvalue, dst, bb);
	arch_6502_set_nz(rvalue, bb);
}

static void
arch_6502_log(cpu_t *cpu, addr_t pc, Instruction::BinaryOps o, BasicBlock *bb)
{
	Value *v1 = arch_6502_get_operand_rvalue(cpu, pc, bb);
	Value *v2 = new LoadInst(ptr_A, "", false, bb);
	v1 = BinaryOperator::Create(o, v1, v2, "", bb);
	new StoreInst(v1, ptr_A, bb);
	arch_6502_set_nz(v1, bb);
}

static void
arch_6502_trap(cpu_t *cpu, addr_t pc, BasicBlock *bb)
{
	ConstantInt* v_pc = ConstantInt::get(getType(Int16Ty), pc);
	new StoreInst(v_pc, cpu->ptr_PC, bb);
	ReturnInst::Create(_CTX(), ConstantInt::get(getType(Int32Ty), JIT_RETURN_TRAP), bb);//XXX needs #define
}

static void
arch_6502_rmw(cpu_t *cpu, uint16_t pc, Instruction::BinaryOps o, Value *c, BasicBlock *bb) {
	Value *lvalue = arch_6502_get_operand_lvalue(cpu, pc, bb);
	Value *v = new LoadInst(lvalue, "", false, bb);
	v = BinaryOperator::Create(o, v, c, "", bb);
	new StoreInst(v, lvalue, bb);
	arch_6502_set_nz(v, bb);
}

static void
arch_6502_shiftrotate(cpu_t *cpu, uint16_t pc, bool left, bool rotate, BasicBlock *bb)
{
	ConstantInt* const_int8_0000 = ConstantInt::get(getType(Int8Ty), 0x0000);
	ConstantInt* const_int8_0001 = ConstantInt::get(getType(Int8Ty), 0x0001);
	ConstantInt* const_int8_0007 = ConstantInt::get(getType(Int8Ty), 0x0007);

	/* load operand */
	Value *lvalue = arch_6502_get_operand_lvalue(cpu, pc, bb);
	Value *v1 = new LoadInst(lvalue, "", false, bb);

	/* shift */
	Value *v2 = BinaryOperator::Create(left? Instruction::Shl : Instruction::LShr, v1, const_int8_0001, "", bb);
	
	if (rotate) {	/* shift in carry */
		/* zext carry to i8 */
		Value *c = new LoadInst(ptr_C, "", false, bb);
		c = new ZExtInst(c, getIntegerType(8), "", bb);
		if (!left)
			c = BinaryOperator::Create(Instruction::Shl, c, const_int8_0007, "", bb);
		v2 = BinaryOperator::Create(Instruction::Or, v2, c, "", bb);
	}

	/* store */	
	new StoreInst(v2, lvalue, false, bb);
	arch_6502_set_nz(v2, bb);

	Value *c;
	if (left)	/* old MSB to carry */
		c = new ICmpInst(*bb, ICmpInst::ICMP_SLT, v1, const_int8_0000);
	else		/* old LSB to carry */
		c = new TruncInst(v1, getIntegerType(1), "", bb);
	new StoreInst(c, ptr_C, bb);
}

/*
 * This is used for ADC, SBC and CMP
 * - for ADC, we add A + B + C
 * - for SBC, we add A + ~B + C
 * - for CMP, we add A + ~B + 1
 * XXX TODO: consider changing code to avoid 16 bit arithmetic:
 *     while this works ok for 8 bit, it doesn't scale. M88K and ARM
 *     do it differently already.
 */
static void
arch_6502_addsub(cpu_t *cpu, uint16_t pc, Value *reg, Value *reg2, int is_sub, int with_carry, BasicBlock *bb) {
	ConstantInt* const_int16_0001 = ConstantInt::get(getType(Int16Ty), 0x0001);
	ConstantInt* const_int16_0008 = ConstantInt::get(getType(Int16Ty), 0x0008);
	ConstantInt* const_int8_00FF = ConstantInt::get(getType(Int8Ty), 0x00FF);

	Value *old_c = NULL; //XXX GCC

	/* load operand, A and C */
	Value *v1 = new LoadInst(reg, "", false, bb);
	Value *v2 = arch_6502_get_operand_rvalue(cpu, pc, bb);

	/* NOT operand (if subtraction) */
	if (is_sub)
		v2 = BinaryOperator::Create(Instruction::Xor, v2, const_int8_00FF, "", bb);

	/* convert to 16 bits */
	v1 = new ZExtInst(v1, getIntegerType(16), "", bb);
	v2 = new ZExtInst(v2, getIntegerType(16), "", bb);

	/* add them together */
	v1 = BinaryOperator::Create(Instruction::Add, v1, v2, "", bb);

	/* add C or 1 */
	if (with_carry) {
		old_c = new LoadInst(ptr_C, "", false, bb);
		old_c = new ZExtInst(old_c, getIntegerType(16), "", bb);
		v1 = BinaryOperator::Create(Instruction::Add, v1, old_c, "", bb);
	} else {
		v1 = BinaryOperator::Create(Instruction::Add, v1, const_int16_0001, "", bb);
	}

	/* get C */
	Value *c = BinaryOperator::Create(Instruction::LShr, v1, const_int16_0008, "", bb);
	c = new TruncInst(c, getIntegerType(1), "", bb);
	new StoreInst(c, ptr_C, bb);

	/* get result */
	v1 = new TruncInst(v1, getIntegerType(8), "", bb);
	if (reg2)
		new StoreInst(v1, reg2, bb);

	/* set flags */
	arch_6502_set_nz(v1, bb);
}

static void
arch_6502_push(cpu_t *cpu, Value *v, BasicBlock *bb) {
	ConstantInt* const_int32_0100 = ConstantInt::get(getType(Int32Ty), 0x0100);
	ConstantInt* const_int8_0001 = ConstantInt::get(getType(Int8Ty), 0x0001);

	/* get pointer to TOS */
	Value *s = new LoadInst(ptr_S, "", false, bb);
	Value *s_ptr = new ZExtInst(s, getIntegerType(32), "", bb);
	s_ptr = BinaryOperator::Create(Instruction::Or, s_ptr, const_int32_0100, "", bb);
	s_ptr = GetElementPtrInst::Create(cpu->ptr_RAM, s_ptr, "", bb);

	/* store value */
	new StoreInst(v, s_ptr, false, bb);

	/* update S */
	s = BinaryOperator::Create(Instruction::Sub, s, const_int8_0001, "", bb);
	new StoreInst(s, ptr_S, false, bb);
}

static Value *
arch_6502_pull(cpu_t *cpu, BasicBlock *bb) {
	ConstantInt* const_int32_0100 = ConstantInt::get(getType(Int32Ty), 0x0100);
	ConstantInt* const_int8_0001 = ConstantInt::get(getType(Int8Ty), 0x0001);

	/* update S */
	Value *s = new LoadInst(ptr_S, "", false, bb);
	s = BinaryOperator::Create(Instruction::Add, s, const_int8_0001, "", bb);
	new StoreInst(s, ptr_S, false, bb);

	/* get pointer to TOS */
	Value *s_ptr = new ZExtInst(s, getIntegerType(32), "", bb);
	s_ptr = BinaryOperator::Create(Instruction::Or, s_ptr, const_int32_0100, "", bb);
	s_ptr = GetElementPtrInst::Create(cpu->ptr_RAM, s_ptr, "", bb);

	/* load value */
	return new LoadInst(s_ptr, "", false, bb);
}

//XXX this generates inefficient IR code - worth fixing?
//	store i8 %408, i8* %S
//	%409 = load i8* %S
static void
arch_6502_push_c16(cpu_t *cpu, uint16_t v, BasicBlock *bb) {
	arch_6502_push(cpu, ConstantInt::get(getType(Int8Ty), v >> 8), bb);
	arch_6502_push(cpu, ConstantInt::get(getType(Int8Ty), v & 0xFF), bb);
}

#define N_SHIFT 7
#define V_SHIFT 6
#define D_SHIFT 3
#define I_SHIFT 2
#define Z_SHIFT 1
#define C_SHIFT 0

static Value *
arch_6502_flags_encode(BasicBlock *bb)
{
	Value *flags = ConstantInt::get(getIntegerType(8), 0);

	flags = arch_encode_bit(flags, ptr_N, N_SHIFT, 8, bb);
	flags = arch_encode_bit(flags, ptr_V, V_SHIFT, 8, bb);
	flags = arch_encode_bit(flags, ptr_D, D_SHIFT, 8, bb);
	flags = arch_encode_bit(flags, ptr_I, I_SHIFT, 8, bb);
	flags = arch_encode_bit(flags, ptr_Z, Z_SHIFT, 8, bb);
	flags = arch_encode_bit(flags, ptr_C, C_SHIFT, 8, bb);

	return flags;
}

static void
arch_6502_flags_decode(Value *flags, BasicBlock *bb)
{
	arch_decode_bit(flags, ptr_N, N_SHIFT, 8, bb);
	arch_decode_bit(flags, ptr_V, V_SHIFT, 8, bb);
	arch_decode_bit(flags, ptr_D, D_SHIFT, 8, bb);
	arch_decode_bit(flags, ptr_I, I_SHIFT, 8, bb);
	arch_decode_bit(flags, ptr_Z, Z_SHIFT, 8, bb);
	arch_decode_bit(flags, ptr_C, C_SHIFT, 8, bb);
}

Value *
arch_6502_recompile_cond(cpu_t *cpu, addr_t pc, BasicBlock *bb) {
	uint8_t opcode = cpu->RAM[pc];
printf("%s:%d pc=%llx opcode=%x\n", __func__, __LINE__, pc, opcode);

	switch (instraddmode[opcode].instr) {
		case INSTR_BEQ: /* Z */		return LOAD(ptr_Z);
		case INSTR_BNE: /* !Z */	return NOT(LOAD(ptr_Z));
		case INSTR_BCS: /* C */		return LOAD(ptr_C);
		case INSTR_BCC: /* !C */	return NOT(LOAD(ptr_C));
		case INSTR_BMI: /* N */		return LOAD(ptr_N);
		case INSTR_BPL: /* !N */	return NOT(LOAD(ptr_N));
		case INSTR_BVS: /* V */		return LOAD(ptr_V);
		case INSTR_BVC: /* !V */	return NOT(LOAD(ptr_V));
		default:					return NULL; /* no condition; should not be reached */
	}
}

static int
arch_6502_recompile_instr(cpu_t *cpu, addr_t pc, BasicBlock *bb) {
	uint8_t opcode = cpu->RAM[pc];

	ConstantInt* const_false = ConstantInt::get(getType(Int1Ty), 0);
	ConstantInt* const_true = ConstantInt::get(getType(Int1Ty), 1);
	ConstantInt* const_int8_0001 = ConstantInt::get(getType(Int8Ty), 0x0001);

//printf("%s:%d PC=$%04X\n", __func__, __LINE__, pc);

#if 0 //XXX this must move into generic code
	// add a call to debug_function()
	ConstantInt* v_pc = ConstantInt::get(Type::Int16Ty, pc);
	new StoreInst(v_pc, cpu->ptr_PC, bb);
	// serialize flags
	Value *flags = arch_6502_flags_encode(bb);
	new StoreInst(flags, ptr_P, false, bb);

	create_call(cpu, ptr_func_debug, bb);

	flags = new LoadInst(ptr_P, "", false, bb);
	arch_6502_flags_decode(flags, bb);
#endif

//	printf("\naddmode = %i\n", instraddmode[opcode].addmode);
	switch (instraddmode[opcode].instr) {
		/* flags */
		case INSTR_CLC:	LET1(ptr_C, FALSE);				break;
		case INSTR_CLD:	LET1(ptr_D, FALSE);				break;
		case INSTR_CLI:	LET1(ptr_I, FALSE);				break;
		case INSTR_CLV:	LET1(ptr_V, FALSE);				break;
		case INSTR_SEC:	LET1(ptr_C, TRUE);				break;
		case INSTR_SED:	LET1(ptr_D, TRUE);				break;
		case INSTR_SEI:	LET1(ptr_I, TRUE);				break;

		/* register transfer */
		case INSTR_TAX:	SET_NZ(LET(X,R(A)));			break;
		case INSTR_TAY:	SET_NZ(LET(Y,R(A)));			break;
		case INSTR_TXA:	SET_NZ(LET(A,R(X)));			break;
		case INSTR_TYA:	SET_NZ(LET(A,R(Y)));			break;
		case INSTR_TSX:	SET_NZ(LET(X,R(S)));			break;
		case INSTR_TXS:	SET_NZ(LET(S,R(X)));			break;

		/* load */
		case INSTR_LDA:	SET_NZ(LET(A,OPERAND));			break;
		case INSTR_LDX:	SET_NZ(LET(X,OPERAND));			break;
		case INSTR_LDY:	SET_NZ(LET(Y,OPERAND));			break;

		/* store */
		case INSTR_STA:
			arch_6502_store_reg(cpu, pc, ptr_A, bb);
			break;
		case INSTR_STX:
			arch_6502_store_reg(cpu, pc, ptr_X, bb);
			break;
		case INSTR_STY:
			arch_6502_store_reg(cpu, pc, ptr_Y, bb);
			break;

		/* stack */
		case INSTR_PHA:
			arch_6502_push(cpu, new LoadInst(ptr_A, "", false, bb), bb);
			break;
		case INSTR_PHP:
			arch_6502_push(cpu, arch_6502_flags_encode(bb), bb);
			break;
		case INSTR_PLA:
			{
			Value *v1 = arch_6502_pull(cpu, bb);
			new StoreInst(v1, ptr_A, bb);
			arch_6502_set_nz(v1, bb);
			break;
			}
		case INSTR_PLP:
			arch_6502_flags_decode(arch_6502_pull(cpu, bb), bb);
			break;

		/* shift */
		case INSTR_ASL:
			arch_6502_shiftrotate(cpu, pc, true, false, bb);
			break;
		case INSTR_LSR:
			arch_6502_shiftrotate(cpu, pc, false, false, bb);
			break;
		case INSTR_ROL:
			arch_6502_shiftrotate(cpu, pc, true, true, bb);
			break;
		case INSTR_ROR:
			arch_6502_shiftrotate(cpu, pc, false, true, bb);
			break;

		/* bit logic */
		case INSTR_AND:	SET_NZ(LET(A,AND(R(A),OPERAND)));			break;
		case INSTR_ORA:	SET_NZ(LET(A,OR(R(A),OPERAND)));			break;
		case INSTR_EOR:	SET_NZ(LET(A,XOR(R(A),OPERAND)));			break;
		case INSTR_BIT:	SET_NZ(OPERAND);							break;

		/* arithmetic */
		case INSTR_ADC:
			arch_6502_addsub(cpu, pc, ptr_A, ptr_A, false, true, bb);
			break;
		case INSTR_SBC:
			arch_6502_addsub(cpu, pc, ptr_A, ptr_A, true, true, bb);
			break;
		case INSTR_CMP:
			arch_6502_addsub(cpu, pc, ptr_A, NULL, true, false, bb);
			break;
		case INSTR_CPX:
			arch_6502_addsub(cpu, pc, ptr_X, NULL, true, false, bb);
			break;
		case INSTR_CPY:
			arch_6502_addsub(cpu, pc, ptr_Y, NULL, true, false, bb);
			break;

		/* increment/decrement */
		case INSTR_DEC:
			arch_6502_rmw(cpu, pc, Instruction::Sub, const_int8_0001, bb);
			break;
		case INSTR_DEX:	SET_NZ(LET(X,SUB(R(X),CONST(1))));			break;
		case INSTR_DEY:	SET_NZ(LET(Y,SUB(R(Y),CONST(1))));			break;
		case INSTR_INX:	SET_NZ(LET(X,ADD(R(X),CONST(1))));			break;
		case INSTR_INY:	SET_NZ(LET(Y,ADD(R(Y),CONST(1))));			break;
		case INSTR_INC:
			arch_6502_rmw(cpu, pc, Instruction::Add, const_int8_0001, bb);
			break;

		/* control flow */
		case INSTR_JMP:
			if (instraddmode[opcode].addmode == ADDMODE_IND) {
				Value *ea = ConstantInt::get(getType(Int32Ty), OPERAND_16);
				Value *v = arch_6502_load_ram_16(cpu, false, ea, bb);
				new StoreInst(v, cpu->ptr_PC, bb);
			}
			break;
		case INSTR_JSR:
			arch_6502_push_c16(cpu, pc+2, bb);
			break;
		case INSTR_RTS:
			{
			ConstantInt* const_int16_0008 = ConstantInt::get(getType(Int16Ty), 0x0008);
			ConstantInt* const_int16_0001 = ConstantInt::get(getType(Int16Ty), 0x0001);
			Value *lo = arch_6502_pull(cpu, bb);
			Value *hi = arch_6502_pull(cpu, bb);
			lo = new ZExtInst(lo, getIntegerType(16), "", bb);
			hi = new ZExtInst(hi, getIntegerType(16), "", bb);
			hi = BinaryOperator::Create(Instruction::Shl, hi, const_int16_0008, "", bb);
			lo = BinaryOperator::Create(Instruction::Add, lo, hi, "", bb);
			lo = BinaryOperator::Create(Instruction::Add, lo, const_int16_0001, "", bb);
			new StoreInst(lo, cpu->ptr_PC, bb);
			break;
			}
		case INSTR_RTI:
			printf("error: encountered RTI!\n");
			arch_6502_trap(cpu, pc, bb);
			break;

		/* branch */
		case INSTR_BEQ:
		case INSTR_BNE:
		case INSTR_BCS:
		case INSTR_BCC:
		case INSTR_BMI:
		case INSTR_BPL:
		case INSTR_BVS:
		case INSTR_BVC:
			break;

		/* other */
		case INSTR_BRK:
			printf("warning: encountered BRK!\n");
			arch_6502_trap(cpu, pc, bb);
			break;
		case INSTR_NOP:
			break;
		case INSTR_XXX:
			printf("warning: encountered XXX!\n");
			arch_6502_trap(cpu, pc, bb);
			break;
	}

//printf("%s:%d opcode=%02X, addmode=%d, length=%d\n", __func__, __LINE__, opcode, instraddmode[opcode].addmode, length[instraddmode[opcode].addmode]);
	return length[instraddmode[opcode].addmode]+1;
}

static void
arch_6502_init(cpu_t *cpu)
{
	reg_6502_t *reg;
	reg = (reg_6502_t*)malloc(sizeof(reg_6502_t));
	reg->pc = 0;
	reg->a = 0;
	reg->x = 0;
	reg->y = 0;
	reg->s = 0xFF;
	reg->p = 0;
	cpu->reg = reg;

	cpu->pc_width = 16;
	cpu->count_regs_i8 = 5;
	cpu->count_regs_i16 = 0;
	cpu->count_regs_i32 = 0;
	cpu->count_regs_i64 = 0;
	cpu->reg_size = 8;

	cpu->is_little_endian = true;
	cpu->fp_reg = NULL;
	cpu->count_regs_f32 = 0;
	cpu->count_regs_f64 = 0;
	cpu->count_regs_f80 = 0;
	cpu->count_regs_f128 = 0;

	assert(offsetof(reg_6502_t, pc) == 5);
}

static void
arch_6502_emit_decode_reg(cpu_t *cpu, BasicBlock *bb)
{
	// declare flags
	ptr_N = new AllocaInst(getIntegerType(1), "N", bb);
	ptr_V = new AllocaInst(getIntegerType(1), "V", bb);
	ptr_D = new AllocaInst(getIntegerType(1), "D", bb);
	ptr_I = new AllocaInst(getIntegerType(1), "I", bb);
	ptr_Z = new AllocaInst(getIntegerType(1), "Z", bb);
	ptr_C = new AllocaInst(getIntegerType(1), "C", bb);

	// decode P
	Value *flags = new LoadInst(ptr_P, "", false, bb);
	arch_6502_flags_decode(flags, bb);
}

static void
arch_6502_spill_reg_state(cpu_t *cpu, BasicBlock *bb)
{
	Value *flags = arch_6502_flags_encode(bb);
	new StoreInst(flags, ptr_P, false, bb);
}

static addr_t
arch_6502_get_pc(cpu_t *, void *reg)
{
	return ((reg_6502_t*)reg)->pc;
}

static uint64_t
arch_6502_get_psr(cpu_t *, void *reg)
{
	return ((reg_6502_t*)reg)->p;
}

static int
arch_6502_get_reg(cpu_t *cpu, void *reg, unsigned reg_no, uint64_t *value)
{
	switch (reg_no) {
		case 0: *value = ((reg_6502_t *)reg)->a; break;
		case 1: *value = ((reg_6502_t *)reg)->x; break;
		case 2: *value = ((reg_6502_t *)reg)->y; break;
		case 3: *value = ((reg_6502_t *)reg)->s; break;
		default: return (-1);
	}
	return (0);
}

arch_func_t arch_func_6502 = {
	arch_6502_init,
	arch_6502_get_pc,
	arch_6502_emit_decode_reg,
	arch_6502_spill_reg_state,
	arch_6502_tag_instr,
	arch_6502_disasm_instr,
	arch_6502_recompile_cond,
	arch_6502_recompile_instr,
	// idbg support
	arch_6502_get_psr,
	arch_6502_get_reg,
	NULL
};
