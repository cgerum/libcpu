#include "libcpu.h"
#include "x86_isa.h"
#include "x86_decode.h"

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
	0, INSTR_MOV|ModRM|SrcReg, 0, 0, 0, 0, 0, 0,
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
decode_dst_operand(struct x86_instr *instr)
{
	struct x86_operand *operand = &instr->dst;

	switch (instr->flags & DstMask) {
	case DstNone:
		break;
	case DstReg:
		operand->type	= OP_REG;

		if (instr->flags & ModRM)
			operand->reg	= instr->rm;
		else
			operand->reg	= instr->opcode & 0x03;
		break;
	case DstMem:
		operand->type	= OP_MEM;
		operand->reg	= instr->rm;
		break;
	case DstMemDisp8:
	case DstMemDisp16:
		operand->type	= OP_MEM_DISP;
		operand->reg	= instr->rm;
		operand->disp	= instr->disp;
		break;
	}
}

static void
decode_src_operand(struct x86_instr *instr)
{
	struct x86_operand *operand = &instr->src;

	switch (instr->flags & SrcMask) {
	case SrcNone:
		break;
	case SrcImm16:
		operand->type	= OP_IMM;
		operand->imm	= instr->imm_data;
		break;
	case SrcReg:
		operand->type	= OP_REG;
		operand->reg	= instr->reg_opc;
		break;
	}
}

static void
decode_imm16(struct x86_instr *instr, uint8_t imm_lo, uint8_t imm_hi)
{
	instr->imm_data	= (imm_hi << 8) | imm_lo;

	instr->nr_bytes	+= 2;
}

static void
decode_disp16(struct x86_instr *instr, uint8_t disp_lo, uint8_t disp_hi)
{
	instr->disp	= (int16_t)((disp_hi << 8) | disp_lo);

	instr->nr_bytes	+= 2;
}

static void
decode_disp8(struct x86_instr *instr, uint8_t disp)
{
	instr->disp	= (int8_t)disp;

	instr->nr_bytes	+= 1;
}

static void
decode_modrm_byte(struct x86_instr *instr, uint8_t modrm)
{
	instr->mod	= (modrm & 0xc0) >> 6;
	instr->reg_opc	= (modrm & 0x38) >> 3;
	instr->rm	= (modrm & 0x07);

	switch (instr->mod) {
	case 0x00:
		instr->flags	|= DstMem;
		break;
	case 0x01:
		instr->flags	|= DstMemDisp8;
		break;
	case 0x02:
		instr->flags	|= DstMemDisp16;
		break;
	case 0x03:
		instr->flags	|= DstReg;
		break;
	}
	instr->nr_bytes++;
}

int
arch_8086_decode_instr(struct x86_instr *instr, uint8_t* RAM, addr_t pc)
{
	uint8_t opcode;

	instr->nr_bytes = 1;

	/* Prefixes */
	instr->seg_override	= NO_OVERRIDE;
	for (;;) {
		switch (opcode = RAM[pc++]) {
		case 0x26:
			instr->seg_override	= ES_OVERRIDE;
			break;
		case 0x2e:
			instr->seg_override	= CS_OVERRIDE;
			break;
		case 0x36:
			instr->seg_override	= SS_OVERRIDE;
			break;
		case 0x3e:
			instr->seg_override	= DS_OVERRIDE;
			break;
		case 0xf2:	/* REPNE/REPNZ */
		case 0xf3:	/* REP/REPE/REPZ */
			break;
		default:
			goto done_prefixes;
		}
		instr->nr_bytes++;
	}

done_prefixes:

	/* Opcode byte */
	instr->opcode	= opcode;

	instr->flags	= decode_table[opcode];

	if (instr->flags == 0)	/* Unrecognized? */
		return -1;

	if (instr->flags & ModRM)
		decode_modrm_byte(instr, RAM[pc++]);

	if (instr->flags & DstMemDisp8)
		decode_disp8(instr, RAM[pc+0]);

	if (instr->flags & DstMemDisp16)
		decode_disp16(instr, RAM[pc+0], RAM[pc+1]);

	if (instr->flags & SrcImm16)
		decode_imm16(instr, RAM[pc+0], RAM[pc+1]);

	decode_src_operand(instr);

	decode_dst_operand(instr);

	return 0;
}

int
arch_8086_instr_length(struct x86_instr *instr)
{
	return instr->nr_bytes;
}
