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
	return HasMorePredefMembers() ? PredefElements->at(CurrentIdx + 1).GetOffset() : 0xFFFFFFFF;
}

template<typename T, bool bSkip>
MemberIterator<T, bSkip>::DereferenceType MemberIterator<T, bSkip>::operator*() const
{

	return bIsCurrentlyPredefined ? DereferenceType(Struct, &PredefElements->at(CurrentPredefIdx)) : DereferenceType(Struct, Members.at(CurrentIdx));
}
