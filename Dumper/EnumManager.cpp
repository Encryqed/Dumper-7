#include "EnumManager.h"


EnumInfoHandle::EnumInfoHandle(const EnumInfo& InInfo)
	: Info(&InInfo)
{
}

int32 EnumInfoHandle::GetUnderlyingTypeSize() const
{
	return Info->UnderlyingTypeSize;
}

const StringEntry& EnumInfoHandle::GetName() const
{
	return EnumManager::GetName(*Info);
}


void EnumManager::InitUnderlayingSizesAndName()
{
	for (auto Obj : ObjectArray())
	{
		if (Obj.IsA(EClassCastFlags::Struct))
		{
			UEStruct ObjAsStruct = Obj.Cast<UEStruct>();

			for (UEProperty Property : ObjAsStruct.GetProperties())
			{
				if (!Property.IsA(EClassCastFlags::EnumProperty))
					continue;

				UEEnum Enum = Property.Cast<UEEnumProperty>().GetEnum();

				if (!Enum)
					continue;

				if (Property.Cast<UEEnumProperty>().GetSize() != 0x1) [[unlikely]]
				{
					EnumInfoOverrides[Enum.GetIndex()].UnderlyingTypeSize = Property.Cast<UEEnumProperty>().GetSize();
					continue;
				}

				UEProperty UnderlayingProperty = Property.Cast<UEEnumProperty>().GetUnderlayingProperty();

				if (!UnderlayingProperty) [[unlikely]]
					continue;

				if (UnderlayingProperty.GetSize() != 0x1) [[unlikely]]
					EnumInfoOverrides[Enum.GetIndex()].UnderlyingTypeSize = UnderlayingProperty.GetSize();
			}
		}
		else if (Obj.IsA(EClassCastFlags::Enum))
		{
			// Add name to override info
			EnumInfo& NewOrExistingInfo = EnumInfoOverrides[Obj.GetIndex()];
			NewOrExistingInfo.Name = UniqueNameTable.FindOrAdd(Obj.Cast<UEEnum>().GetEnumPrefixedName()).first;
		}
	}
}

void EnumManager::Init()
{
	if (bIsInitialized)
		return;

	bIsInitialized = true;

	EnumInfoOverrides.reserve(0x1000);

	InitUnderlayingSizesAndName();
}