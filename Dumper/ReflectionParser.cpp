#include "ReflectionParser.h"

MemberNode ReflectionParser::ParseMemberNode(UEProperty Property)
{
	return MemberNode();
}

StructNode ReflectionParser::ParseStructNode(UEStruct Struct)
{
	StructNode OutNode;

	OutNode.UniqueName = "";
	OutNode.RawName = Struct.GetName();
	OutNode.FullName = Struct.GetFullName();
	OutNode.Size = Struct.GetStructSize();

	auto It = UEStruct::StructSizes.find(Struct.GetIndex());
	if (It != UEStruct::StructSizes.end())
		OutNode.Size = It->second;

	for (UEProperty Property : Struct.GetProperties())
		std::cout << "Property";

	return StructNode();
}

StructNode ReflectionParser::ParseClassNode(UEClass Class)
{
	return StructNode();
}

EnumNode ReflectionParser::ParseEnumNode(UEEnum Class)
{
	return EnumNode();
}

FunctionNode ReflectionParser::ParseFunctionNode(UEFunction Function)
{
	return FunctionNode();
}