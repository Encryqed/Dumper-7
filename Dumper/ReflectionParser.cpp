#include "ReflectionParser.h"

MemberNode ReflectionParser::ParseMemberNode(HashStringTable& Names, UEProperty Property)
{
	return MemberNode();
}

StructNode ReflectionParser::ParseStructNode(HashStringTable& Names, UEStruct Struct)
{
	StructNode OutNode;

	OutNode.FullName = Names.FindOrAdd(Struct.GetFullName()).first;
	OutNode.Size = Struct.GetStructSize();

	auto It = UEStruct::StructSizes.find(Struct.GetIndex());
	if (It != UEStruct::StructSizes.end())
		OutNode.Size = It->second;

	struct MemberLookupInfo
	{
		uint16 Length;
		uint16 Hash;
	};

	std::vector<UEProperty> Properties = Struct.GetProperties();

	//MemberLookupInfo Lookups[Properties.size()];

	std::vector<UEProperty> Properties = Struct.GetProperties();
	OutNode.Members.reserve(Properties.size());

	for (UEProperty Property : Properties)
		OutNode.Members.push_back(ParseMemberNode(Names, Property));

	return StructNode();
}

StructNode ReflectionParser::ParseClassNode(HashStringTable& Names, UEClass Class)
{
	return StructNode();
}

EnumNode ReflectionParser::ParseEnumNode(HashStringTable& Names, UEEnum Class)
{
	return EnumNode();
}

FunctionNode ReflectionParser::ParseFunctionNode(HashStringTable& Names, UEFunction Function)
{
	return FunctionNode();
}