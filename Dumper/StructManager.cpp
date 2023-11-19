#include "StructManager.h"
#include "ObjectArray.h"

template<typename T>
constexpr T Align(T Size, T Alignment)
{
	assert(Alignment != 0 && "Alignment was 0, division by zero exception.");

	return Size + (Alignment - (Size % Alignment));
}

StructInfoHandle::StructInfoHandle(const StructInfo& InInfo)
	: Info(InInfo)
{
}

int32 StructInfoHandle::GetSize() const
{
	return Align(Info.Size, Info.Alignment);
}

int32 StructInfoHandle::GetAlignment() const
{
	return Info.Alignment;
}

bool StructInfoHandle::ShouldUseExplicitAlignment() const
{
	return Info.bUseExplicitAlignment;
}

const StringEntry& StructInfoHandle::GetName() const
{
	return StructManager::GetName(Info);
}

bool StructInfoHandle::IsFinal() const
{
	return Info.bIsFinal;
}

void StructManager::InitAlignmentsAndNames()
{
	constexpr int DefaultClassAlignment = 0x8;

	for (auto Obj : ObjectArray())
	{
		if (!Obj.IsA(EClassCastFlags::Struct) || Obj.IsA(EClassCastFlags::Function))
			continue;

		UEStruct ObjAsStruct = Obj.Cast<UEStruct>();

		// Add name to override info
		StructInfo& NewOrExistingInfo = StructInfoOverrides[Obj.GetIndex()];
		NewOrExistingInfo.Name = UniqueNameTable.FindOrAdd(Obj.GetCppName()).first;

		UEStruct Super = ObjAsStruct.GetSuper();

		int32 HighestAlignment = ObjAsStruct.GetMinAlignment();
		bool bIsUsingMinAlignment = true;

		// Find member with the highest alignment
		for (UEProperty Property : ObjAsStruct.GetProperties())
		{
			if (!Property.IsA(EClassCastFlags::StructProperty))
				continue;

			int32 UnderlayingStructAlignment = Property.Cast<UEStructProperty>().GetUnderlayingStruct().GetMinAlignment();

			if (UnderlayingStructAlignment > HighestAlignment)
			{
				bIsUsingMinAlignment = false;
				HighestAlignment = UnderlayingStructAlignment;
			}
		}

		// if Class alignment is below pointer-alignment (0x8), use pointer-alignment instead, else use whichever, MinAlignment or HighestAlignment, is bigger
		if (ObjAsStruct.IsA(EClassCastFlags::Class) &&  HighestAlignment < DefaultClassAlignment)
		{
			NewOrExistingInfo.bUseExplicitAlignment = false;
			NewOrExistingInfo.Alignment = DefaultClassAlignment;
		}
		else
		{
			NewOrExistingInfo.bUseExplicitAlignment = bIsUsingMinAlignment;
			NewOrExistingInfo.Alignment = HighestAlignment;
		}
	}

	for (auto Obj : ObjectArray())
	{
		if (!Obj.IsA(EClassCastFlags::Struct) || Obj.IsA(EClassCastFlags::Function))
			continue;

		UEStruct ObjAsStruct = Obj.Cast<UEStruct>();

		constexpr int MaxNumSuperClasses = 0x20;

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
	for (auto Obj : ObjectArray())
	{
		if (!Obj.IsA(EClassCastFlags::Struct) || Obj.IsA(EClassCastFlags::Function))
			continue;

		UEStruct ObjAsStruct = Obj.Cast<UEStruct>();

		StructInfo& NewOrExistingInfo = StructInfoOverrides[Obj.GetIndex()];

		// Initialize struct-size if it wasn't set already
		if (NewOrExistingInfo.Size >= ObjAsStruct.GetStructSize())
			NewOrExistingInfo.Size = ObjAsStruct.GetStructSize();

		UEStruct Super = ObjAsStruct.GetSuper();

		if (!Super)
			continue;

		int32 LowestOffset = INT_MAX;
		int32 HighestAlignment = 0x0;

		// Find member with the lowest offset
		for (UEProperty Property : ObjAsStruct.GetProperties())
		{
			if (Property.Cast<UEProperty>().GetOffset() < LowestOffset)
				LowestOffset = Property.Cast<UEProperty>().GetOffset();
		}

		// Loop all super-structs and set their struct-size to the lowest offset we found. Sets this size on the direct Super and all higher *empty* supers
		for (UEStruct S = Super; S; S = S.GetSuper())
		{
			auto It = StructInfoOverrides.find(S.GetIndex());

			if (It != StructInfoOverrides.end())
			{
				StructInfo& Info = It->second;

				// Struct is not final, as it is another struct's super
				Info.bIsFinal = false;

				// Only change lowest offset if it's lower than the already found lowest offset
				if (Info.Size > LowestOffset)
					Info.Size = LowestOffset;
			}
			else
			{
				StructInfo& Info = StructInfoOverrides[S.GetIndex()];

				// Struct is not final, as it is another struct's super
				Info.bIsFinal = false;

				// Only add lowest offset if it's lower than the size ofthe struct
				Info.Size = LowestOffset < S.GetStructSize() ? LowestOffset : S.GetStructSize();
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
}
