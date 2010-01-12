//
// Code generator
//
#include "cg/generate.h"
#include "ast/ast.h"

#include <cctype>
#include <sstream>

using namespace upcl;

// Forward declarations
static void
cg_write_reg_file(std::ostream &o, std::string const &arch_name,
		c::register_def_vector const &regs);

static void
cg_write_psr_bitfield(std::ostream &o, c::sub_register_def const *bf);

typedef std::string::value_type charT;

static inline charT upper(charT arg)
{ return std::use_facet<std::ctype<charT> >(std::locale()).toupper(arg); }

static void
make_upper_string(std::string &x)
{ std::transform(x.begin(), x.end(), x.begin(), upper); }

static std::string
upper_string(std::string const &x)
{
	std::string r(x);
	make_upper_string(r);
	return r;
}

// transform a name suitable for C
static std::string
make_def_name(std::string const &name)
{
	std::string result(name);
	if (!result.empty()) {
		for (size_t n = 0; n < result.length(); n++) {
			if (!std::isalnum(result[n]) && result[n] != '_')
				result[n] = '_';
		}
		if (std::isdigit(result[0]))
			result = "_" + result;
	}
		
	return result;
}

// transform a register name suitable for C
static std::string
make_register_c_name(std::string const &name)
{
	size_t pos;
	pos = name.find('$');
	if (pos != std::string::npos) {
		std::string unnamed("__unnamed" + name);

		while ((pos = unnamed.find('$')) != std::string::npos)
			unnamed[pos] = '_';

		return make_def_name(unnamed);
	}
	return make_def_name(name);
}

static inline size_t
get_aligned_size_for_type(c::type const *type, bool max64 = true)
{
	size_t nbits = type->get_bits();

	if (nbits <= 8)
		nbits = 8;
	else if (nbits <= 16)
		nbits = 16;
	else if (nbits <= 32)
		nbits = 32;
	else if (nbits <= 64)
		nbits = 64;
	else if (max64)
		nbits = 64;
	else
		nbits = ((nbits + 63) / 64) * 64;

	return nbits;
}

// emit file banners
static void
cg_write_banner(std::ostream &o, std::string const &fname,
		bool editable = false, std::string const &description = std::string())
{
	o << "/*" << std::endl
	 << " * " << "libcpu: " << fname << std::endl
	 << " * " << std::endl;
	if (!description.empty()) {
		o << " * " << description << std::endl
		 << " * " << std::endl;
	}
	if (!editable) {
		o << " * !!! DO NOT EDIT THIS FILE !!!" << std::endl
		 << " * " << std::endl;
	}
	o << " * THIS FILE HAS BEEN AUTOMATICALLY GENERATED BY UPCC" << std::endl
	 << " */" << std::endl << std::endl;
}

// emit a local include
static inline void
cg_write_include(std::ostream &of, std::string const &fname)
{
	of << "#include \"" << fname << "\"" << std::endl;
}

// emit a common include
static inline void
cg_write_std_include(std::ostream &of, std::string const &fname)
{
	of << "#include <" << fname << ">" << std::endl;
}

// emit begin of #ifdef guard
static inline void
cg_write_guard_begin(std::ostream &of, std::string const &fname)
{
	of << "#ifndef " << make_def_name("__" + fname + "__") << std::endl;
	of << "#define " << make_def_name("__" + fname + "__") << std::endl;
	of << std::endl;
}

// emit end of #ifdef guard
static inline void
cg_write_guard_end(std::ostream &of, std::string const &fname)
{
	of << std::endl;
	of << "#endif  /* " << make_def_name("__" + fname + "__") << " */"
	 << std::endl;
}

// emit typed register honoring the register type.
static void
cg_print_typed_var(std::ostream &o, size_t indent, size_t data_size,
		c::type const *type, std::string const &name,
		std::string const &comment = std::string())
{
	o << std::string(indent, '\t');

	size_t nbits = type->get_bits();
	if (type->get_type_id() == c::type::FLOAT)
		o << "fp_reg" << nbits << "_t " << make_register_c_name(name);
	else {
		o << "uint" << data_size << "_t " << make_register_c_name(name);
		if (nbits > 64)
			o << '[' << ((nbits+data_size-1) / data_size) << ']';
		else if (nbits < data_size)
			o << " : " << nbits;
	}

	o << ';';
	if (!comment.empty())
		o << " // " << comment;

	o << std::endl;
}

// emit typed register or bitfield in integer format.
static void
cg_print_typed_var(std::ostream &o, size_t indent, size_t data_size,
		size_t nbits, std::string const &name,
		std::string const &comment = std::string())
{
	o << std::string(indent, '\t');

	o << "uint" << data_size << "_t " << make_register_c_name(name);
	if (nbits > 64)
		o << '[' << ((nbits+data_size-1) / data_size) << ']';
	else if (nbits < data_size)
		o << " : " << nbits;

	o << ';';
	if (!comment.empty())
		o << " // " << comment;

	o << std::endl;
}

// emit physical register definition.
static void
cg_write_reg_def(std::ostream &o, size_t indent, c::register_def *def,
		bool maybe_unused = true)
{
	size_t nbits;
	if (def->is_sub_register())
		nbits = get_aligned_size_for_type(((c::sub_register_def *)def)->
				get_master_register()->get_type());
	else
		nbits = get_aligned_size_for_type(def->get_type());

	std::string comment;
	if (def->is_sub_register())
		comment = "sub";
	if (def->is_hardwired()) {
		if (!comment.empty()) comment += ", ";
		comment += "hardwired";
	} else if (def->get_bound_special_register() != c::NO_SPECIAL_REGISTER) {
		if (!comment.empty()) comment += ", ";
		comment += "bound to special register";
	} else if (def->get_bound_register() != 0) {
		if (!comment.empty()) comment += ", ";
		comment += "bound to register";
	}
	if (def->is_uow()) {
		if (!comment.empty()) comment += ", ";
		comment += "update-on-write";
	}

	c::sub_register_vector const &sub =
		def->get_sub_register_vector();

	if (!sub.empty()) {
		o << std::string(indent, '\t');
		o << "union {" << std::endl;

		indent++;

		cg_print_typed_var(o, indent, nbits, def->get_type(),
				def->get_name(), comment);

		o << std::string(indent, '\t');
		o << "struct {" << std::endl;

		indent++;
		size_t total = 0;
		for(c::sub_register_vector::const_iterator i = sub.begin();
				i != sub.end(); i++) {
			total += (*i)->get_type()->get_bits();
			cg_write_reg_def(o, indent, *i, false);
		}

		size_t unused_bits =
		 	(((def->get_type()->get_bits()+nbits-1)/nbits)*nbits) - total;

		if (maybe_unused && unused_bits != 0) {
			std::stringstream ss;
			ss << "__unused_" << def->get_name() << '_' << total;
			cg_print_typed_var(o, indent, nbits, unused_bits, ss.str());
		}

		indent--;

		o << std::string(indent, '\t');
		o << "};" << std::endl;

		indent--;

		o << std::string(indent, '\t');
		o << "} " << def->get_name() << ';';

		if (!comment.empty())
			o << " // " << comment;
		o << std::endl;
	} else {
		size_t unused_bits = (maybe_unused && nbits <= 64 &&
				nbits > def->get_type()->get_bits() ?
				(nbits - def->get_type()->get_bits()) : 0);

		if (unused_bits != 0) {
			o << std::string(indent, '\t');
			o << "struct {" << std::endl;
			indent++;
		}

		cg_print_typed_var(o, indent, nbits, def->get_type(),
				def->get_name(), comment);

		if (unused_bits != 0) {
			cg_print_typed_var(o, indent, nbits, unused_bits,
				"__unused_" + def->get_name());
			indent--;
			o << std::string(indent, '\t');
			o << "};" << std::endl;
		}
	}
}

// emit a PSR-bound sub-register flags layout entries
static void
cg_write_psr_bitfield(std::ostream &o, c::sub_register_vector const &bitfields)
{
	for (c::sub_register_vector::const_iterator i = bitfields.begin();
			i != bitfields.end(); i++)
		cg_write_psr_bitfield(o, (*i));
}

// emit a PSR-bound register flags layout entry
static void
cg_write_psr_bitfield(std::ostream &o, c::sub_register_def const *bf)
{
	if (bf->get_name().find('$') != std::string::npos)
		return;

	c::sub_register_vector const &sub_bitfields =
		bf->get_sub_register_vector();
	if (!sub_bitfields.empty()) {
		cg_write_psr_bitfield(o, sub_bitfields);
		return;
	}

	uint64_t shift = 0;
	char special = 0;
	if (bf->get_first_bit()->evaluate(shift)) {
		switch (bf->get_bound_special_register()) {
			case c::SPECIAL_REGISTER_C: special = 'C'; break;
			case c::SPECIAL_REGISTER_N: special = 'N'; break;
			case c::SPECIAL_REGISTER_P: special = 'P'; break;
			case c::SPECIAL_REGISTER_V: special = 'V'; break;
			case c::SPECIAL_REGISTER_Z: special = 'Z'; break;
			default: break;
		}
	}

	o << '\t' << "{ " << shift << ", ";
	if (special != 0)
		o << '\'' << special << '\'';
	else
		o << '0';

	o << ", \"" << bf->get_name() << "\" }" << std::endl;
}

// emit a PSR-bound register flags layout
static void
cg_write_psr_flags_layout(std::ostream &o, std::string const &arch_name, 
		std::string const &reg_name, c::sub_register_vector const &bitfields)
{
	o << "static flags_layout_t const arch_" << arch_name << "_flags_layout[] = {" << std::endl;
	for (c::sub_register_vector::const_iterator i = bitfields.begin();
			i != bitfields.end(); i++)
		cg_write_psr_bitfield(o, *i);
	o << '\t' << "{ -1, 0, NULL }" << std::endl;
	o << "};" << std::endl;
}

// emit the register file.
static void
cg_write_register_file(std::ostream &o, std::string const &arch_name,
		c::register_def_vector const &regs)
{
	size_t offset = 0, padding_count = 0;

	// dump the register file struct
	o << "PACKED(struct reg_" << arch_name << "_t {" << std::endl;
	for (c::register_def_vector::const_iterator i = regs.begin ();
			i != regs.end(); i++) {

		size_t aligned_size = get_aligned_size_for_type((*i)->get_type(), false);
		size_t byte_size = aligned_size >> 3;

		if ((*i)->is_hardwired())
			continue;

		// pad if necessary
		if (offset & (byte_size-1)) {
			o << '\t' << "uint8_t __padding_" << padding_count
				<< '[' << (offset & (byte_size-1)) << ']' << ';' << std::endl;

			offset = (offset + (byte_size-1)) & -byte_size;
			padding_count++;
		}

		cg_write_reg_def(o, 1, *i);

		offset += byte_size;
	}
	o << "});" << std::endl;
}

// emit the register file layout.
void
cg_write_register_file_layout(std::ostream &o, std::string const &arch_name,
		c::register_def_vector const &regs)
{	
	o << "static register_layout_t const arch_" << arch_name << "_register_layout[] = {" << std::endl;
	size_t offset = 0;
	for (c::register_def_vector::const_iterator i = regs.begin ();
			i != regs.end(); i++) {

		if ((*i)->is_hardwired())
			continue;

		size_t size = (*i)->get_type()->get_bits();
		size_t aligned_size = get_aligned_size_for_type((*i)->get_type(), false);
		size_t byte_size = aligned_size >> 3;

		// pad if necessary
		if (offset & (byte_size-1))
			offset = (offset + (byte_size-1)) & -byte_size;

		o << '\t' << "{ ";
		switch ((*i)->get_type()->get_type_id()) {
			case c::type::INTEGER:
				o << "REG_TYPE_INT";
				break;
			case c::type::FLOAT:
				o << "REG_TYPE_FLOAT";
				break;
			case c::type::VECTOR:
			case c::type::VECTOR_INTEGER:
			case c::type::VECTOR_FLOAT:
				o << "REG_TYPE_VECTOR";
				break;
			default:
				o << "REG_TYPE_UNKNOWN";
				break;
		}
		o << ", ";
		o << size << ", ";
		o << aligned_size << ", ";
		o << offset << ", ";
		// flags
		switch ((*i)->get_bound_special_register()) {
			case c::SPECIAL_REGISTER_PC:
				o << "REG_FLAG_PC";
				break;
			case c::SPECIAL_REGISTER_NPC:
				o << "REG_FLAG_NPC";
				break;
			case c::SPECIAL_REGISTER_PSR:
				o << "REG_FLAG_PSR";
				break;
			default:
				o << '0';
				break;
		}
		o << ", ";
		o << '"' << (*i)->get_name() << '"';
		o << " }," << std::endl;

skip:
		offset += byte_size;
	}
	o << '\t' << "{ 0, 0, 0, 0, 0, NULL }" << std::endl;
	o << std::endl << "};" << std::endl;
}

// emit function to initialize architecture.
static void
cg_generate_arch_init(std::ostream &o, std::string const &arch_name,
		std::string const &full_name, uint64_t const *tags)
{
	o << "static void" << std::endl;
	o << "arch_" << arch_name << "_init(arch_info_t *info, arch_regfile_t *rf)" << std::endl;
	o << '{' << std::endl;
	
	o << '\t' << "reg_" << arch_name << "_t *register_file = NULL;" << std::endl;
	o << std::endl;

	o << '\t' << "// Architecture definition" << std::endl;

	// name
	o << '\t' << "info->name = " << '"' << arch_name << '"' << ';' << std::endl;

	// fullname
	if (!full_name.empty())
		o << '\t' << "info->full_name = " << '"' << full_name << '"';
	else
		o << '\t' << "info->full_name = " << '"' << arch_name << '"';
	o << ';' << std::endl;

	// endian
	switch (tags[ast::architecture::ENDIAN]) {
		case ast::architecture::ENDIAN_BIG:
			o << '\t' << "info->common_flags = CPU_FLAG_ENDIAN_BIG";
			break;
		case ast::architecture::ENDIAN_LITTLE:
			o << '\t' << "info->common_flags = CPU_FLAG_ENDIAN_LITTLE";
			break;
		case ast::architecture::ENDIAN_BOTH:
			o << '\t' << "info->common_flags &= CPU_FLAG_ENDIAN_MASK";
			break;
		default:
			o << '\t' << "info->common_flags &= ~CPU_FLAG_ENDIAN_MASK";
			break;
	}
	o << ';' << std::endl;

	// minimum addressable unit
	o << '\t' << "info->byte_size = " << tags[ast::architecture::BYTE_SIZE] << ';' << std::endl;
	// maximum addressable unit
	o << '\t' << "info->word_size = " << tags[ast::architecture::WORD_SIZE] << ';' << std::endl;
	// address size
	o << '\t' << "info->address_size = " << tags[ast::architecture::ADDRESS_SIZE] << ';' << std::endl;
	// largest float size
	if (tags[ast::architecture::FLOAT_SIZE] != 0)
		o << '\t' << "info->float_size = " << tags[ast::architecture::FLOAT_SIZE] << ';' << std::endl;
	// psr size
	if (tags[ast::architecture::PSR_SIZE] != 0) {
		o << '\t' << "info->flags_size = " << tags[ast::architecture::PSR_SIZE] << ';' << std::endl;
	}

	o << std::endl;
	o << '\t' << "// Register file initialization" << std::endl;
	o << '\t' << "register_file = (reg_" << arch_name << "_t *)calloc(1, sizeof(reg_" << arch_name << "_t));"
	 << std::endl;
	o << '\t' << "assert(register_file != NULL);" << std::endl;
	o << '\t' << "rf->storage = register_file;" << std::endl;
	o << '\t' << "rf->layout = arch_" << arch_name << "_register_layout;" << std::endl;
	o << '\t' << "rf->flags_layout = arch_" << arch_name << "_flags_layout;" << std::endl;
	o << '}' << std::endl;
}

// emit function to finalize architecture.
static void
cg_generate_arch_done(std::ostream &o, std::string const &arch_name)
{
	o << "static void" << std::endl;
	o << "arch_" << arch_name << "_done(void *feptr, arch_regfile_t *rf)" << std::endl;
	o << '{' << std::endl;
	o << '\t' << "free(rf->storage);" << std::endl;
	o << '}' << std::endl;
}

// emit architecture callbacks
static void
cg_generate_arch_callbacks(std::ostream &o, std::string const &arch_name)
{
	o << "arch_func_t const arch_func_" << arch_name << " = {" << std::endl;
	o << '\t' << "arch_" << arch_name << "_init," << std::endl;
	o << '\t' << "arch_" << arch_name << "_done," << std::endl;
	o << '\t' << "NULL, /* get_pc */" << std::endl;
	o << '\t' << "NULL, /* emit_decode_reg */" << std::endl;
	o << '\t' << "NULL, /* spill_reg_state */" << std::endl;
	o << '\t' << "arch_" << arch_name << "_tag_instr," << std::endl;
	o << '\t' << "arch_" << arch_name << "_disasm_instr," << std::endl;
	o << '\t' << "arch_" << arch_name << "_translate_cond," << std::endl;
	o << '\t' << "arch_" << arch_name << "_translate_instr," << std::endl;
	o << '\t' << "/* idbg support */" << std::endl;
	o << '\t' << "NULL, /* get_psr */" << std::endl;
	o << '\t' << "NULL, /* get_reg */" << std::endl;
	o << '\t' << "NULL  /* get_fpreg */" << std::endl;
	o << "};" << std::endl;
}

#if 0
//
// hardwired: return expr;
// special evaluation: return expr;
// complex indexing: index = expr; return expr2;
// normal: return reference to register;
//

void
cg_generate_gen_get_reg_set(std::ostream &o, std::string const &arch_name,
		c::register_def_vector const &phys_regs,
		c::register_def_vector const &virt_regs)
{
	size_t offset = 0;

	o << "static bool" << std::endl
	 << "arch_" << arch_name << "_get_reg(void" << arch_name << " *register_file, "
	 "size_t reg_name, void *buffer)" << std::endl;
	o << '{' << std::endl;
	o << '\t' << "switch (reg_name) {" << std::endl;
	for (c::register_def_vector::const_iterator i = phys_regs.begin ();
			i != phys_regs.end(); i++) {
		o << "\t\t" << "case " << "REG_" << arch_name << "_" << (*i)->get_name()
		 << ":" << std::endl;
		
		size_t size = get_aligned_size_for_type((*i)->get_type());
		o << "\t\t\t" << "*(uint" << size << "_t *)buffer = "
		 << "arch->regfile." << (*i)->get_name() << ";" << std::endl;
		o << "\t\t\t" << "break;" << std::endl;
	}
	o << "\t\t" << "default:" << std::endl;
	o << "\t\t\t" << "assert(0 && \"register not defined\");" << std::endl;
	o << "\t\t\t" << "return false;" << std::endl;
	o << '\t' << "}" << std::endl;
	o << '\t' << "return true;" << std::endl;
	o << '}' << std::endl;
}

#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                            P U B L I C  A P I                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void
upcl::cg::generate_arch_h(std::ostream &o, std::string const &fname,
		std::string const &arch_name)
{
	cg_write_banner(o, fname);

	cg_write_guard_begin(o, fname);

	cg_write_std_include(o, "assert.h");

	o << std::endl;

	cg_write_include(o, "libcpu/libcpu.h");
	cg_write_include(o, "libcpu/fp_types.h");
	cg_write_include(o, "libcpu/frontend.h");

	o << std::endl;

	cg_write_include(o, arch_name + "_opc.h");
	cg_write_include(o, arch_name + "_types.h");

	o << std::endl;

	o << "int arch_" << arch_name << "_tag_instr(cpu_t *cpu, addr_t pc, "
	  << "tag_t *tag, addr_t *new_pc, addr_t *next_pc);" << std::endl;
	
	cg_write_guard_end(o, fname);
}

void
upcl::cg::generate_types_h(std::ostream &o, std::string const &fname,
		std::string const &arch_name, c::register_def_vector const &regs)
{
	cg_write_banner(o, fname, false, "the register file");

	cg_write_guard_begin(o, fname);

	o << std::endl;

	cg_write_register_file(o, arch_name, regs);

	cg_write_guard_end(o, fname);
}

void
upcl::cg::generate_arch_cpp(std::ostream &o, std::string const &fname,
		std::string const &arch_name, std::string const &arch_full_name,
		uint64_t const *arch_tags, c::register_def const *pcr,
		c::register_def const *psr, c::register_def_vector const &regs)
{
	// banner
	cg_write_banner(o, fname);

	// includes
	cg_write_include(o, arch_name + "_arch.h");

	o << std::endl;

	// register layout
	o << "// Register File Layout" << std::endl;
	cg_write_register_file_layout(o, arch_name, regs);
	o << std::endl;

	if (psr != 0) {
		o << "// Processor Status Register Flags Layout" << std::endl;
		cg_write_psr_flags_layout(o, arch_name, psr->get_name(),
				psr->get_sub_register_vector());
		o << std::endl;
	}

	o << "// Architecture Initalization" << std::endl;
	cg_generate_arch_init(o, arch_name, arch_full_name, arch_tags);
	o << std::endl;
	o << "// Architecture Finalization" << std::endl;
	cg_generate_arch_done(o, arch_name);
	o << std::endl;
	o << "// Architecture Callbacks" << std::endl;
	cg_generate_arch_callbacks(o, arch_name);
	o << std::endl;
}

void
upcl::cg::generate_opc_h(std::ostream &o, std::string const &fname,
		std::string const &arch_name, c::instruction_vector const &insns)
{
	cg_write_banner(o, fname, false, "the instructions opcode");

	cg_write_guard_begin(o, fname);

	if (!insns.empty()) {
		size_t index = 0;
		o << "enum {" << std::endl;
		for (c::instruction_vector::const_iterator i = insns.begin();
				i != insns.end(); i++) {
			if (i != insns.begin())
				o << ',' << std::endl;
			o << '\t' << "ARCH_" << upper_string(arch_name) << "_OPC_"
			 << upper_string((*i)->get_name());
		}
		o << std::endl << "};" << std::endl;
	}

	cg_write_guard_end(o, fname);
}

void
upcl::cg::generate_tag_cpp(std::ostream &o, std::string const &fname,
		std::string const &arch_name, c::jump_instruction_vector const &jumps)
{
	// banner
	cg_write_banner(o, fname, true);

	// includes
	cg_write_include(o, arch_name + "_arch.h");

	o << std::endl;

	o << "int" << std::endl
	  << "arch_" << arch_name << "_tag_instr(cpu_t *cpu, addr_t pc, "
	  << "tag_t *tag, addr_t *new_pc, addr_t *next_pc)" << std::endl;
	o << '{' << std::endl;
	o << '\t' << "arch_" << arch_name << "_opcode_t opcode;" << std::endl;
	o << '\t' << "size_t length;" << std::endl;
	o << std::endl;
	o << '\t' << "ARCH_" << upper_string(arch_name)
	 	<< "_INSN_GET_OPCODE(cpu, pc, opcode, length);" << std::endl;
	o << '\t' << "switch (opcode) {" << std::endl;
	for(c::jump_instruction_vector::const_iterator i = jumps.begin();
			i != jumps.end(); i++) {
		o << "\t\t" << "case "
		 << "ARCH_" << upper_string(arch_name) << "_OPC_"
		 << upper_string((*i)->get_name()) << ':'
		 << std::endl;
		o << "\t\t\t" << "*tag = TAG_";
		switch ((*i)->get_type()) {
			case c::jump_instruction::BRANCH:
				if ((*i)->get_condition() != 0)
					o << "COND_BRANCH";
				else
					o << "BRANCH";
				break;
			case c::jump_instruction::CALL:
				o << "CALL";
				break;
			case c::jump_instruction::RETURN:
				o << "RET";
				break;
			case c::jump_instruction::TRAP:
				if ((*i)->get_condition() != 0)
					o << "COND_TRAP";
				else
					o << "TRAP";
				break;
			default:
				o << "CONTINUE";
				break;
		}
		o << ';' << std::endl;
		o << "\t\t\t" << "//" << std::endl;
		o << "\t\t\t" << "// Insert here tagging code." << std::endl;
		o << "\t\t\t" << "//" << std::endl;
		o << "\t\t\t" << "*new_pc = NEW_PC_NONE;" << std::endl;
		o << "\t\t\t" << "break;" << std::endl;
		o << std::endl;
	}
	o << "\t\t" << "default:" << std::endl;
	o << "\t\t\t" << "*tag = TAG_CONTINUE;" << std::endl;
	o << "\t\t\t" << "break;" << std::endl;
	o << '\t' << '}' << std::endl;
	o << std::endl;
	o << '\t' << "*next_pc = pc + length;" << std::endl;
	o << '\t' << "return length;" << std::endl;
	o << '}' << std::endl;
}
