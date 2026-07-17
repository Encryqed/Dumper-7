#include "SharedPredefinedMembers.h"

#include "Unreal/ObjectArray.h"

void InitCorePredefinedMembers(PredefinedMemberLookupMapType& OutMembers)
{
	if (Off::InSDK::ULevel::Actors != -1)
	{
		UEClass Level = ObjectArray::FindClassFast("Level");

		if (Level == nullptr)
			Level = ObjectArray::FindClassFast("level");

		PredefinedElements& ULevelPredefs = OutMembers[Level.GetIndex()];
		ULevelPredefs.Members =
		{
			PredefinedMember {
				.Comment = "THIS IS THE ARRAY YOU'RE LOOKING FOR! [NOT AUTO-GENERATED PROPERTY]",
				.Type = "class TArray<class AActor*>", .Name = "Actors", .Offset = Off::InSDK::ULevel::Actors, .Size = sizeof(TArray<int>), .ArrayDim = 0x1, .Alignment = alignof(TArray<int>),
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			},
		};
	}

	UEClass DataTable = ObjectArray::FindClassFast("DataTable");

	PredefinedElements& UDataTablePredefs = OutMembers[DataTable.GetIndex()];
	UDataTablePredefs.Members =
	{
		PredefinedMember {
			.Comment = "So, here's a RowMap. Good luck with it.",
			.Type = "TMap<class FName, uint8*>", .Name = "RowMap", .Offset = Off::InSDK::UDataTable::RowMap, .Size = sizeof(TMap<int, int>), .ArrayDim = 0x1, .Alignment = alignof(TMap<int, int>),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	PredefinedElements& UObjectPredefs = OutMembers[ObjectArray::FindClassFast("Object").GetIndex()];
	UObjectPredefs.Members =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "void*", .Name = "VTable", .Offset = Off::UObject::Vft, .Size = sizeof(void**), .ArrayDim = 0x1, .Alignment = alignof(void**),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "EObjectFlags", .Name = "Flags", .Offset = Off::UObject::Flags, .Size = sizeof(EObjectFlags), .ArrayDim = 0x1, .Alignment = alignof(EObjectFlags),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "Index", .Offset = Off::UObject::Index, .Size = sizeof(int32), .ArrayDim = 0x1, .Alignment = alignof(int32),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UClass*", .Name = "Class", .Offset = Off::UObject::Class, .Size = sizeof(void*), .ArrayDim = 0x1, .Alignment = alignof(void*),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class FName", .Name = "Name", .Offset = Off::UObject::Name, .Size = Off::InSDK::Name::FNameSize, .ArrayDim = 0x1, .Alignment = alignof(int32),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class UObject*", .Name = "Outer", .Offset = Off::UObject::Outer, .Size = sizeof(void*), .ArrayDim = 0x1, .Alignment = alignof(void*),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	const UEClass UField = ObjectArray::FindClassFast("Field");
	PredefinedElements& UFieldPredefs = OutMembers[UField.GetIndex()];

	// Starting from UE5.7 UField::Next is reflected and doesn't need to be added manually anymore
	if (!UField.FindMember("Next", EClassCastFlags::ObjectProperty))
	{
		UFieldPredefs.Members.insert(UFieldPredefs.Members.begin(),
			PredefinedMember{
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "class UField*", .Name = "Next", .Offset = Off::UField::Next, .Size = sizeof(void*), .ArrayDim = 0x1, .Alignment = alignof(void*),
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			}
		);
	}

	PredefinedElements& UEnumPredefs = OutMembers[ObjectArray::FindClassFast("Enum").GetIndex()];
	UEnumPredefs.Members =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class TArray<class TPair<class FName, int64>>", .Name = "Names", .Offset = Off::UEnum::Names, .Size = sizeof(TArray<int>), .ArrayDim = 0x1, .Alignment = alignof(TArray<int>),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	UEClass UStruct = ObjectArray::FindClassFast("Struct");

	if (UStruct == nullptr)
		UStruct = ObjectArray::FindClassFast("struct");

	PredefinedElements& UStructPredefs = OutMembers[UStruct.GetIndex()];
	UStructPredefs.Members =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int16", .Name = "MinAlignment", .Offset = Off::UStruct::MinAlignment, .Size = sizeof(int16), .ArrayDim = 0x1, .Alignment = alignof(int16),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "int32", .Name = "Size", .Offset = Off::UStruct::Size, .Size = sizeof(int32), .ArrayDim = 0x1, .Alignment = alignof(int32),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	// Starting from UE5.7 UStruct::SuperStruct is reflected and doesn't need to be added manually anymore
	if (!UStruct.FindMember("SuperStruct", EClassCastFlags::ObjectProperty))
	{
		UStructPredefs.Members.insert(UStructPredefs.Members.begin(),
			PredefinedMember{
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "class UStruct*", .Name = "SuperStruct", .Offset = Off::UStruct::SuperStruct, .Size = sizeof(void*), .ArrayDim = 0x1, .Alignment = alignof(void*),
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			}
		);
	}

	// Starting from UE5.7 UStruct::Children is reflected and doesn't need to be added manually anymore
	if (!UStruct.FindMember("Children", EClassCastFlags::ObjectProperty))
	{
		UStructPredefs.Members.insert(UStructPredefs.Members.begin(),
			PredefinedMember{
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "class UField*", .Name = "Children", .Offset = Off::UStruct::Children, .Size = sizeof(void*), .ArrayDim = 0x1, .Alignment = alignof(void*),
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			}
		);
	}

	if (Settings::Internal::bUseFProperty)
	{
		UStructPredefs.Members.push_back({
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "class FField*", .Name = "ChildProperties", .Offset = Off::UStruct::ChildProperties, .Size = sizeof(void*), .ArrayDim = 0x1, .Alignment = alignof(void*),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			}
		);
	}

	if (Off::UStruct::StructBaseChain != -1)
	{
		UStructPredefs.Members.push_back({
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "struct FStructBaseChain", .Name = "BaseChain", .Offset = Off::UStruct::StructBaseChain, .Size = sizeof(void*) + sizeof(int32) + sizeof(uint32) /* PAD */, .ArrayDim = 0x1, .Alignment = alignof(void*),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			});
	}

	PredefinedElements& UFunctionPredefs = OutMembers[ObjectArray::FindClassFast("Function").GetIndex()];
	UFunctionPredefs.Members =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "EFunctionFlags", .Name = "FunctionFlags", .Offset = Off::UFunction::FunctionFlags, .Size = sizeof(EFunctionFlags), .ArrayDim = 0x1, .Alignment = alignof(EFunctionFlags),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "void*", .Name = "ExecFunction", .Offset = Off::UFunction::ExecFunction, .Size = sizeof(void*), .ArrayDim = 0x1, .Alignment = alignof(void*),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	const UEClass UClass = ObjectArray::FindClassFast("Class");
	PredefinedElements& UClassPredefs = OutMembers[UClass.GetIndex()];
	UClassPredefs.Members =
	{
		PredefinedMember {
			.Comment = "NOT AUTO-GENERATED PROPERTY",
			.Type = "enum class EClassCastFlags", .Name = "CastFlags", .Offset = Off::UClass::CastFlags, .Size = sizeof(EClassCastFlags), .ArrayDim = 0x1, .Alignment = alignof(EClassCastFlags),
			.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
		},
	};

	// Starting from UE5.7 UClass::ClassDefaultObject is reflected and doesn't need to be added manually anymore
	if (!UClass.FindMember("ClassDefaultObject", EClassCastFlags::ObjectProperty))
	{
		UClassPredefs.Members.insert(UClassPredefs.Members.begin(),
			PredefinedMember{
				.Comment = "NOT AUTO-GENERATED PROPERTY",
				.Type = "class UObject*", .Name = "ClassDefaultObject", .Offset = Off::UClass::ClassDefaultObject, .Size = sizeof(void*), .ArrayDim = 0x1, .Alignment = alignof(void*),
				.bIsStatic = false, .bIsZeroSizeMember = false, .bIsBitField = false, .BitIndex = 0xFF
			}
		);
	}
}
