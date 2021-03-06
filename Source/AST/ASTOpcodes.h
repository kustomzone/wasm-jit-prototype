#pragma once

#include <cstdint>

#include "ASTTypes.h"

namespace AST
{
	#define ENUM_AST_OPS_Any() \
		AST_OP(error) \
		AST_OP(getLocal) \
		AST_OP(setLocal) \
		AST_OP(load) AST_OP(store) \
		AST_OP(callDirect) AST_OP(callImport) AST_OP(callIndirect) \
		AST_OP(loop) AST_OP(ifElse) AST_OP(select) AST_OP(label) AST_OP(sequence) \
		AST_OP(branch) AST_OP(branchTable) AST_OP(ret) AST_OP(unreachable)

	#define ENUM_AST_UNARY_OPS_Int() \
		AST_OP(neg) \
		AST_OP(abs) \
		AST_OP(bitwiseNot) \
		AST_OP(clz) \
		AST_OP(ctz) \
		AST_OP(popcnt)

	#define ENUM_AST_BINARY_OPS_Int() \
		AST_OP(add) \
		AST_OP(sub) \
		AST_OP(mul) \
		AST_OP(divs) \
		AST_OP(divu) \
		AST_OP(rems) \
		AST_OP(remu) \
		AST_OP(bitwiseAnd) \
		AST_OP(bitwiseOr) \
		AST_OP(bitwiseXor) \
		AST_OP(shl) \
		AST_OP(shrSExt) \
		AST_OP(shrZExt) \
		AST_OP(rotl) \
		AST_OP(rotr)

	#define ENUM_AST_CAST_OPS_Int() \
		AST_OP(wrap) \
		AST_OP(truncSignedFloat) \
		AST_OP(truncUnsignedFloat) \
		AST_OP(sext) \
		AST_OP(zext) \
		AST_OP(reinterpretFloat)
	
	#define ENUM_AST_COMPARISON_OPS() \
		AST_OP(eq) AST_OP(ne) \
		AST_OP(lts) AST_OP(ltu) AST_OP(lt) \
		AST_OP(les) AST_OP(leu) AST_OP(le) \
		AST_OP(gts) AST_OP(gtu) AST_OP(gt) \
		AST_OP(ges) AST_OP(geu) AST_OP(ge)

	#define ENUM_AST_OPS_Int() \
		ENUM_AST_OPS_Any() \
		ENUM_AST_UNARY_OPS_Int() \
		ENUM_AST_BINARY_OPS_Int() \
		ENUM_AST_CAST_OPS_Int() \
		ENUM_AST_COMPARISON_OPS() \
		AST_OP(lit) \
		AST_OP(loadZExt) \
		AST_OP(loadSExt) \
		AST_OP(hasFeature)

	#define ENUM_AST_UNARY_OPS_Float() \
		AST_OP(neg) \
		AST_OP(abs) \
		AST_OP(ceil) \
		AST_OP(floor) \
		AST_OP(trunc) \
		AST_OP(nearestInt) \
		AST_OP(sqrt)

	#define ENUM_AST_BINARY_OPS_Float() \
		AST_OP(add) \
		AST_OP(sub) \
		AST_OP(mul) \
		AST_OP(div) \
		AST_OP(rem) \
		AST_OP(min) \
		AST_OP(max) \
		AST_OP(copySign)

	#define ENUM_AST_CAST_OPS_Float() \
		AST_OP(convertSignedInt) \
		AST_OP(convertUnsignedInt) \
		AST_OP(promote) \
		AST_OP(demote) \
		AST_OP(reinterpretInt)

	#define ENUM_AST_OPS_Float() \
		ENUM_AST_OPS_Any() \
		ENUM_AST_UNARY_OPS_Float() \
		ENUM_AST_BINARY_OPS_Float() \
		ENUM_AST_CAST_OPS_Float() \
		AST_OP(lit)

	#define ENUM_AST_OPS_Void() \
		ENUM_AST_OPS_Any() \
		AST_OP(discardResult) AST_OP(nop) \
		AST_OP(branchIf)

	#define ENUM_AST_OPS_None() ENUM_AST_OPS_Any()

	// Define the ClassOp enums: AnyOp, IntOp, etc.
	#define AST_OP(op) op,
	enum class AnyOp : uint8		{ ENUM_AST_OPS_Any() };
	enum class IntOp : uint8		{ ENUM_AST_OPS_Int() };
	enum class FloatOp : uint8	{ ENUM_AST_OPS_Float() };
	enum class VoidOp : uint8	{ ENUM_AST_OPS_Void() };
	enum class NoneOp : uint8 { ENUM_AST_OPS_None() };
	#undef AST_OP
	
	// Used to represent an opcode as a type for compile-time specialization.
	template<typename Class> struct OpTypes;
	#define AST_OP(op) struct op {};
	#define AST_TYPECLASS(className) template<> struct OpTypes<className##Class> { ENUM_AST_OPS_##className() };
	ENUM_AST_TYPECLASSES()
	#undef AST_OP
	#undef AST_TYPECLASS

	// Define the getOpName function for each ClassOp enum.
	#define AST_TYPECLASS(className) AST_API const char* getOpName(className##Op op);
	ENUM_AST_TYPECLASSES()
	#undef AST_TYPECLASS
}
