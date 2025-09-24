#include <format>

#include "Utils.h"

#include "OffsetFinder/Offsets.h"
#include "OffsetFinder/OffsetFinder.h"

#include "Unreal/ObjectArray.h"
#include "Unreal/NameArray.h"

#include "Platform.h"
#include "Architecture.h"


void Off::InSDK::ProcessEvent::InitPE_Windows()
{
#ifdef PLATFORM_WINDOWS

	void** Vft = *(void***)ObjectArray::GetByIndex(0).GetAddress();

#if defined(_WIN64)
	/* Primary, and more reliable, check for ProcessEvent */
	auto IsProcessEvent = [](const uint8_t* FuncAddress, [[maybe_unused]] int32_t Index) -> bool
	{
		return Platform::FindPatternInRange({ 0xF7, -0x1, Off::UFunction::FunctionFlags, 0x0, 0x0, 0x0, 0x0, 0x04, 0x0, 0x0 }, FuncAddress, 0x400)
			&& Platform::FindPatternInRange({ 0xF7, -0x1, Off::UFunction::FunctionFlags, 0x0, 0x0, 0x0, 0x0, 0x0, 0x40, 0x0 }, FuncAddress, 0xF00);
	};
#elif defined(_WIN32)
	/* Primary, and more reliable, check for ProcessEvent */
	auto IsProcessEvent = [](const uint8_t* FuncAddress, [[maybe_unused]] int32_t Index) -> bool
	{
		return Platform::FindPatternInRange({ 0xF7, -0x1, Off::UFunction::FunctionFlags, 0x0, 0x4, 0x0, 0x0 }, FuncAddress, 0x400)
			&& Platform::FindPatternInRange({ 0xF7, -0x1, Off::UFunction::FunctionFlags, 0x0, 0x0, 0x40, 0x0 }, FuncAddress, 0xF00);
	};
#endif

	const void* ProcessEventAddr = nullptr;
	int32_t ProcessEventIdx = 0;

	const auto [FuncPtr, FuncIdx] = Platform::IterateVTableFunctions(Vft, IsProcessEvent);

	ProcessEventAddr = FuncPtr;
	ProcessEventIdx = FuncIdx;

	if (!FuncPtr)
	{
		const void* StringRefAddr = Platform::FindByStringInAllSections(L"Accessed None", 0x0, 0x0, Settings::General::bSearchOnlyExecutableSectionsForStrings);
		/* ProcessEvent is sometimes located right after a func with the string L"Accessed None. Might as well check for it, because else we're going to crash anyways. */
		const void* PossiblePEAddr = reinterpret_cast<void*>(Architecture_x86_64::FindNextFunctionStart(StringRefAddr));

		auto IsSameAddr = [PossiblePEAddr](const uint8_t* FuncAddress, [[maybe_unused]] int32_t Index) -> bool
		{
			return FuncAddress == PossiblePEAddr;
		};

		const auto [FuncPtr2, FuncIdx2] = Platform::IterateVTableFunctions(Vft, IsSameAddr);
		ProcessEventAddr = FuncPtr2;
		ProcessEventIdx = FuncIdx2;
	}

	if (ProcessEventAddr)
	{
		Off::InSDK::ProcessEvent::PEIndex = ProcessEventIdx;
		Off::InSDK::ProcessEvent::PEOffset = Platform::GetOffset(ProcessEventAddr);

		std::cerr << std::format("PE-Offset: 0x{:X}\n", Off::InSDK::ProcessEvent::PEOffset);
		std::cerr << std::format("PE-Index: 0x{:X}\n\n", ProcessEventIdx);
		return;
	}

	std::cerr << "\nCouldn't find ProcessEvent!\n\n" << std::endl;

#endif // PLATFORM_WINDOWS
}

void Off::InSDK::ProcessEvent::InitPE(const int32 Index, const char* const ModuleName)
{
	Off::InSDK::ProcessEvent::PEIndex = Index;

	void** VFT = *reinterpret_cast<void***>(ObjectArray::GetByIndex(0).GetAddress());

	Off::InSDK::ProcessEvent::PEOffset = Platform::GetOffset(VFT[Off::InSDK::ProcessEvent::PEIndex], ModuleName);

	std::cerr << std::format("PE-Offset: 0x{:X}\n", Off::InSDK::ProcessEvent::PEOffset);
}

/* UWorld */
void Off::InSDK::World::InitGWorld()
{
	UEClass UWorld = ObjectArray::FindClassFast("World");

	for (UEObject Obj : ObjectArray())
	{
		if (Obj.HasAnyFlags(EObjectFlags::ClassDefaultObject) || !Obj.IsA(UWorld))
			continue;

		/* Try to find a pointer to the word, aka UWorld** GWorld */
		auto Results = Platform::FindAllAlignedValuesInProcess(Obj.GetAddress());

		void* Result = nullptr;
		if (Results.size())
		{
			if (Results.size() == 1)
			{
				Result = Results[0];
			}
			else if (Results.size() == 2)
			{
				auto ObjAddress = reinterpret_cast<uintptr_t>(Obj.GetAddress());
				auto PossibleGWorld = reinterpret_cast<volatile uintptr_t*>(Results[0]);
				auto CurrentValue = *PossibleGWorld;

				for (int i = 0; CurrentValue == ObjAddress && i < 50; ++i)
				{
					::Sleep(1);
					CurrentValue = *PossibleGWorld;
				}
				if (CurrentValue == ObjAddress)
				{
					Result = Results[0];
				}
				else
				{
					Result = Results[1];
					std::cerr << std::format("Filter GActiveLogWorld at 0x{:X}\n\n", reinterpret_cast<uintptr_t>(PossibleGWorld));
				}
			}
			else
			{
				std::cerr << std::format("Detected {} GWorld \n\n", Results.size());
			}
		}

		/* Pointer to UWorld* couldn't be found */
		if (Result)
		{
			Off::InSDK::World::GWorld = Platform::GetOffset(Result);
			std::cerr << std::format("GWorld-Offset: 0x{:X}\n\n", Off::InSDK::World::GWorld);
			break;
		}
	}

	if (Off::InSDK::World::GWorld == 0x0)
		std::cerr << std::format("\nGWorld WAS NOT FOUND!!!!!!!!!\n\n");
}

/* FText */
void Off::InSDK::Text::InitTextOffsets()
{
	if (!Off::InSDK::ProcessEvent::PEIndex)
	{
		std::cerr << std::format("\nDumper-7: Error, 'InitInSDKTextOffsets' was called before ProcessEvent was initialized!\n") << std::endl;
		return;
	}

	auto IsValidPtr = [](void* a) -> bool
	{
		return !Platform::IsBadReadPtr(a) /* && (uintptr_t(a) & 0x1) == 0*/; // realistically, there wont be any pointers to unaligned memory
	};


	const UEFunction Conv_StringToText = ObjectArray::FindObjectFast<UEFunction>("Conv_StringToText", EClassCastFlags::Function);

	UEProperty InStringProp = nullptr;
	UEProperty ReturnProp = nullptr;

	if (!Conv_StringToText)
	{
		std::cerr << "Conv_StringToText is invalid!\n";
		return;
	}

	for (UEProperty Prop : Conv_StringToText.GetProperties())
	{
		/* Func has 2 params, if the param is the return value assign to ReturnProp, else InStringProp*/
		if (Prop.HasPropertyFlags(EPropertyFlags::ReturnParm))
		{
			ReturnProp = Prop;
		}
		else
		{
			InStringProp = Prop;
		}
	}

	const int32 ParamSize = Conv_StringToText.GetStructSize();
	const int32 FTextSize = ReturnProp.GetSize();

	const int32 StringOffset = InStringProp.GetOffset();
	const int32 ReturnValueOffset = ReturnProp.GetOffset();

	Off::InSDK::Text::TextSize = FTextSize;


	/* Allocate and zero-initialize ParamStruct */
#pragma warning(disable: 6255)
	uint8_t* ParamPtr = static_cast<uint8_t*>(alloca(ParamSize));
	memset(ParamPtr, 0, ParamSize);

	/* Choose a, fairly random, string to later search for in FTextData */
	constexpr const wchar_t* StringText = L"ThisIsAGoodString!";
	constexpr int32 StringLength = (sizeof(L"ThisIsAGoodString!") / sizeof(wchar_t));
	constexpr int32 StringLengthBytes = (sizeof(L"ThisIsAGoodString!"));

	/* Initialize 'InString' in the ParamStruct */
	*reinterpret_cast<FString*>(ParamPtr + StringOffset) = StringText;

	/* This function is 'static' so the object on which we call it doesn't matter */
	ObjectArray::GetByIndex(0).ProcessEvent(Conv_StringToText, ParamPtr);

	uint8_t* FTextDataPtr = nullptr;

	/* Search for the first valid pointer inside of the FText and make the offset our 'TextDatOffset' */
	for (int32 i = 0; i < (FTextSize - sizeof(void*)); i += sizeof(void*))
	{
		void* PossibleTextDataPtr = *reinterpret_cast<void**>(ParamPtr + ReturnValueOffset + i);

		if (IsValidPtr(PossibleTextDataPtr))
		{
			FTextDataPtr = static_cast<uint8_t*>(PossibleTextDataPtr);
			Off::InSDK::Text::TextDatOffset = i;
			break;
		}
	}

	if (!FTextDataPtr)
	{
		std::cerr << std::format("\nDumper-7: Error, 'FTextDataPtr' could not be found!\n") << std::endl;
		return;
	}

	constexpr int32 MaxOffset = 0x50;
	constexpr int32 StartOffset = sizeof(void*); // FString::NumElements offset

	/* Search for a pointer pointing to a int32 Value (FString::NumElements) equal to StringLength */
	for (int32 i = StartOffset; i < MaxOffset; i += sizeof(int32))
	{
		wchar_t* PosibleStringPtr = *reinterpret_cast<wchar_t**>((FTextDataPtr + i) - sizeof(void*));
		const int32 PossibleLength = *reinterpret_cast<int32*>(FTextDataPtr + i);

		if (PossibleLength == StringLength && PosibleStringPtr && IsValidPtr(PosibleStringPtr) && memcmp(StringText, PosibleStringPtr, StringLengthBytes) == 0)
		{
			Off::InSDK::Text::InTextDataStringOffset = (i - sizeof(void*));
			break;
		}
	}

	std::cerr << std::format("Off::InSDK::Text::TextSize: 0x{:X}\n", Off::InSDK::Text::TextSize);
	std::cerr << std::format("Off::InSDK::Text::TextDatOffset: 0x{:X}\n", Off::InSDK::Text::TextDatOffset);
	std::cerr << std::format("Off::InSDK::Text::InTextDataStringOffset: 0x{:X}\n\n", Off::InSDK::Text::InTextDataStringOffset);
}

void Off::Init()
{
	auto OverwriteIfInvalidOffset = [](int32& Offset, int32 DefaultValue)
	{
		if (Offset == OffsetFinder::OffsetNotFound)
		{
			std::cerr << std::format("Defaulting to offset: 0x{:X}\n", DefaultValue);
			Offset = DefaultValue;
		}
	};

	Off::UObject::Flags = OffsetFinder::FindUObjectFlagsOffset();
	OverwriteIfInvalidOffset(Off::UObject::Flags, sizeof(void*)); // Default to right after VTable
	std::cerr << std::format("Off::UObject::Flags: 0x{:X}\n", Off::UObject::Flags);

	Off::UObject::Index = OffsetFinder::FindUObjectIndexOffset();
	OverwriteIfInvalidOffset(Off::UObject::Index, (Off::UObject::Flags + sizeof(int32))); // Default to right after Flags
	std::cerr << std::format("Off::UObject::Index: 0x{:X}\n", Off::UObject::Index);

	Off::UObject::Class = OffsetFinder::FindUObjectClassOffset();
	OverwriteIfInvalidOffset(Off::UObject::Class, (Off::UObject::Index + sizeof(int32))); // Default to right after Index
	std::cerr << std::format("Off::UObject::Class: 0x{:X}\n", Off::UObject::Class);

	Off::UObject::Outer = OffsetFinder::FindUObjectOuterOffset();
	std::cerr << std::format("Off::UObject::Outer: 0x{:X}\n", Off::UObject::Outer);

	Off::UObject::Name = OffsetFinder::FindUObjectNameOffset();
	OverwriteIfInvalidOffset(Off::UObject::Name, (Off::UObject::Class + sizeof(void*))); // Default to right after Class
	std::cerr << std::format("Off::UObject::Name: 0x{:X}\n\n", Off::UObject::Name);

	OverwriteIfInvalidOffset(Off::UObject::Outer, (Off::UObject::Name + sizeof(int32) + sizeof(int32)));  // Default to right after Name

	OffsetFinder::InitFNameSettings();

	::NameArray::PostInit();

	// Castflags needs to stay here since the FindChildOffset() uses CastFlags
	Off::UClass::CastFlags = OffsetFinder::FindCastFlagsOffset();
	std::cerr << std::format("Off::UClass::CastFlags: 0x{:X}\n", Off::UClass::CastFlags);

	Off::UStruct::Children = OffsetFinder::FindChildOffset();
	std::cerr << std::format("Off::UStruct::Children: 0x{:X}\n", Off::UStruct::Children);

	Off::UField::Next = OffsetFinder::FindUFieldNextOffset();
	std::cerr << std::format("Off::UField::Next: 0x{:X}\n", Off::UField::Next);

	Off::UStruct::SuperStruct = OffsetFinder::FindSuperOffset();
	std::cerr << std::format("Off::UStruct::SuperStruct: 0x{:X}\n", Off::UStruct::SuperStruct);

	Off::UStruct::Size = OffsetFinder::FindStructSizeOffset();
	std::cerr << std::format("Off::UStruct::Size: 0x{:X}\n", Off::UStruct::Size);

	Off::UStruct::MinAlignment = OffsetFinder::FindMinAlignmentOffset();
	std::cerr << std::format("Off::UStruct::MinAlignment: 0x{:X}\n", Off::UStruct::MinAlignment);

	Off::UClass::CastFlags = OffsetFinder::FindCastFlagsOffset();
	std::cerr << std::format("Off::UClass::CastFlags: 0x{:X}\n", Off::UClass::CastFlags);

	// Castflags become available for use

	if (Settings::Internal::bUseFProperty)
	{
		std::cerr << std::format("\nGame uses FProperty system\n\n");

		Off::UStruct::ChildProperties = OffsetFinder::FindChildPropertiesOffset();
		std::cerr << std::format("Off::UStruct::ChildProperties: 0x{:X}\n", Off::UStruct::ChildProperties);

		OffsetFinder::FixupHardcodedOffsets(); // must be called after FindChildPropertiesOffset 

		Off::FField::Next = OffsetFinder::FindFFieldNextOffset();
		std::cerr << std::format("Off::FField::Next: 0x{:X}\n", Off::FField::Next);

		Off::FField::Class = OffsetFinder::FindFFieldClassOffset();
		std::cerr << std::format("Off::FField::Class: 0x{:X}\n", Off::FField::Class);

		// Comment out this line if you're crashing here and see if the NewFindFFieldNameOffset might work!
		Off::FField::Name = OffsetFinder::FindFFieldNameOffset();
		//Off::FField::Name = OffsetFinder::NewFindFFieldNameOffset();

		if (Off::FField::Name == OffsetFinder::OffsetNotFound)
			Off::FField::Name = OffsetFinder::NewFindFFieldNameOffset();

		std::cerr << std::format("Off::FField::Name: 0x{:X}\n", Off::FField::Name);

		/*
		* FNameSize might be wrong at this point of execution.
		* FField::Flags is not critical so a fix is only applied later in OffsetFinder::PostInitFNameSettings().
		*/
		Off::FField::Flags = Off::FField::Name + Off::InSDK::Name::FNameSize;
		std::cerr << std::format("Off::FField::Flags: 0x{:X}\n", Off::FField::Flags);
	}

	Off::UClass::ClassDefaultObject = OffsetFinder::FindDefaultObjectOffset();
	std::cerr << std::format("Off::UClass::ClassDefaultObject: 0x{:X}\n", Off::UClass::ClassDefaultObject);

	Off::UClass::ImplementedInterfaces = OffsetFinder::FindImplementedInterfacesOffset();
	std::cerr << std::format("Off::UClass::ImplementedInterfaces: 0x{:X}\n", Off::UClass::ImplementedInterfaces);

	Off::UEnum::Names = OffsetFinder::FindEnumNamesOffset();
	std::cerr << std::format("Off::UEnum::Names: 0x{:X}\n", Off::UEnum::Names) << std::endl;

	Off::UFunction::FunctionFlags = OffsetFinder::FindFunctionFlagsOffset();
	std::cerr << std::format("Off::UFunction::FunctionFlags: 0x{:X}\n", Off::UFunction::FunctionFlags);

	Off::UFunction::ExecFunction = OffsetFinder::FindFunctionNativeFuncOffset();
	std::cerr << std::format("Off::UFunction::ExecFunction: 0x{:X}\n", Off::UFunction::ExecFunction) << std::endl;

	Off::Property::ElementSize = OffsetFinder::FindElementSizeOffset();
	std::cerr << std::format("Off::Property::ElementSize: 0x{:X}\n", Off::Property::ElementSize);

	Off::Property::ArrayDim = OffsetFinder::FindArrayDimOffset();
	std::cerr << std::format("Off::Property::ArrayDim: 0x{:X}\n", Off::Property::ArrayDim);

	Off::Property::Offset_Internal = OffsetFinder::FindOffsetInternalOffset();
	std::cerr << std::format("Off::Property::Offset_Internal: 0x{:X}\n", Off::Property::Offset_Internal);

	Off::Property::PropertyFlags = OffsetFinder::FindPropertyFlagsOffset();
	std::cerr << std::format("Off::Property::PropertyFlags: 0x{:X}\n", Off::Property::PropertyFlags);

	Off::BoolProperty::Base = OffsetFinder::FindBoolPropertyBaseOffset();
	std::cerr << std::format("UBoolProperty::Base: 0x{:X}\n", Off::BoolProperty::Base) << std::endl;

	Off::EnumProperty::Base = OffsetFinder::FindEnumPropertyBaseOffset();
	std::cerr << std::format("Off::EnumProperty::Base: 0x{:X}\n", Off::EnumProperty::Base) << std::endl;


	if (Off::EnumProperty::Base == OffsetFinder::OffsetNotFound)
	{
		Off::InSDK::Properties::PropertySize = Off::BoolProperty::Base;
		Off::EnumProperty::Base = Off::BoolProperty::Base;
	}
	else
	{
		Off::InSDK::Properties::PropertySize = Off::EnumProperty::Base;
	}

	std::cerr << std::format("UPropertySize: 0x{:X}\n", Off::InSDK::Properties::PropertySize) << std::endl;

	Off::ObjectProperty::PropertyClass = OffsetFinder::FindObjectPropertyClassOffset();
	std::cerr << std::format("Off::ObjectProperty::PropertyClass: 0x{:X}", Off::ObjectProperty::PropertyClass) << std::endl;
	OverwriteIfInvalidOffset(Off::ObjectProperty::PropertyClass, Off::InSDK::Properties::PropertySize);

	Off::ByteProperty::Enum = OffsetFinder::FindBytePropertyEnumOffset();
	OverwriteIfInvalidOffset(Off::ByteProperty::Enum, Off::InSDK::Properties::PropertySize);
	std::cerr << std::format("Off::ByteProperty::Enum: 0x{:X}", Off::ByteProperty::Enum) << std::endl;

	Off::StructProperty::Struct = OffsetFinder::FindStructPropertyStructOffset();
	OverwriteIfInvalidOffset(Off::StructProperty::Struct, Off::InSDK::Properties::PropertySize);
	std::cerr << std::format("Off::StructProperty::Struct: 0x{:X}\n", Off::StructProperty::Struct) << std::endl;

	Off::DelegateProperty::SignatureFunction = OffsetFinder::FindDelegatePropertySignatureFunctionOffset();
	OverwriteIfInvalidOffset(Off::DelegateProperty::SignatureFunction, Off::InSDK::Properties::PropertySize);
	std::cerr << std::format("Off::DelegateProperty::SignatureFunction: 0x{:X}\n", Off::DelegateProperty::SignatureFunction) << std::endl;

	Off::ArrayProperty::Inner = OffsetFinder::FindInnerTypeOffset(Off::InSDK::Properties::PropertySize);
	std::cerr << std::format("Off::ArrayProperty::Inner: 0x{:X}\n", Off::ArrayProperty::Inner);

	Off::SetProperty::ElementProp = OffsetFinder::FindSetPropertyBaseOffset(Off::InSDK::Properties::PropertySize);
	std::cerr << std::format("Off::SetProperty::ElementProp: 0x{:X}\n", Off::SetProperty::ElementProp);

	Off::MapProperty::Base = OffsetFinder::FindMapPropertyBaseOffset(Off::InSDK::Properties::PropertySize);
	std::cerr << std::format("Off::MapProperty::Base: 0x{:X}\n", Off::MapProperty::Base) << std::endl;

	Off::InSDK::ULevel::Actors = OffsetFinder::FindLevelActorsOffset();
	std::cerr << std::format("Off::InSDK::ULevel::Actors: 0x{:X}\n", Off::InSDK::ULevel::Actors) << std::endl;

	Off::InSDK::UDataTable::RowMap = OffsetFinder::FindDatatableRowMapOffset();
	std::cerr << std::format("Off::InSDK::UDataTable::RowMap: 0x{:X}\n", Off::InSDK::UDataTable::RowMap) << std::endl;

	OffsetFinder::PostInitFNameSettings();

	std::cerr << std::endl;

	Off::FieldPathProperty::FieldClass = Off::InSDK::Properties::PropertySize;
	Off::OptionalProperty::ValueProperty = Off::InSDK::Properties::PropertySize;

	Off::ClassProperty::MetaClass = Off::ObjectProperty::PropertyClass + sizeof(void*); //0x8 inheritance from ObjectProperty
}

void PropertySizes::Init()
{
	InitTDelegateSize();
	InitFFieldPathSize();
}

void PropertySizes::InitTDelegateSize()
{
	/* If the AudioComponent class or the OnQueueSubtitles member weren't found, fallback to looping GObjects and looking for a Delegate. */
	auto OnPropertyNotFoudn = [&]() -> void
	{
		for (UEObject Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct))
				continue;

			for (UEProperty Prop : Obj.Cast<UEClass>().GetProperties())
			{
				if (Prop.IsA(EClassCastFlags::DelegateProperty))
				{
					PropertySizes::DelegateProperty = Prop.GetSize();
					return;
				}
			}
		}
	};

	const UEClass AudioComponentClass = ObjectArray::FindClassFast("AudioComponent");

	if (!AudioComponentClass)
		return OnPropertyNotFoudn();

	const UEProperty OnQueueSubtitlesProp = AudioComponentClass.FindMember("OnQueueSubtitles", EClassCastFlags::DelegateProperty);

	if (!OnQueueSubtitlesProp)
		return OnPropertyNotFoudn();

	PropertySizes::DelegateProperty = OnQueueSubtitlesProp.GetSize();
}

void PropertySizes::InitFFieldPathSize()
{
	if (!Settings::Internal::bUseFProperty)
		return;

	/* If the SetFieldPathPropertyByName function or the Value parameter weren't found, fallback to looping GObjects and looking for a Delegate. */
	auto OnPropertyNotFoudn = [&]() -> void
	{
		for (UEObject Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct))
				continue;

			for (UEProperty Prop : Obj.Cast<UEClass>().GetProperties())
			{
				if (Prop.IsA(EClassCastFlags::FieldPathProperty))
				{
					PropertySizes::FieldPathProperty = Prop.GetSize();
					return;
				}
			}
		}
	};

	const UEFunction SetFieldPathPropertyByNameFunc = ObjectArray::FindObjectFast<UEFunction>("SetFieldPathPropertyByName", EClassCastFlags::Function);

	if (!SetFieldPathPropertyByNameFunc)
		return OnPropertyNotFoudn();

	const UEProperty ValueParamProp = SetFieldPathPropertyByNameFunc.FindMember("Value", EClassCastFlags::FieldPathProperty);

	if (!ValueParamProp)
		return OnPropertyNotFoudn();

	PropertySizes::FieldPathProperty = ValueParamProp.GetSize();
}