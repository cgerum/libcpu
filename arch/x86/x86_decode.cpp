#include "libcpu.h"
#include "x86_isa.h"
#include "x86_decode.h"

static unsigned long decode_table[256] = {
	/*[0x0]*/	0,
	/*[0x1]*/	0,
	/*[0x2]*/	0,
	/*[0x3]*/	0,
	/*[0x4]*/	0,
	/*[0x5]*/	0,
	/*[0x6]*/	0,
	/*[0x7]*/	0,
	/*[0x8]*/	0,
	/*[0x9]*/	0,
	/*[0xA]*/	0,
	/*[0xB]*/	0,
	/*[0xC]*/	0,
	/*[0xD]*/	0,
	/*[0xE]*/	0,
	/*[0xF]*/	0,
	/*[0x10]*/	0,
	/*[0x11]*/	0,
	/*[0x12]*/	0,
	/*[0x13]*/	0,
	/*[0x14]*/	0,
	/*[0x15]*/	0,
	/*[0x16]*/	0,
	/*[0x17]*/	0,
	/*[0x18]*/	0,
	/*[0x19]*/	0,
	/*[0x1A]*/	0,
	/*[0x1B]*/	0,
	/*[0x1C]*/	0,
	/*[0x1D]*/	0,
	/*[0x1E]*/	0,
	/*[0x1F]*/	0,
	/*[0x20]*/	0,
	/*[0x21]*/	0,
	/*[0x22]*/	0,
	/*[0x23]*/	0,
	/*[0x24]*/	0,
	/*[0x25]*/	0,
	/*[0x26]*/	0,
	/*[0x27]*/	0,
	/*[0x28]*/	0,
	/*[0x29]*/	0,
	/*[0x2A]*/	0,
	/*[0x2B]*/	0,
	/*[0x2C]*/	0,
	/*[0x2D]*/	0,
	/*[0x2E]*/	0,
	/*[0x2F]*/	0,
	/*[0x30]*/	0,
	/*[0x31]*/	0,
	/*[0x32]*/	0,
	/*[0x33]*/	0,
	/*[0x34]*/	0,
	/*[0x35]*/	0,
	/*[0x36]*/	0,
	/*[0x37]*/	0,
	/*[0x38]*/	0,
	/*[0x39]*/	0,
	/*[0x3A]*/	0,
	/*[0x3B]*/	0,
	/*[0x3C]*/	0,
	/*[0x3D]*/	0,
	/*[0x3E]*/	0,
	/*[0x3F]*/	0,
	/*[0x40]*/	0,
	/*[0x41]*/	0,
	/*[0x42]*/	0,
	/*[0x43]*/	0,
	/*[0x44]*/	0,
	/*[0x45]*/	0,
	/*[0x46]*/	0,
	/*[0x47]*/	0,
	/*[0x48]*/	0,
	/*[0x49]*/	0,
	/*[0x4A]*/	0,
	/*[0x4B]*/	0,
	/*[0x4C]*/	0,
	/*[0x4D]*/	0,
	/*[0x4E]*/	0,
	/*[0x4F]*/	0,
	/*[0x50]*/	0,
	/*[0x51]*/	0,
	/*[0x52]*/	0,
	/*[0x53]*/	0,
	/*[0x54]*/	0,
	/*[0x55]*/	0,
	/*[0x56]*/	0,
	/*[0x57]*/	0,
	/*[0x58]*/	0,
	/*[0x59]*/	0,
	/*[0x5A]*/	0,
	/*[0x5B]*/	0,
	/*[0x5C]*/	0,
	/*[0x5D]*/	0,
	/*[0x5E]*/	0,
	/*[0x5F]*/	0,
	/*[0x60]*/	0,
	/*[0x61]*/	0,
	/*[0x62]*/	0,
	/*[0x63]*/	0,
	/*[0x64]*/	0,
	/*[0x65]*/	0,
	/*[0x66]*/	0,
	/*[0x67]*/	0,
	/*[0x68]*/	0,
	/*[0x69]*/	0,
	/*[0x6A]*/	0,
	/*[0x6B]*/	0,
	/*[0x6C]*/	0,
	/*[0x6D]*/	0,
	/*[0x6E]*/	0,
	/*[0x6F]*/	0,
	/*[0x70]*/	0,
	/*[0x71]*/	0,
	/*[0x72]*/	0,
	/*[0x73]*/	0,
	/*[0x74]*/	0,
	/*[0x75]*/	0,
	/*[0x76]*/	0,
	/*[0x77]*/	0,
	/*[0x78]*/	0,
	/*[0x79]*/	0,
	/*[0x7A]*/	0,
	/*[0x7B]*/	0,
	/*[0x7C]*/	0,
	/*[0x7D]*/	0,
	/*[0x7E]*/	0,
	/*[0x7F]*/	0,
	/*[0x80]*/	0,
	/*[0x81]*/	0,
	/*[0x82]*/	0,
	/*[0x83]*/	0,
	/*[0x84]*/	0,
	/*[0x85]*/	0,
	/*[0x86]*/	0,
	/*[0x87]*/	0,
	/*[0x88]*/	0,
	/*[0x89]*/	INSTR_MOV|ModRM|SrcReg,
	/*[0x8A]*/	0,
	/*[0x8B]*/	0,
	/*[0x8C]*/	0,
	/*[0x8D]*/	0,
	/*[0x8E]*/	0,
	/*[0x8F]*/	0,
	/*[0x90]*/	0,
	/*[0x91]*/	0,
	/*[0x92]*/	0,
	/*[0x93]*/	0,
	/*[0x94]*/	0,
	/*[0x95]*/	0,
	/*[0x96]*/	0,
	/*[0x97]*/	0,
	/*[0x98]*/	0,
	/*[0x99]*/	0,
	/*[0x9A]*/	0,
	/*[0x9B]*/	0,
	/*[0x9C]*/	0,
	/*[0x9D]*/	0,
	/*[0x9E]*/	0,
	/*[0x9F]*/	0,
	/*[0xA0]*/	0,
	/*[0xA1]*/	0,
	/*[0xA2]*/	0,
	/*[0xA3]*/	0,
	/*[0xA4]*/	0,
	/*[0xA5]*/	0,
	/*[0xA6]*/	0,
	/*[0xA7]*/	0,
	/*[0xA8]*/	0,
	/*[0xA9]*/	0,
	/*[0xAA]*/	0,
	/*[0xAB]*/	0,
	/*[0xAC]*/	0,
	/*[0xAD]*/	0,
	/*[0xAE]*/	0,
	/*[0xAF]*/	0,
	/*[0xB0]*/	0,
	/*[0xB1]*/	0,
	/*[0xB2]*/	0,
	/*[0xB3]*/	0,
	/*[0xB4]*/	0,
	/*[0xB5]*/	0,
	/*[0xB6]*/	0,
	/*[0xB7]*/	0,
	/*[0xB8]*/	INSTR_MOV|SrcImm16|DstReg,
	/*[0xB9]*/	INSTR_MOV|SrcImm16|DstReg,
	/*[0xBA]*/	INSTR_MOV|SrcImm16|DstReg,
	/*[0xBB]*/	INSTR_MOV|SrcImm16|DstReg,
	/*[0xBC]*/	0,
	/*[0xBD]*/	0,
	/*[0xBE]*/	0,
	/*[0xBF]*/	0,
	/*[0xC0]*/	0,
	/*[0xC1]*/	0,
	/*[0xC2]*/	0,
	/*[0xC3]*/	INSTR_RET|SrcNone|DstNone,
	/*[0xC4]*/	0,
	/*[0xC5]*/	0,
	/*[0xC6]*/	0,
	/*[0xC7]*/	0,
	/*[0xC8]*/	0,
	/*[0xC9]*/	0,
	/*[0xCA]*/	0,
	/*[0xCB]*/	0,
	/*[0xCC]*/	0,
	/*[0xCD]*/	0,
	/*[0xCE]*/	0,
	/*[0xCF]*/	0,
	/*[0xD0]*/	0,
	/*[0xD1]*/	0,
	/*[0xD2]*/	0,
	/*[0xD3]*/	0,
	/*[0xD4]*/	0,
	/*[0xD5]*/	0,
	/*[0xD6]*/	0,
	/*[0xD7]*/	0,
	/*[0xD8]*/	0,
	/*[0xD9]*/	0,
	/*[0xDA]*/	0,
	/*[0xDB]*/	0,
	/*[0xDC]*/	0,
	/*[0xDD]*/	0,
	/*[0xDE]*/	0,
	/*[0xDF]*/	0,
	/*[0xE0]*/	0,
	/*[0xE1]*/	0,
	/*[0xE2]*/	0,
	/*[0xE3]*/	0,
	/*[0xE4]*/	0,
	/*[0xE5]*/	0,
	/*[0xE6]*/	0,
	/*[0xE7]*/	0,
	/*[0xE8]*/	0,
	/*[0xE9]*/	0,
	/*[0xEA]*/	0,
	/*[0xEB]*/	0,
	/*[0xEC]*/	0,
	/*[0xED]*/	0,
	/*[0xEE]*/	0,
	/*[0xEF]*/	0,
	/*[0xF0]*/	0,
	/*[0xF1]*/	0,
	/*[0xF2]*/	0,
	/*[0xF3]*/	0,
	/*[0xF4]*/	0,
	/*[0xF5]*/	0,
	/*[0xF6]*/	0,
	/*[0xF7]*/	0,
	/*[0xF8]*/	0,
	/*[0xF9]*/	0,
	/*[0xFA]*/	0,
	/*[0xFB]*/	0,
	/*[0xFC]*/	0,
	/*[0xFD]*/	0,
	/*[0xFE]*/	0,
	/*[0xFF]*/	0,
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
