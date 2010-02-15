#ifndef __upcl_c_sema_analyzer_h
#define __upcl_c_sema_analyzer_h

#include "ast/ast.h"
#include "c/type.h"
#include "c/expression.h"
#include "c/instruction.h"
#include "c/statement.h"
#include "c/decoder_operand_def.h"
#include "c/temp_value_def.h"
#include "sema/register_dep_tracker.h"
#include "sema/register_file_builder.h"
#include "sema/register_info.h"
#include "sema/expr_convert.h"

namespace upcl { namespace c {

typedef std::map<std::string, c::instruction *> named_instruction_map;
typedef std::map<std::string, c::decoder_operand_def *> named_decoder_operand_map;

class sema_analyzer : protected sema::expr_convert_lookup {
private:
	// architecture

	std::string m_arch_name;

	std::string m_arch_full_name;
	uint64_t m_arch_tags[10];

	sema::register_dep_tracker m_dpt;
	sema::register_file_builder m_rfb;
	
	named_instruction_map m_insns;

	named_decoder_operand_map m_operands;

	temp_value_map m_locals; // valid in current instruction

public:
	sema_analyzer();

	bool parse(ast::token_list const *root);

public:
	bool process_architecture(ast::architecture const *arch);

private:
	bool process_register_file(ast::register_file const *reg_file);
	bool process_register_group_dep(ast::register_group const *group);
	bool process_register_group(ast::register_group const *group);
	bool process_register_dep(ast::register_declaration const *rd,
			std::string const &group_name);

private:
	bool process_register_splitter_dep(sema::register_info *ri);
	bool process_bound_value_dep(sema::register_info *ri, size_t &offset,
			ast::type const *type, ast::bound_value const *bv, bool explic);
	bool process_typed_bound_value_dep(sema::register_info *ri, size_t &offset,
			size_t max_size, ast::type const *type,
			ast::typed_bound_value const *bv, bool explic);

private:
	bool process_decoder_operands(ast::decoder_operands const *dec);

private:
	bool process_instructions(ast::token_list const *insns);
	bool process_instruction(ast::instruction const *);
	bool process_jump_instruction(ast::jump_instruction const *);
	bool process_macro(ast::macro const *);

private:
	bool process_instruction_body(c::instruction *insn,
			ast::token_list const *body);
	bool process_assignment(c::instruction *insn,
		ast::assignment_statement const *stmt);

private:
	static inline std::string destringify(std::string const &s)
	{ return s.substr(1, s.length()-2); }

private:
	bool process_single_assignment(ast::assignment_statement::assignment_type const &type,
			c::statement_vector &stmts, ast::identifier const *ident, c::expression *rhs);
	bool process_multiple_assignments(ast::assignment_statement::assignment_type const &type,
			c::statement_vector &stmts, ast::qualified_identifier const *qi, expression *rhs);

private:
	c::statement *create_store(ast::assignment_statement::assignment_type const &type,
		c::expression *target, c::expression *value);

private:
	expression *lookup_target(ast::identifier const *identifier,
			c::type *hint_type = 0);
	expression *lookup_target(ast::identifier const *identifier,
			ast::identifier const *sub_identifier,
			c::type *hint_type = 0);

protected:
	expression *expr_convert_lookup_identifier(std::string const &name) const;
	expression *expr_convert_lookup_identifier(std::string const &base,
			std::string const &name) const;
	type *expr_convert_get_default_word_type() const;
};

} }

#endif  // !__upcl_c_sema_analyzer_h
