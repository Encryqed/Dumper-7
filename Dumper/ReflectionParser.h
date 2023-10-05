#pragma once
#include "ObjectArray.h"
#include "Nodes.h"

class StringTable
{
};

class ReflectionParser
{
private:
	friend class GeneratorBase;

private:
	static MemberNode ParseMemberNode(StringTable& Names, UEProperty Property);
	static StructNode ParseStructNode(StringTable& Names, UEStruct Struct);
	static StructNode ParseClassNode(StringTable& Names, UEClass Class);
	static EnumNode ParseEnumNode(StringTable& Names, UEEnum Class);
	static FunctionNode ParseFunctionNode(StringTable& Names, UEFunction Function);
};