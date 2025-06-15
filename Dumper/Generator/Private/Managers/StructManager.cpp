#include "Unreal/ObjectArray.h"
#include "Managers/StructManager.h"

StructInfoHandle::StructInfoHandle(const StructInfo& InInfo)
	: Info(&InInfo)
{
}

int32 StructInfoHandle::GetLastMemberEnd() const
{
	return Info->LastMemberEnd;
}

int32 StructInfoHandle::GetSize() const
{
	return Align(Info->Size, Info->Alignment);
}

int32 StructInfoHandle::GetUnalignedSize() const
{
	return Info->Size;
}

int32 StructInfoHandle::GetAlignment() const
{
	return Info->Alignment;
}

bool StructInfoHandle::ShouldUseExplicitAlignment() const
{
	return Info->bUseExplicitAlignment;
}

const StringEntry& StructInfoHandle::GetName() const
{
	return StructManager::GetName(*Info);
}

bool StructInfoHandle::IsFinal() const
{
	return Info->bIsFinal;
}

bool StructInfoHandle::HasReusedTrailingPadding() const
{
	return Info->bHasReusedTrailingPadding;
}

bool StructInfoHandle::IsPartOfCyclicPackage() const
{
	return Info->bIsPartOfCyclicPackage;
}

void StructManager::InitAlignmentsAndNames()
{
	constexpr int32 DefaultClassAlignment = sizeof(void*);

	const UEClass InterfaceClass = ObjectArray::FindClassFast("Interface");

	for (auto Obj : ObjectArray())
	{
		if (!Obj.IsA(EClassCastFlags::Struct) /* || Obj.IsA(EClassCastFlags::Function)*/)
			continue;

		UEStruct ObjAsStruct = Obj.Cast<UEStruct>();

		// Add name to override info
		StructInfo& NewOrExistingInfo = StructInfoOverrides[Obj.GetIndex()];
		NewOrExistingInfo.Name = UniqueNameTable.FindOrAdd(Obj.GetCppName(), !Obj.IsA(EClassCastFlags::Function)).first;

		// Interfaces inherit from UObject by default, but as a workaround to no virtual-inheritance we make them empty
		if (ObjAsStruct.HasType(InterfaceClass))
		{
			NewOrExistingInfo.Alignment = 0x1;
			NewOrExistingInfo.bHasReusedTrailingPadding = false;
			NewOrExistingInfo.bIsFinal = true;
			NewOrExistingInfo.Size = 0x0;

			continue;
		}

		int32 MinAlignment = ObjAsStruct.GetMinAlignment();
		int32 HighestMemberAlignment = 0x1; // starting at 0x1 when checking **all**, not just struct-properties

		// Find member with the highest alignment
		for (UEProperty Property : ObjAsStruct.GetProperties())
		{
			int32 CurrentPropertyAlignment = Property.GetAlignment();

			if (CurrentPropertyAlignment > HighestMemberAlignment)
				HighestMemberAlignment = CurrentPropertyAlignment;
		}

		/* On some strange games there are BlueprintGeneratedClass UClasses which don't inherit from UObject. */
		const bool bHasSuperClass = static_cast<bool>(ObjAsStruct.GetSuper());

		// if Class alignment is below pointer-alignment (0x8), use pointer-alignment instead, else use whichever, MinAlignment or HighestAlignment, is bigger
		if (ObjAsStruct.IsA(EClassCastFlags::Class) && bHasSuperClass && HighestMemberAlignment < DefaultClassAlignment)
		{
			NewOrExistingInfo.bUseExplicitAlignment = false;
			NewOrExistingInfo.Alignment = DefaultClassAlignment;
		}
		else
		{
			NewOrExistingInfo.bUseExplicitAlignment = MinAlignment > HighestMemberAlignment;
			NewOrExistingInfo.Alignment = max(MinAlignment, HighestMemberAlignment);
		}
	}

	for (auto Obj : ObjectArray())
	{
		if (!Obj.IsA(EClassCastFlags::Struct) || Obj.IsA(EClassCastFlags::Function) || Obj.Cast<UEStruct>().HasType(InterfaceClass))
			continue;

		UEStruct ObjAsStruct = Obj.Cast<UEStruct>();

		constexpr int MaxNumSuperClasses = 0x30;

		std::array<UEStruct, MaxNumSuperClasses> StructStack;
		int32 NumElementsInStructStack = 0x0;

		// Get a top to bottom list of a struct and all of its supers
		for (UEStruct S = ObjAsStruct; S; S = S.GetSuper())
		{
			StructStack[NumElementsInStructStack] = S;
			NumElementsInStructStack++;
		}

		int32 CurrentHighestAlignment = 0x0;

		for (int i = NumElementsInStructStack - 1; i >= 0; i--)
		{
			StructInfo& Info = StructInfoOverrides[StructStack[i].GetIndex()];

			if (CurrentHighestAlignment < Info.Alignment)
			{
				CurrentHighestAlignment = Info.Alignment;
			}
			else
			{
				// We use the super classes' alignment, no need to explicitely set it
				Info.bUseExplicitAlignment = false; 
				Info.Alignment = CurrentHighestAlignment;
			}
		}
	}
}

void StructManager::InitSizesAndIsFinal()
{
	const UEClass InterfaceClass = ObjectArray::FindClassFast("Interface");

	for (auto Obj : ObjectArray())
	{
		if (!Obj.IsA(EClassCastFlags::Struct) || Obj.Cast<UEStruct>().HasType(InterfaceClass))
			continue;

		UEStruct ObjAsStruct = Obj.Cast<UEStruct>();

		StructInfo& NewOrExistingInfo = StructInfoOverrides[Obj.GetIndex()];

		// Initialize struct-size if it wasn't set already
		if (NewOrExistingInfo.Size > ObjAsStruct.GetStructSize())
			NewOrExistingInfo.Size = ObjAsStruct.GetStructSize();

		UEStruct Super = ObjAsStruct.GetSuper();

		if (NewOrExistingInfo.Size == 0x0 && Super != nullptr)
			NewOrExistingInfo.Size = Super.GetStructSize();

		int32 LastMemberEnd = 0x0;
		int32 LowestOffset = INT_MAX;

		// Find member with the lowest offset
		for (UEProperty Property : ObjAsStruct.GetProperties())
		{
			const int32 PropertyOffset = Property.GetOffset();
			const int32 PropertySize = Property.GetSize();

			if (PropertyOffset < LowestOffset)
				LowestOffset = PropertyOffset;

			if ((PropertyOffset + PropertySize) > LastMemberEnd)
				LastMemberEnd = PropertyOffset + PropertySize;
		}

		/* No need to check any other structs, as finding the LastMemberEnd only involves this struct */
		NewOrExistingInfo.LastMemberEnd = LastMemberEnd;

		if (!Super || Obj.IsA(EClassCastFlags::Function))
			continue;

		/*
		* Loop all super-structs and set their struct-size to the lowest offset we found. Sets this size on the direct Super and all higher *empty* supers
		* 
		* breaks out of the loop after encountering a super-struct which is not empty (aka. has member-variables)
		*/
		for (UEStruct S = Super; S; S = S.GetSuper())
		{
			auto It = StructInfoOverrides.find(S.GetIndex());

			if (It == StructInfoOverrides.end())
			{
				std::cerr << "\n\n\nDumper-7: Error, struct wasn't found in 'StructInfoOverrides'! Exiting...\n\n\n" << std::endl;
				Sleep(10000);
				exit(1);
			}

			StructInfo& Info = It->second;

			// Struct is not final, as it is another structs' super
			Info.bIsFinal = false;

			const int32 SizeToCheck = Info.Size == INT_MAX ? S.GetStructSize() : Info.Size;

			// Only change lowest offset if it's lower than the already found lowest offset (by default: struct-size)
			if (Align(SizeToCheck, Info.Alignment) > LowestOffset)
			{
				if (Info.Size > LowestOffset)
					Info.Size = LowestOffset;

				Info.bHasReusedTrailingPadding = true;
			}

			if (S.HasMembers())
				break;
		}
	}
}

void StructManager::Init()
{
	if (bIsInitialized)
		return;

	bIsInitialized = true;

	StructInfoOverrides.reserve(0x2000);

	InitAlignmentsAndNames();
	InitSizesAndIsFinal();

	/* 
	* The default class-alignment of 0x8 is only set for classes with a valid Super-class, because they inherit from UObject. 
	* UObject however doesn't have a super, so this needs to be set manually.
	*/
	const UEObject UObjectClass = ObjectArray::FindClassFast("Object");
	StructInfoOverrides.find(UObjectClass.GetIndex())->second.Alignment = sizeof(void*);

	/* I still hate whoever decided to call "UStruct" "Ustruct" on some UE versions. */
	if (const UEObject UStructClass = ObjectArray::FindClassFast("struct"))
		StructInfoOverrides.find(UStructClass.GetIndex())->second.Name = UniqueNameTable.FindOrAdd(std::string("UStruct"), false).first;
}
