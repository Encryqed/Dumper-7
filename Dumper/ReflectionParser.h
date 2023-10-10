#pragma once
#include "ObjectArray.h"
#include "Nodes.h"
#include "HashStringTable.h"

class ReflectionParser
{
private:
	friend class GeneratorBase;

private:
	static MemberNode ParseMemberNode(HashStringTable& Names, UEProperty Property);
	static StructNode ParseStructNode(HashStringTable& Names, UEStruct Struct);
	static StructNode ParseClassNode(HashStringTable& Names, UEClass Class);
	static EnumNode ParseEnumNode(HashStringTable& Names, UEEnum Class);
	static FunctionNode ParseFunctionNode(HashStringTable& Names, UEFunction Function);
};