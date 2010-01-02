#ifndef __upcl_c_expression_h
#define __upcl_c_expression_h

#include <sys/types.h>
#include <stdint.h>
#include <vector>

namespace upcl { namespace c {

class expression;

typedef std::vector<expression *> expression_vector;

class register_def;
class type;

class expression {
public:
	enum operation {
		INTEGER,		// Integer Data Type
		FLOAT,			// Float Data Type
		STRING,			// String Data Type (used in assertions)

		REGISTER,		// Register
		LOCAL,			// Local Variable
		DECARG,			// Decoder Argument

		UNARY,			// Unary Expression
		BINARY,			// Binary Expression
		ASSIGN,			// Assignment Expression

		CAST,			// Cast Expression
		BITCAST,		// BitCast Expression
		BIT_COMBINE,	// Bit Combine Expression
		BIT_SLICE,		// Bit Slice Expression

		SIGNED			// Signed Expression
	};

	operation m_expr_op;

protected:
	expression(operation const &op);

public:
	virtual ~expression();

public:
	static expression *fromFloat(double d, unsigned bits);
	static expression *fromFloat(char const *string, unsigned bits);
	static expression *fromString(char const *string);
	static expression *fromInteger(uint64_t x, unsigned bits);
	static expression *fromInteger(int64_t x, unsigned bits);
	static expression *fromInteger(char const *string, unsigned bits);
	static expression *fromRegister(register_def *reg);

public:
	static expression *fromInteger(uint32_t x, unsigned bits)
	{ return fromInteger(static_cast<uint64_t>(x), bits); }
	static expression *fromInteger(int32_t x, unsigned bits)
	{ return fromInteger(static_cast<int64_t>(x), bits); }
	// static expression *fromInteger(size_t x, unsigned bits)
	// { return fromInteger(static_cast<uint64_t>(x), bits); }

public:
	static expression *Neg(expression *a);
	static expression *Com(expression *a);
	static expression *Not(expression *a);

public:
	static expression *Add(expression *a, expression *b);
	static expression *Sub(expression *a, expression *b);
	static expression *Mul(expression *a, expression *b);
	static expression *Div(expression *a, expression *b);
	static expression *Rem(expression *a, expression *b);
	static expression *Shl(expression *a, expression *b);
	static expression *ShlC(expression *a, expression *b);
	static expression *Shr(expression *a, expression *b);
	static expression *ShrC(expression *a, expression *b);
	static expression *Rol(expression *a, expression *b);
	static expression *RolC(expression *a, expression *b);
	static expression *Ror(expression *a, expression *b);
	static expression *RorC(expression *a, expression *b);
	static expression *And(expression *a, expression *b);
	static expression *Or(expression *a, expression *b);
	static expression *Xor(expression *a, expression *b);

public:
	static expression *Eq(expression *a, expression *b);
	static expression *Ne(expression *a, expression *b);
	static expression *Le(expression *a, expression *b);
	static expression *Lt(expression *a, expression *b);
	static expression *Ge(expression *a, expression *b);
	static expression *Gt(expression *a, expression *b);

public:
	static expression *Assign(expression *lhs, expression *rhs, type *ty = 0);
	static expression *Cast(type *ty, expression *expr);
	static expression *Signed(expression *expr);
	static expression *BitSlice(expression *expr, expression *first_bit,
		expression *bit_count);
	static expression *BitCombine(expression *expr, ...);
	static expression *BitCombine(expression_vector const &exprs);

public:
	virtual bool is_constant() const = 0;
	virtual expression *sub_expr(size_t index) const = 0;
	virtual type *get_type() const = 0;

public:
	virtual bool evaluate_as_integer(uint64_t &result, bool sign = false) const = 0;
	virtual bool evaluate_as_float(double &result) const = 0;

	virtual bool is_equal(expression const *expr) const;

public:
	inline operation get_expression_operation() const
	{ return m_expr_op; }

public:
	inline bool evaluate(uint64_t &result, bool sign = false) const
	{ return evaluate_as_integer(result, sign); }
	inline bool evaluate(double &result, bool = false) const
	{ return evaluate_as_float(result); }

public:
	inline bool is_constant_value(bool accept_string = false) const {
		switch (get_expression_operation()) {
			case STRING: // XXX special case!
				return accept_string;
			case INTEGER:
			case FLOAT:
				return true;
			default:
				return false;
		}
	}

public:
	virtual expression *simplify(bool sign = false) const;
	virtual void replace_sub_expr(size_t index, expression *expr);
	virtual bool is_zero() const;

protected:
	bool is_compatible(expression const *expr) const;

protected:
	friend class cast_expression;
	virtual void replace_type(type *ty);
};

} }

#endif  // !__upcl_c_expression_h
