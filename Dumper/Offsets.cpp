#include "Offsets.h"
#include "ObjectArray.h"
#include "OffsetFinder.h"

void Off::InSDK::InitPE()
{
	void* PeAddr = (void*)FindByWString(L"Accessed None").FindNextFunctionStart();
	void** Vft = *(void***)ObjectArray::GetByIndex(0).GetAddress();

	Off::InSDK::PEOffset = uintptr_t(PeAddr) - uintptr_t(GetModuleHandle(0));

	for (int i = 0; i < 0x150; i++)
	{
		if (Vft[i] == PeAddr)
		{
			Off::InSDK::PEIndex = i;
			std::cout << "PE-Offset: 0x" << std::hex << Off::InSDK::PEOffset << "\n";
			std::cout << "PE-Index: 0x" << std::hex << i << "\n\n";
			
			return;
		}
	}

	// If PE wasn't found by string ref, use a sig (or two)

	for (int i = 0; i < 0x150; i++)
	{
		if (!Vft[i])
			break;

		if (FindPatternInRange({ 0xF7, -1, Off::UFunction::FunctionFlags, 0x0, 0x0, 0x0, 0x0, 0x04, 0x0, 0x0 }, (uint8*)Vft[i], 0x400)
		&&  FindPatternInRange({ 0xF7, -1, Off::UFunction::FunctionFlags, 0x0, 0x0, 0x0, 0x0, 0x80, 0x0, 0x0 }, (uint8*)Vft[i], 0x400))
		{
			Off::InSDK::PEOffset = uintptr_t(Vft[i]) - uintptr_t(GetModuleHandle(0));
			Off::InSDK::PEIndex = i;

			std::cout << "PE-Offset: 0x" << std::hex << Off::InSDK::PEOffset << "\n";
			std::cout << "PE-Index: 0x" << std::hex << i << "\n\n";
			return;
		}
	}
}

void Off::InSDK::InitPE(int32 Index)
{
	Off::InSDK::PEIndex = Index;

	void** VFT = *reinterpret_cast<void***>(ObjectArray::GetByIndex(0).GetAddress());

	Off::InSDK::PEOffset = uintptr_t(VFT[Off::InSDK::PEIndex]) - uintptr_t(GetModuleHandle(0));

	std::cout << "VFT-Offset: 0x" << std::hex << uintptr_t(VFT) - uintptr_t(GetModuleHandle(0)) << std::endl;
}

void Off::Init()
{
	OffsetFinder::InitUObjectOffsets();

	Off::UField::Next = OffsetFinder::FindFieldNextOffset();
	std::cout << "Off::UField::Next: " << Off::UField::Next << "\n";

	Off::UStruct::Children = OffsetFinder::FindChildOffset();
	std::cout << "Off::UStruct::Children: " << Off::UStruct::Children << "\n";

	Off::UStruct::SuperStruct = OffsetFinder::FindSuperOffset();
	std::cout << "Off::UStruct::SuperStruct: " << Off::UStruct::SuperStruct << "\n";

	Off::UStruct::Size = OffsetFinder::FindStructSizeOffset();
	std::cout << "Off::UStruct::Size: " << Off::UStruct::Size << "\n";

	if (Settings::Internal::bUseFProperty)
	{
		std::cout << "Game uses FProperty system\n\n";

		Off::UStruct::ChildProperties = OffsetFinder::FindChildPropertiesOffset();
		std::cout << "Off::UStruct::ChildProperties: " << Off::UStruct::ChildProperties << "\n";

		OffsetFinder::FixupHardcodedOffsets(); // must be called after FindChildPropertiesOffset 
	}

	Off::UClass::ClassFlags = OffsetFinder::FindCastFlagsOffset();
	std::cout << "Off::UClass::ClassFlags: " << Off::UClass::ClassFlags << "\n";

	Off::UClass::ClassDefaultObject = OffsetFinder::FindDefaultObjectOffset();
	std::cout << "Off::UClass::ClassDefaultObject: " << Off::UClass::ClassDefaultObject << "\n";

	Off::UEnum::Names = OffsetFinder::FindEnumNamesOffset();
	std::cout << "Off::UEnum::Names: " << Off::UEnum::Names << "\n";

	Off::UFunction::FunctionFlags = OffsetFinder::FindFunctionFlagsOffset();
	std::cout << "Off::UFunction::FunctionFlags: " << Off::UFunction::FunctionFlags << "\n\n";

	Off::UProperty::ElementSize = OffsetFinder::FindElementSizeOffset();
	std::cout << "Off::UProperty::ElementSize: " << Off::UProperty::ElementSize << "\n";

	Off::UProperty::ArrayDim = Off::UProperty::ElementSize - 0x4;
	std::cout << "Off::UProperty::ArrayDim: " << Off::UProperty::ArrayDim << "\n";

	Off::UProperty::Offset_Internal = OffsetFinder::FindOffsetInternalOffset();
	std::cout << "Off::UProperty::Offset_Internal: " << Off::UProperty::Offset_Internal << "\n";

	Off::UProperty::PropertyFlags = OffsetFinder::FindPropertyFlagsOffset();
	std::cout << "Off::UProperty::PropertyFlags: " << Off::UProperty::PropertyFlags << "\n";

	const int32 PropertySize = OffsetFinder::FindBoolPropertyBaseOffset();
	std::cout << "UPropertySize: " << PropertySize << "\n\n";

	Off::UArrayProperty::Inner = OffsetFinder::FindInnerTypeOffset(PropertySize);
	std::cout << "Off::UArrayProperty::Inner: " << Off::UArrayProperty::Inner << "\n";
	
	Off::USetProperty::ElementProp = OffsetFinder::FindSetPropertyBaseOffset(PropertySize);
	std::cout << "Off::USetProperty::ElementProp: " << Off::USetProperty::ElementProp << "\n";
	
	Off::UMapProperty::Base = OffsetFinder::FindMapPropertyBaseOffset(PropertySize);
	std::cout << "Off::UMapProperty::Base: " << Off::UMapProperty::Base << "\n\n";

	Off::UByteProperty::Enum = PropertySize;
	Off::UBoolProperty::Base = PropertySize;
	Off::UObjectProperty::PropertyClass = PropertySize;
	Off::UStructProperty::Struct = PropertySize;
	Off::UEnumProperty::Base = PropertySize;
	
	Off::UClassProperty::MetaClass = PropertySize + 0x8; //0x8 inheritance from UObjectProperty
}
