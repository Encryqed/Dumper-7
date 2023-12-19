#include "MemberManager.h"
#include "MemberWrappers.h"

#include <algorithm>


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
	return GetNumFunctions() > 0x0 || GetNumPredefFunctions() > 0x0;
}

bool MemberManager::HasMembers() const
{
	return GetNumMembers() > 0x0 || GetNumPredefMembers() > 0x0;
}

