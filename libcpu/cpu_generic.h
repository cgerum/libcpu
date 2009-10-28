#include "libcpu.h"

extern Value* ptr_RAM;

extern Value* ptr_r8[32];
extern Value* ptr_r16[32];
extern Value* ptr_r32[32];
extern Value* ptr_r64[32];

extern uint32_t reg_size;
extern bool is_little_endian;
extern bool has_special_r0;

/* emitter functions */
Value *arch_get_reg(uint32_t index, uint32_t bits, BasicBlock *bb);
void arch_put_reg(uint32_t index, Value *v, uint32_t bits, bool sext, BasicBlock *bb);
Value *arch_load32_aligned(Value *a, BasicBlock *bb);
void arch_store32_aligned(Value *v, Value *a, BasicBlock *bb);
Value *arch_load8(Value *addr, BasicBlock *bb);
Value *arch_load16_aligned(Value *addr, BasicBlock *bb);
void arch_store8(Value *val, Value *addr, BasicBlock *bb);

/* host functions */
uint32_t RAM32BE(uint8_t *RAM, addr_t a);

/*
 * a collection of preprocessor macros
 * that make the LLVM interface nicer
 */

#define CONSTs(s,v) ConstantInt::get(IntegerType::get(s), v)
#define CONST8(v) CONSTs(8,v)
#define CONST16(v) CONSTs(16,v)
#define CONST32(v) CONSTs(32,v)
#define CONST64(v) CONSTs(64,v)

#define CONST(v) CONSTs(reg_size,v)

#define TRUNC(s,v) new TruncInst(v, IntegerType::get(s), "", bb)
#define TRUNC8(v) TRUNC(8,v)
#define TRUNC16(v) TRUNC(16,v)
#define TRUNC32(v) TRUNC(32,v)

#define ZEXT(s,v) new ZExtInst(v, IntegerType::get(s), "", bb)
#define ZEXT8(v) ZEXT(8,v)
#define ZEXT16(v) ZEXT(16,v)
#define ZEXT32(v) ZEXT(32,v)

#define SEXT(s,v) new SExtInst(v, IntegerType::get(s), "", bb)
#define SEXT8(v) SEXT(8,v)
#define SEXT16(v) SEXT(16,v)
#define SEXT32(v) SEXT(32,v)

#define ADD(a,b) BinaryOperator::Create(Instruction::Add, a, b, "", bb)
#define SUB(a,b) BinaryOperator::Create(Instruction::Sub, a, b, "", bb)
#define AND(a,b) BinaryOperator::Create(Instruction::And, a, b, "", bb)
#define OR(a,b) BinaryOperator::Create(Instruction::Or, a, b, "", bb)
#define XOR(a,b) BinaryOperator::Create(Instruction::Xor, a, b, "", bb)
#define SHL(a,b) BinaryOperator::Create(Instruction::Shl, a, b, "", bb)
#define LSHR(a,b) BinaryOperator::Create(Instruction::LShr, a, b, "", bb)
#define ASHR(a,b) BinaryOperator::Create(Instruction::AShr, a, b, "", bb)
#define ICMP_EQ(a,b) new ICmpInst(ICmpInst::ICMP_EQ, a, b, "", bb)
#define ICMP_NE(a,b) new ICmpInst(ICmpInst::ICMP_NE, a, b, "", bb)
#define ICMP_ULT(a,b) new ICmpInst(ICmpInst::ICMP_ULT, a, b, "", bb)
#define ICMP_SLT(a,b) new ICmpInst(ICmpInst::ICMP_SLT, a, b, "", bb)
#define ICMP_SGT(a,b) new ICmpInst(ICmpInst::ICMP_SGT, a, b, "", bb)
#define ICMP_SGE(a,b) new ICmpInst(ICmpInst::ICMP_SGE, a, b, "", bb)
#define ICMP_SLE(a,b) new ICmpInst(ICmpInst::ICMP_SLE, a, b, "", bb)

/* interface to the GPRs */
#define R(i) arch_get_reg(i, 0, bb)
#define R32(i) arch_get_reg(i, 32, bb)

#define LET(i,v) arch_put_reg(i, v, 0, false, bb)
#define LET32(i,v) arch_put_reg(i, v, 32, true, bb)
#define LET_ZEXT(i,v) arch_put_reg(i, v, 1, false, bb)

/* interface to memory */
#define LOAD8(i,v) arch_put_reg(i, arch_load8(v,bb), 8, false, bb)
#define LOAD8S(i,v) arch_put_reg(i, arch_load8(v,bb), 8, true, bb)
#define LOAD16(i,v) arch_put_reg(i, arch_load16_aligned(v,bb), 16, false, bb)
#define LOAD16S(i,v) arch_put_reg(i, arch_load16_aligned(v,bb), 16, true, bb)
#define LOAD32(i,v) arch_put_reg(i, arch_load32_aligned(v,bb), 32, true, bb)

#define STORE8(v,a) arch_store8(v, a, bb)
#define STORE32(v,a) arch_store32_aligned(v, a, bb)

/* host */
#define RAM32(RAM,a) RAM32BE(RAM,a)