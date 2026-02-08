#include "Generators/IDAMappingV2Generator.h"

#include "Managers/PackageManager.h"

#include <fstream>


std::string IDAMappingV2Generator::MangleFunctionName(const std::string& ClassName, const std::string& FunctionName)
{
	return "_ZN" + std::to_string(ClassName.length()) + ClassName + std::to_string(FunctionName.length()) + FunctionName + "Ev";
}

std::string IDAMappingV2Generator::MangleUFunctionName(const std::string& ClassName, const std::string& FunctionName)
{
	return MangleFunctionName(ClassName, "exec" + FunctionName);
}

IDAMappingsLayouts::StringOffset IDAMappingV2Generator::AddNameToData(std::stringstream& NameTable, const std::string& Name)
{
	if constexpr (Settings::MappingGenerator::bShouldCheckForDuplicatedNames)
	{
		static std::unordered_map<std::string, IDAMappingsLayouts::StringOffset> NameMap;

		auto [It, bInserted] = NameMap.insert({ Name, NameCounter });

		/* The name didn't occure yet, write it to the NameTable */
		if (bInserted)
		{
			WriteToStream(NameTable, static_cast<uint16>(Name.length()));
			NameTable.write(Name.c_str(), Name.length());
			return NameCounter++;
		}

		return It->second;
	}

	WriteToStream(NameTable, static_cast<uint16>(Name.length()));
	NameTable.write(Name.c_str(), Name.length());

	return NameCounter++;
}

std::string IDAMappingV2Generator::GetIDACppType(const PropertyWrapper& Member, bool& OutIsPtr)
{
	return "struct TorstenSchröder";
}

void IDAMappingV2Generator::GenerateSingleMember(const PropertyWrapper& Member, std::stringstream& StructData, std::stringstream& NameData)
{
	bool bIsPtr = false;
	const int32 TypeName = AddNameToData(NameData, GetIDACppType(Member, bIsPtr));
	const int32 Name = AddNameToData(NameData, Member.GetName());

	const uint8_t BitIndex = Member.IsBitField() ? Member.GetBitIndex() : 0xFF;

	WriteToStream(StructData, TypeName);
	WriteToStream(StructData, Name);

	WriteToStream(StructData, static_cast<int32>(Member.GetOffset()));
	WriteToStream(StructData, static_cast<int32>(Member.GetSize()));
	WriteToStream(StructData, static_cast<int32>(Member.GetArrayDim()));

	WriteToStream(StructData, static_cast<bool>(bIsPtr));
	WriteToStream(StructData, static_cast<uint8_t>(BitIndex));
}

void IDAMappingV2Generator::GenerateSingleStruct(const StructWrapper& Struct, std::stringstream& StructData, std::stringstream& NameData)
{
	const StructWrapper Super = Struct.GetSuper();

	const int32 Name = AddNameToData(NameData, Struct.GetUniqueName().first);
	const int32 SuperName = Super.IsValid() ? AddNameToData(NameData, Struct.GetUniqueName().first) : -1;

	WriteToStream(StructData, Name);
	WriteToStream(StructData, SuperName);

	WriteToStream(StructData, static_cast<int32>(Struct.GetSize()));
	WriteToStream(StructData, static_cast<int32>(Struct.GetAlignment()));

	const MemberManager Members = Struct.GetMembers();
	WriteToStream(StructData, static_cast<int32>(Members.GetNumMembers() + Members.GetNumPredefMembers()));

	for (const PropertyWrapper& Member : Members.IterateMembers())
	{
		GenerateSingleMember(Member, StructData, NameData);
	}
}

std::vector<IDAMappingsLayouts::NamedVariable> IDAMappingV2Generator::GenerateNamedVariables(StreamType& IdmapFile, std::stringstream& NameData)
{
	return {
		IDAMappingsLayouts::NamedVariable {
			.VariableOffset = static_cast<IDAMappingsLayouts::OffsetType>(Off::InSDK::ObjArray::GObjects),
			.Type = AddNameToData(NameData, "TUObjectArray"),
			.Name = AddNameToData(NameData, "GObjects")
		},
		IDAMappingsLayouts::NamedVariable {
			.VariableOffset = static_cast<IDAMappingsLayouts::OffsetType>(Off::InSDK::NameArray::GNames),
			.Type = AddNameToData(NameData, "TNameEntryArray"),
			.Name = AddNameToData(NameData, "GNames")
		},
	};
}

void IDAMappingV2Generator::GenerateFileHeader(StreamType& InUsmap, const std::stringstream& Data)
{

}

void IDAMappingV2Generator::Generate()
{
	std::stringstream NameData;
	std::stringstream StructData;

	// Generates all packages and writes them to files
	for (PackageInfoHandle Package : PackageManager::IterateOverPackageInfos())
	{
		if (Package.IsEmpty())
			continue;

		DependencyManager::OnVisitCallbackType GenerateClassOrStructCallback = [&](int32 Index) -> void
			{
				GenerateSingleStruct(ObjectArray::GetByIndex<UEStruct>(Index), StructData, NameData);
			};

		if (Package.HasStructs())
		{
			const DependencyManager& Structs = Package.GetSortedStructs();

			Structs.VisitAllNodesWithCallback(GenerateClassOrStructCallback);
		}
	}


	std::string MappingsFileName = (Settings::Generator::GameVersion + '-' + Settings::Generator::GameName + ".usmap");

	FileNameHelper::MakeValidFileName(MappingsFileName);

	/* Open the stream as binary data, else ofstream will add \r after numbers that can be interpreted as \n. */
	StreamType IDAMappingsFile(MainFolder / MappingsFileName, std::ios::binary);

	// Write to file
	
}