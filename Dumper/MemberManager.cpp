#include "MemberManager.h"
#include "MemberWrappers.h"

template<>
auto MemberIterator<UEProperty, false>::operator*() const
{
	using DereferenceType = std::conditional_t<bIsProperty, PropertyWrapper, FunctionWrapper>;

	return bIsCurrentlyPredefined ? DereferenceType(Struct, &PredefElements->at(CurrentPredefIdx)) : DereferenceType(Struct, Members.at(CurrentIdx));
}
