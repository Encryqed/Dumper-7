#pragma once
#include "ObjectArray.h"
#include "Nodes.h"


class ReflectionParser
{
private:
	friend class GeneratorBase;

private:
	static MemberNode ParseMemberNode(UEProperty Property);
	static StructNode ParseStructNode(UEStruct Struct);
	static StructNode ParseClassNode(UEClass Class);
	static EnumNode ParseEnumNode(UEEnum Class);
	static FunctionNode ParseFunctionNode(UEFunction Function);
};