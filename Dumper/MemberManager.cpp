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