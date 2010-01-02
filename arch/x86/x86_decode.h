#ifndef I8086_DECODE_H
#define I8086_DECODE_H

enum x86_operand_type {
	OP_REG,
	OP_IMM,
	OP_MEM,
	OP_MEM_DISP,
};

enum x86_seg_override {
	NO_OVERRIDE,
	ES_OVERRIDE,
	CS_OVERRIDE,
	SS_OVERRIDE,
	DS_OVERRIDE,
};

struct x86_operand {
	enum x86_operand_type	type;
	uint8_t			reg;
	int			disp;
	uint8_t			imm;
};

enum x86_instr_flags {
	ModRM			= (1U << 8),

	/* Operand sizes */
	WidthByte		= (1U << 9),	/* 8 bits */
	WidthWide		= (1U << 10),	/* 16 bits or 32 bits */
	WidthMask		= WidthByte|WidthWide,

	/* Source operand */
	SrcNone			= (1U << 11),
	SrcImm			= (1U << 12),
	SrcReg			= (1U << 13),
	SrcMask			= SrcNone|SrcImm|SrcReg,

	/* Destination operand */
	DstNone			= (1U << 14),
	DstReg			= (1U << 15),
	DstMem			= (1U << 16),
	DstMemDisp8		= (1U << 17),
	DstMemDisp16		= (1U << 18),
	DstMask			= DstNone|DstReg|DstMem|DstMemDisp8|DstMemDisp16,

	MemDispMask		= DstMemDisp8|DstMemDisp16,
};

struct x86_instr {
	unsigned long		nr_bytes;

	uint8_t			opcode;		/* Opcode byte */
	uint8_t			width;
	uint8_t			mod;		/* Mod */
	uint8_t			rm;		/* R/M */
	uint8_t			reg_opc;	/* Reg/Opcode */
	int			disp;		/* Address displacement */
	unsigned long		imm_data;	/* Immediate data */

	unsigned long		flags;		/* See enum x86_instr_flags */
	enum x86_seg_override	seg_override;
	struct x86_operand	src;
	struct x86_operand	dst;
};

static uint8_t x86_instr_type(struct x86_instr *instr)
{
	return instr->flags & 0xff;
}

int
arch_8086_decode_instr(struct x86_instr *instr, uint8_t* RAM, addr_t pc);

int
arch_8086_instr_length(struct x86_instr *instr);

#endif /* I8086_DECODE_H */
