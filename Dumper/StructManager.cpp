#include "StructManager.h"
#include "ObjectArray.h"

StructInfoHandle::StructInfoHandle(const StructManager* InManager, StructInfo InInfo)
	: Manager(InManager), Info(InInfo)
{
}

int32 StructInfoHandle::GetSize() const
{
	return Info.Size;
}

const StringEntry& StructInfoHandle::GetName() const
{
	return Manager->GetName(Info);
}

bool StructInfoHandle::IsFinal() const
{
	return Info.bIsFinal;
}

void StructManager::Init()
{
	if (bIsInitialized)
		return;

	bIsInitialized = true;

	StructInfoOverrides.reserve(0x2000);

	for (auto Obj : ObjectArray())
	{
		if (!Obj.IsA(EClassCastFlags::Struct) || Obj.IsA(EClassCastFlags::Function))
			continue;

		UEStruct ObjAsStruct = Obj.Cast<UEStruct>();

		// Add name to override info
		StructInfo& NewOrExistingInfo = StructInfoOverrides[Obj.GetIndex()];
		NewOrExistingInfo.Name = UniqueNameTable.FindOrAdd(Obj.GetCppName(), true).first;

		// Initialize struct-size if it wasn't set already
		if (NewOrExistingInfo.Size > ObjAsStruct.GetStructSize())
			NewOrExistingInfo.Size = ObjAsStruct.GetStructSize();

		UEStruct Super = ObjAsStruct.GetSuper();

		if (!Super)
			continue;

		int32 LowestOffset = INT_MAX;

		// Loop members to find member with the lowest offset
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
