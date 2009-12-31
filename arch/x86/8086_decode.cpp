#include "libcpu.h"
#include "8086_isa.h"
#include "8086_decode.h"

static unsigned long decode_table[256] = {
	/* 0x00 - 0x07 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x08 - 0x0F */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x10 - 0x17 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x18 - 0x1F */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x20 - 0x27 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x28 - 0x2F */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x30 - 0x37 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x38 - 0x3F */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x40 - 0x47 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x48 - 0x4F */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x50 - 0x57 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x58 - 0x5F */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x60 - 0x67 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x68 - 0x6F */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70 - 0x77 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x78 - 0x7F */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x80 - 0x87 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x88 - 0x8F */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x90 - 0x97 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x98 - 0x9F */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xA0 - 0xA7 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xA8 - 0xAF */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xB0 - 0xB7 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xB8 - 0xBF */
	INSTR_MOV|SrcImm16|DstReg, INSTR_MOV|SrcImm16|DstReg, INSTR_MOV|SrcImm16|DstReg, INSTR_MOV|SrcImm16|DstReg,
	0, 0, 0, 0,
	/* 0xC0 - 0xC7 */
	0, 0, 0, INSTR_RET|SrcNone|DstNone, 0, 0, 0, 0,
	/* 0xC8 - 0xCF */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xD0 - 0xD7 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xD8 - 0xDF */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xE0 - 0xE7 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xE8 - 0xEF */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xF0 - 0xF7 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xF8 - 0xFF */
	0, 0, 0, 0, 0, 0, 0, 0,
};

static void
decode_imm_operand(struct x86_instr *instr, struct x86_operand *operand, uint8_t imm_lo, uint8_t imm_hi)
{
	operand->type	= OP_IMM;
	operand->imm	= (imm_hi << 8) | imm_lo;

	instr->op_bytes += 2;
}

static void
decode_reg_operand(struct x86_instr *instr, struct x86_operand *operand)
{
	operand->type	= OP_REG;
	operand->reg	= instr->opcode & 0x03;
}

static void
decode_modrm_byte(struct x86_instr *instr, uint8_t modrm)
{
	instr->op_bytes++;
}

int
arch_8086_decode_instr(struct x86_instr *instr, uint8_t* RAM, addr_t pc)
{
	uint8_t opcode;

	instr->op_bytes = 1;

	/* Prefixes */
	for (;;) {
		switch (opcode = RAM[pc++]) {
		case 0x26:	/* ES override */
		case 0x2e:	/* CS override */
		case 0x36:	/* SS override */
		case 0x3e:	/* DS override */
		case 0xf2:	/* REPNE/REPNZ */
		case 0xf3:	/* REP/REPE/REPZ */
			instr->op_bytes++;
			break;
		default:
			goto done_prefixes;	
		}
	}

done_prefixes:

	/* Opcode byte */
	instr->opcode	= opcode;

	instr->flags	= decode_table[opcode];

	if (instr->flags == 0)	/* Unrecognized? */
		return -1;

	if (instr->flags & ModRM)
		decode_modrm_byte(instr, RAM[pc++]);

	switch (instr->flags & SrcMask) {
	case SrcNone:
		break;
	case SrcImm16:
		decode_imm_operand(instr, &instr->src, RAM[pc+0], RAM[pc+1]);
		break;
	}

	switch (instr->flags & DstMask) {
	case DstReg:
		decode_reg_operand(instr, &instr->dst);
		break;
	}

	return 0;
}

int
arch_8086_instr_length(struct x86_instr *instr)
{
	return instr->op_bytes;
}
