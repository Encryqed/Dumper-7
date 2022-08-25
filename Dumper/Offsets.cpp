#include "Offsets.h"
#include "ObjectArray.h"
#include "OffsetFinder.h"

void Off::Init()
{
	Off::UClass::ClassFlags = OffsetFinder::FindCastFlagsOffset();	
	Off::UClass::ClassDefaultObject = OffsetFinder::FindDefaultObjectOffset();	

	Off::UEnum::Names = OffsetFinder::FindEnumNamesOffset();	

	Off::UStruct::SuperStruct = OffsetFinder::FindSuperOffset();
	Off::UStruct::Children = OffsetFinder::FindChildOffset();
	Off::UStruct::Size = OffsetFinder::FindStructSizeOffset();

	Off::UFunction::FunctionFlags = OffsetFinder::FindFunctionFlagsOffset();
	
	Off::UProperty::ElementSize = OffsetFinder::FindElementSizeOffset();	
	Off::UProperty::PropertyFlags = OffsetFinder::FindPropertyFlagsOffset();	
	Off::UProperty::Offset_Internal = OffsetFinder::FindOffsetInternalOffset();


	const int32 UPropertySize = OffsetFinder::FindInnerTypeOffset();

	Off::UByteProperty::Enum = UPropertySize;
	Off::UBoolProperty::Base = UPropertySize;
	Off::UObjectProperty::PropertyClass = UPropertySize;
	Off::UStructProperty::Struct = UPropertySize;
	Off::UArrayProperty::Inner = UPropertySize;
	Off::UMapProperty::Base = UPropertySize;
	Off::USetProperty::ElementProp = UPropertySize;
	Off::UEnumProperty::Base = UPropertySize;

	Off::UClassProperty::MetaClass = UPropertySize + 0x8; //0x8 inheritance from UObjectProperty
}