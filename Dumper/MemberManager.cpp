#include "MemberManager.h"
#include "MemberWrappers.h"

#include <algorithm>

template<typename T>
int32 MemberIterator<T>::GetUnrealMemberOffset() const
{
	return IsValidUnrealMemberIndex() ? Members.at(CurrentIdx).GetOffset() : 0xFFFFFFF;
}

template<typename T>
int32 MemberIterator<T>::GetPredefMemberOffset() const
{
	return IsValidPredefMemberIndex() ? PredefElements->at(CurrentPredefIdx).Offset : 0xFFFFFFF;
}

template<typename T>
MemberIterator<T>::DereferenceType MemberIterator<T>::operator*() const
{
	return bIsCurrentlyPredefined ? DereferenceType(Struct, &PredefElements->at(CurrentPredefIdx)) : DereferenceType(Struct, Members.at(CurrentIdx));
}


MemberManager::MemberManager(UEStruct Str)
	: Struct(Str)
	, Functions(Struct.GetFunctions())
	, Members(Struct.GetProperties())
{
	// sorts functions/members in O(n * log(n)), can be sorted via radix, O(n), but the overhead might not be worth it
	std::sort(Functions.begin(), Functions.end(), CompareUnrealFunctions);
	std::sort(Members.begin(), Members.end(), CompareUnrealProperties);

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

	MemberIterator<UEProperty> Prop(EmptyProp);
	Prop.GetUnrealMemberOffset();
	Prop.GetPredefMemberOffset();
	*Prop;

	MemberIterator<UEFunction> Func(EmptyFunc);
	*Func;

	MemberIterator<UEFunction> FuncFalse(EmptyFunc);
	*Func;
}