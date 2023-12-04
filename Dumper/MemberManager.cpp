#include "MemberManager.h"
#include "MemberWrappers.h"

template<typename T, bool bSkip>
int32 MemberIterator<T, bSkip>::GetNextUnrealMemberOffset() const
{
	return HasMoreUnrealMembers() ? Members[CurrentIdx + 1].GetOffset() : 0xFFFFFFFF;
}

template<typename T, bool bSkip>
int32 MemberIterator<T, bSkip>::GetNextPredefMemberOffset() const
{
	return HasMorePredefMembers() ? PredefElements->at(CurrentIdx + 1).Offset : 0xFFFFFFFF;
}

template<typename T, bool bSkip>
MemberIterator<T, bSkip>::DereferenceType MemberIterator<T, bSkip>::operator*() const
{

	return bIsCurrentlyPredefined ? DereferenceType(Struct, &PredefElements->at(CurrentPredefIdx)) : DereferenceType(Struct, Members.at(CurrentIdx));
}


MemberManager::MemberManager(UEStruct Str)
	: Struct(Str)
	, Functions(Struct.GetFunctions())
	, Members(Struct.GetProperties())
{
	if (!PredefinedMemberLookup)
		return;

	auto It = PredefinedMemberLookup->find(Struct.GetIndex());
	if (It != PredefinedMemberLookup->end())
	{
		PredefMembers = &It->second.Members;
		PredefFunctions = &It->second.Functions;

		auto FindFirstStaticMember = [](auto& ElementVector) -> int32
		{
			for (int i = 0; i < ElementVector.size(); i++)
			{
				if (ElementVector[i].bIsStatic)
					return i + 1;
			}

			return 0x0;
		};

		NumStaticPredefMembers = FindFirstStaticMember(*PredefMembers);
		NumStaticPredefFunctions = FindFirstStaticMember(*PredefFunctions);
	}
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
	return GetNumFunctions() > 0x0 && GetNumPredefMembers() > 0x0;
}

bool MemberManager::HasMembers() const
{
	return GetNumMembers() > 0x0 && GetNumPredefMembers() > 0x0;
}

/*
* The compiler won't generate functions for a specific template type unless it's used in the .cpp file corresponding to the
* header it was declatred in.
*
* See https://stackoverflow.com/questions/456713/why-do-i-get-unresolved-external-symbol-errors-when-using-templates
*/
[[maybe_unused]] void TemplateTypeCreationForMemberIterator(void)
{
	std::vector<UEProperty> EmptyProp;
	std::vector<UEFunction> EmptyFunc;

	MemberIterator<UEProperty, true> PropTrue(EmptyProp);
	PropTrue.GetNextUnrealMemberOffset();
	PropTrue.GetNextPredefMemberOffset();
	*PropTrue;


	MemberIterator<UEProperty, false> PropFalse(EmptyProp);
	PropFalse.GetNextUnrealMemberOffset();
	PropFalse.GetNextPredefMemberOffset();
	*PropFalse;

	MemberIterator<UEFunction, true> FuncTrue(EmptyFunc);
	*FuncTrue;

	MemberIterator<UEFunction, false> FuncFalse(EmptyFunc);
	*FuncFalse;
}