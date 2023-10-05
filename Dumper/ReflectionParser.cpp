#include "ReflectionParser.h"

MemberNode ReflectionParser::ParseMemberNode(StringTable& Names, UEProperty Property)
{
	return MemberNode();
}

StructNode ReflectionParser::ParseStructNode(StringTable& Names, UEStruct Struct)
{
	StructNode OutNode;

	//OutNode.FullName = Names.FindOrAdd(Struct.GetFullName());
	OutNode.Size = Struct.GetStructSize();

	auto It = UEStruct::StructSizes.find(Struct.GetIndex());
	if (It != UEStruct::StructSizes.end())
		OutNode.Size = It->second;

	for (UEProperty Property : Struct.GetProperties())
		std::cout << "Property";

	return StructNode();
}

StructNode ReflectionParser::ParseClassNode(StringTable& Names, UEClass Class)
{
	return StructNode();
}

EnumNode ReflectionParser::ParseEnumNode(StringTable& Names, UEEnum Class)
{
	return EnumNode();
}

FunctionNode ReflectionParser::ParseFunctionNode(StringTable& Names, UEFunction Function)
{
	return FunctionNode();
}