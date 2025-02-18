
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

