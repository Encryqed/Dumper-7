#include <algorithm>

#include "Managers/MemberManager.h"
#include "Wrappers/MemberWrappers.h"

MemberManager::MemberManager(UEStruct Str)
	: Struct(std::make_shared<StructWrapper>(Str))
	, Functions(Str.GetFunctions())
	, Members(Str.GetProperties())
{
	// sorts functions/members in O(n * log(n)), can be sorted via radix, O(n), but the overhead might not be worth it
	std::sort(Functions.begin(), Functions.end(), CompareUnrealFunctions);
	std::sort(Members.begin(), Members.end(), CompareUnrealProperties);

	if (!PredefinedMemberLookup)
		return;

	auto It = PredefinedMemberLookup->find(Struct->GetUnrealStruct().GetIndex());
	if (It != PredefinedMemberLookup->end())
	{
		PredefMembers = &It->second.Members;
		PredefFunctions = &It->second.Functions;
	}
}

MemberManager::MemberManager(const PredefinedStruct* Str)
	: Struct(std::make_shared<StructWrapper>(Str))
	, Functions()
	, Members()
	, PredefMembers(&Str->Properties)
	, PredefFunctions(&Str->Functions)
{
}

int32 MemberManager::GetNumFunctions() const
{
	return Functions.size();
}

int32 MemberManager::GetNumPredefFunctions() const
{
	return PredefFunctions ? PredefFunctions->size() : 0x0;
}

int32 MemberManager::GetNumMembers() const
{
	return Members.size();
}

int32 MemberManager::GetNumPredefMembers() const
{
	return PredefMembers ? PredefMembers->size() : 0x0;
}

bool MemberManager::HasFunctions() const
{
	return GetNumFunctions() > 0x0 || GetNumPredefFunctions() > 0x0;
}

bool MemberManager::HasMembers() const
{
	return GetNumMembers() > 0x0 || GetNumPredefMembers() > 0x0;
}

MemberIterator<true> MemberManager::IterateMembers() const
{
	return MemberIterator<true>(Struct, Members, PredefMembers);
}

FunctionIterator<true> MemberManager::IterateFunctions() const
{
	return FunctionIterator<true>(Struct, Functions, PredefFunctions);
}

void MemberManager::InitReservedNames()
{
	/* UObject reserved names */
	MemberNames.AddReservedClassName("Flags", false);
	MemberNames.AddReservedClassName("Index", false);
	MemberNames.AddReservedClassName("Class", false);
	MemberNames.AddReservedClassName("Name", false);
	MemberNames.AddReservedClassName("Outer", false);

	/* UFunction reserved names */
	MemberNames.AddReservedClassName("FunctionFlags", false);

	/* Function-body reserved names */
	MemberNames.AddReservedClassName("Func", true);
	MemberNames.AddReservedClassName("Parms", true);
	MemberNames.AddReservedClassName("Params", true);
	MemberNames.AddReservedClassName("Flgs", true);


	/* Reserved C++ keywords, typedefs and macros */
	MemberNames.AddReservedName("byte");
	MemberNames.AddReservedName("short");
	MemberNames.AddReservedName("int");
	MemberNames.AddReservedName("float");
	MemberNames.AddReservedName("double");
	MemberNames.AddReservedName("long");
	MemberNames.AddReservedName("signed");
	MemberNames.AddReservedName("unsigned");
	MemberNames.AddReservedName("operator");
	MemberNames.AddReservedName("char");


	/* Control flow keywords */
	MemberNames.AddReservedName("if");
	MemberNames.AddReservedName("else");
	MemberNames.AddReservedName("switch");
	MemberNames.AddReservedName("return");
	MemberNames.AddReservedName("case");
	MemberNames.AddReservedName("default");
	MemberNames.AddReservedName("break");
	MemberNames.AddReservedName("continue");
	MemberNames.AddReservedName("goto");
	MemberNames.AddReservedName("do");

	/* Exception handling */
	MemberNames.AddReservedName("try");
	MemberNames.AddReservedName("catch");
	MemberNames.AddReservedName("throw");

	/* Type related */
	MemberNames.AddReservedName("void");
	MemberNames.AddReservedName("auto");
	MemberNames.AddReservedName("static");
	MemberNames.AddReservedName("extern");
	MemberNames.AddReservedName("register");
	MemberNames.AddReservedName("volatile");
	MemberNames.AddReservedName("typedef");
	MemberNames.AddReservedName("typename");
	MemberNames.AddReservedName("template");
	MemberNames.AddReservedName("namespace");
	MemberNames.AddReservedName("using");
	MemberNames.AddReservedName("friend");
	MemberNames.AddReservedName("virtual");
	MemberNames.AddReservedName("explicit");
	MemberNames.AddReservedName("inline");
	MemberNames.AddReservedName("mutable");

	/* Size specifiers */
	MemberNames.AddReservedName("sizeof");
	MemberNames.AddReservedName("alignof");
	MemberNames.AddReservedName("alignas");

	/* Logical operators */
	MemberNames.AddReservedName("or");
	MemberNames.AddReservedName("and");
	MemberNames.AddReservedName("xor");
	MemberNames.AddReservedName("not");

	/* Additional reserved names */
	MemberNames.AddReservedName("struct");
	MemberNames.AddReservedName("class");
	MemberNames.AddReservedName("for");
	MemberNames.AddReservedName("while");
	MemberNames.AddReservedName("this");
	MemberNames.AddReservedName("override");
	MemberNames.AddReservedName("private");
	MemberNames.AddReservedName("public");
	MemberNames.AddReservedName("const");

	/* Unreal Engine typedefs */
	MemberNames.AddReservedName("int8");
	MemberNames.AddReservedName("int16");
	MemberNames.AddReservedName("int32");
	MemberNames.AddReservedName("int64");
	MemberNames.AddReservedName("uint8");
	MemberNames.AddReservedName("uint16");
	MemberNames.AddReservedName("uint32");
	MemberNames.AddReservedName("uint64");

	MemberNames.AddReservedName("true");
	MemberNames.AddReservedName("false");
	MemberNames.AddReservedName("TRUE");
	MemberNames.AddReservedName("FALSE");

	MemberNames.AddReservedName("min");
	MemberNames.AddReservedName("max");

	MemberNames.AddReservedName("new");
	MemberNames.AddReservedName("delete");

	/* Windows.h macros*/
	MemberNames.AddReservedName("IN");
	MemberNames.AddReservedName("OUT");
	MemberNames.AddReservedName("CreateObject");
	MemberNames.AddReservedName("CreateWindow");

	MemberNames.AddReservedName("NO_ERROR");
	MemberNames.AddReservedName("EVENT_MAX");
	MemberNames.AddReservedName("IGNORE");
}

void MemberManager::FixIncorrectNames()
{
	const UEStruct RotatorStruct = ObjectArray::FindStructFast("Rotator");

	// Search for properties with incorrect casing, if "pitch" is found correct it to "Pitch"
	if (const UEProperty PitchProperty = RotatorStruct.FindMember("pitch"))
		StructManager_NameAccessHelper::ReplaceName(MemberNames, RotatorStruct, PitchProperty, "Pitch");

	if (const UEProperty PitchProperty = RotatorStruct.FindMember("yaw"))
		StructManager_NameAccessHelper::ReplaceName(MemberNames, RotatorStruct, PitchProperty, "Yaw");

	if (const UEProperty PitchProperty = RotatorStruct.FindMember("roll"))
		StructManager_NameAccessHelper::ReplaceName(MemberNames, RotatorStruct, PitchProperty, "Roll");
}
