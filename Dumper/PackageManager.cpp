#include "PackageManager.h"
#include "ObjectArray.h"


void GetPropertyDependency(UEProperty Prop, std::unordered_set<int32>& Store)
{
	if (Prop.IsA(EClassCastFlags::StructProperty))
	{
		Store.insert(Prop.Cast<UEStructProperty>().GetUnderlayingStruct().GetIndex());
	}
	else if (Prop.IsA(EClassCastFlags::EnumProperty))
	{
		if (auto Enum = Prop.Cast<UEEnumProperty>().GetEnum())
			Store.insert(Enum.GetIndex());
	}
	else if (Prop.IsA(EClassCastFlags::ByteProperty))
	{
		if (UEObject Enum = Prop.Cast<UEByteProperty>().GetEnum())
			Store.insert(Enum.GetIndex());
	}
	else if (Prop.IsA(EClassCastFlags::ArrayProperty))
	{
		GetPropertyDependency(Prop.Cast<UEArrayProperty>().GetInnerProperty(), Store);
	}
	else if (Prop.IsA(EClassCastFlags::SetProperty))
	{
		GetPropertyDependency(Prop.Cast<UESetProperty>().GetElementProperty(), Store);
	}
	else if (Prop.IsA(EClassCastFlags::MapProperty))
	{
		GetPropertyDependency(Prop.Cast<UEMapProperty>().GetKeyProperty(), Store);
		GetPropertyDependency(Prop.Cast<UEMapProperty>().GetValueProperty(), Store);
	}
}

void PackageManager::InitPackageDependencies()
{
	// Collects all packages required to compile this file
	static inline auto AddMemberDependenciesToList = [](DependencyListType& DependencyTracker, UEStruct Struct) -> void
	{
		std::unordered_set<int32> Dependencies;

		for (UEProperty Property : Struct.GetProperties())
		{
			GetPropertyDependency(Property, Dependencies);
		}

		for (int32 Dependency : Dependencies)
		{
			DependencyTracker[ObjectArray::GetByIndex(Dependency).GetPackageIndex()].bShouldInclude.Structs = true;
		}
	};

	for (auto Obj : ObjectArray())
	{
		int32 CurrentPackageIdx = Obj.GetOutermost().GetIndex();

		if (Obj.IsA(EClassCastFlags::Struct) && !Obj.IsA(EClassCastFlags::Function))
		{
			PackageInfo& Info = PackageInfos[CurrentPackageIdx];

			UEStruct ObjAsStruct = Obj.Cast<UEStruct>();

			Info.bHasFunctions = true;
			Info.Functions.push_back(Obj.GetIndex());

			AddMemberDependenciesToList(Info.PackageDependencies.StructsDependencies, ObjAsStruct);

			if (ObjAsStruct.IsA(EClassCastFlags::Class))
			{
				Info.bHasClasses = true;

				if (UEStruct Super = ObjAsStruct.GetSuper())
					Info.PackageDependencies.StructsDependencies[Super.GetPackageIndex()].bShouldInclude.Classes = true;

				for (UEFunction Func : ObjAsStruct.GetFunctions())
				{
					Info.bHasFunctions = true;
					Info.Functions.push_back(Obj.GetIndex());
					AddMemberDependenciesToList(Info.PackageDependencies.ParametersDependencies, Obj.Cast<UEFunction>());
				}
			}
		}
		else if (Obj.IsA(EClassCastFlags::Enum))
		{
			PackageInfo& Info = PackageInfos[CurrentPackageIdx];

			Info.bHasEnums = true;
			Info.Enums.push_back(Obj.GetIndex());
		}
	}
}

void PackageManager::Init()
{
	if (bIsInitialized)
		return;

	bIsInitialized = true;

	PackageInfos.reserve(0x800);

	InitInternal();
}
