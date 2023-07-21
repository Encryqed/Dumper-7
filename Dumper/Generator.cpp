#include "Generator.h"

#include <future>

Generator::FunctionsMap Generator::PredefinedFunctions;
Generator::MemberMap Generator::PredefinedMembers;

std::mutex Generator::PackageMutex;
std::vector<std::future<void>> Generator::Futures;


void Generator::Init()
{
	/* manual override */
	//ObjectArray::Init(/*GObjects*/, /*ChunkSize*/, /*bIsChunked*/);
	//FName::Init(/*FName::AppendString*/);
	//Off::InSDK::InitPE(/*PEIndex*/);

	//ObjectArray::DecryptPtr = [](void* Objptr) -> uint8_t* { return (uint8_t*)(uint64_t(Objptr) ^ 0x8375); };

	ObjectArray::Init();

	//Demo
	//FName::Init(0x2411BB0, 0x6EA4F00);

	//Retail
	//Sig: 48 8D 0D ? ? ? ? E8 ? ? ? ? 4C 8B C0 C6 05 ? ? ? ? ? 48 8B 45 ? 48 8D 55 ? 48 C1 E8 ? 8D 0C 00 49 03 4C F8 ? E8 ? ? ? ? 8B 7D ? 85 FF 74 ? 8B 4D ? 8D 59 ? 85 C9 7F ? 41 8B DC
	//        AppendString, FNamePool
	FName::Init(0x25E1DB0, 0x729ABC0);

	Off::Init();
	Off::InSDK::InitPE(); //Must be last, relies on offsets initialized in Off::Init()

	InitPredefinedMembers();
	InitPredefinedFunctions();
}

void Generator::GenerateMappings()
{
	fs::path DumperFolder;
	fs::path GenFolder;

	try
	{
		DumperFolder = Settings::SDKGenerationPath;
		GenFolder = DumperFolder / (Settings::GameVersion + '-' + Settings::GameName + "_MAPPINGS");

		if (!fs::exists(DumperFolder))
		{
			fs::create_directories(DumperFolder);
		}

		if (fs::exists(GenFolder))
		{
			fs::path Old = GenFolder.generic_string() + "_OLD";

			fs::remove_all(Old);

			fs::rename(GenFolder, Old);
		}

		fs::create_directory(GenFolder);
	}
	catch (const std::filesystem::filesystem_error& fe)
	{
		std::cout << "Could not create required folders! Info: \n";
		std::cout << fe.what() << std::endl;
		return;
	}


	struct EnumInfo
	{
		std::vector<int32> MemberNames;
		int32 EnumNameIdx;

		EnumInfo(std::vector<int32>&& Names, int32 Idx)
			: MemberNames(std::move(Names)), EnumNameIdx(Idx)
		{
		}
	};

	struct StructInfo
	{
		int32 NameIdx;
		int32 SuperNameIdx;

		UEStruct SelfStruct;

		StructInfo(int32 Name, int32 SuperName, UEStruct Self)
			: NameIdx(Name), SuperNameIdx(SuperName), SelfStruct(Self)
		{
		}
	};

	MappingFileWriter<std::ofstream> MappingsStream(GenFolder / (Settings::GameVersion + ".usmap"));
	MappingFileWriter<std::stringstream> Buffer;

	std::unordered_map<int32, int32> NameIdxPairs;
	std::vector<EnumInfo> Enums;
	std::vector<StructInfo> Structs;

	int NameIndex = 0;

	auto WriteNameToBuffer = [&Buffer](FName Name)
	{
		std::string StrName = Name.ToString();

		StrName = StrName.substr(StrName.find_last_of(':') + 1);

		if (StrName.length() > 0xFF)
		{
			Buffer.Write<uint8>(0xFF);
			Buffer.WriteStr(StrName.substr(0, 0xFF));
		}
		else
		{
			Buffer.Write<uint8>(StrName.length());
			Buffer.WriteStr(StrName);
		}
	};

	for (auto Obj : ObjectArray())
	{
		if (Obj.IsA(EClassCastFlags::Struct))
		{
			UEStruct AsStruct = Obj.Cast<UEStruct>();

			auto [StructNameIt, bInsertedStructName] = NameIdxPairs.insert({ Obj.GetFName().GetCompIdx(), NameIndex });

			if (bInsertedStructName)
			{
				WriteNameToBuffer(Obj.GetFName());
				NameIndex++;
			}

			int32 SuperIdx = AsStruct.GetSuper() ? NameIdxPairs[AsStruct.GetSuper().GetFName().GetCompIdx()] : 0xFFFFFFFF;
			Structs.emplace_back(StructNameIt->second, SuperIdx, AsStruct);

			for (UEProperty Prop : Obj.Cast<UEStruct>().GetProperties())
			{
				auto [It, bInsertedPropName] = NameIdxPairs.insert({ Prop.GetFName().GetCompIdx(), NameIndex });

				if (bInsertedPropName)
				{
					WriteNameToBuffer(Prop.GetFName());
					NameIndex++;
				}
			}
		}
		else if (Obj.IsA(EClassCastFlags::Enum))
		{
			auto [EnumNameIt, bInsertedEnumname] = NameIdxPairs.insert({ Obj.GetFName().GetCompIdx(), NameIndex });

			if (bInsertedEnumname)
			{
				WriteNameToBuffer(Obj.GetFName());
				NameIndex++;
			}

			std::vector<int32> MemberNames;

			for (auto& [Name, Value] : Obj.Cast<UEEnum>().GetNameValuePairs())
			{
				auto [MemberNameIt, bInsertedMemberName] = NameIdxPairs.insert({ Name.GetCompIdx(), NameIndex });

				if (bInsertedMemberName)
				{
					WriteNameToBuffer(Name);
					NameIndex++;
				}

				MemberNames.push_back(MemberNameIt->second); //Index
			}

			Enums.emplace_back(std::move(MemberNames), EnumNameIt->second);
		}
	}

	auto WriteProperty = [&NameIdxPairs, &Buffer](UEProperty Property, auto&& WriteProperty) -> void
	{
		if (Property.IsA(EClassCastFlags::ByteProperty) && Property.Cast<UEByteProperty>().GetEnum()) 
			Buffer.Write<uint8>((uint8)EMappingsTypeFlags::EnumProperty);
		
		Buffer.Write<uint8>((uint8)Property.GetMappingType());

		if (Property.IsA(EClassCastFlags::EnumProperty))
		{
			UEEnumProperty EnumProperty = Property.Cast<UEEnumProperty>();

			WriteProperty(EnumProperty.GetUnderlayingProperty(), WriteProperty);

			auto Enum = EnumProperty.GetEnum();
			Buffer.Write<int32>(NameIdxPairs[Enum ? Enum.GetFName().GetCompIdx() : 0]);
		}
		else if (Property.IsA(EClassCastFlags::ByteProperty) && Property.Cast<UEByteProperty>().GetEnum())
		{
			Buffer.Write<int32>(NameIdxPairs[Property.Cast<UEByteProperty>().GetEnum().GetFName().GetCompIdx()]);
		}
		else if (Property.IsA(EClassCastFlags::StructProperty))
		{
			Buffer.Write<int32>(NameIdxPairs[Property.Cast<UEStructProperty>().GetUnderlayingStruct().GetFName().GetCompIdx()]);
		}
		else if (Property.IsA(EClassCastFlags::ArrayProperty))
		{
			WriteProperty(Property.Cast<UEArrayProperty>().GetInnerProperty(), WriteProperty);
		}
		else if (Property.IsA(EClassCastFlags::SetProperty))
		{
			WriteProperty(Property.Cast<UESetProperty>().GetElementProperty(), WriteProperty);
		}
		else if (Property.IsA(EClassCastFlags::MapProperty))
		{
			WriteProperty(Property.Cast<UEMapProperty>().GetKeyProperty(), WriteProperty);
			WriteProperty(Property.Cast<UEMapProperty>().GetValueProperty(), WriteProperty);
		}
	};

	Buffer.Write<uint32>(Enums.size());
	for (auto& Enum : Enums)
	{
		Buffer.Write<int32>(Enum.EnumNameIdx);
		Buffer.Write<uint8>(Enum.MemberNames.size());

		for (int32 NameIdx : Enum.MemberNames)
		{
			Buffer.Write<int32>(NameIdx);
		}
	}

	Buffer.Write<uint32>(Structs.size());
	for (auto& Struct : Structs)
	{
		Buffer.Write<int32>(Struct.NameIdx);
		Buffer.Write<int32>(Struct.SuperNameIdx);

		uint16 PropertyCount = 0;
		uint16 SerializableCount = 0;

		auto Properties = Struct.SelfStruct.GetProperties();

		for (auto Property : Properties)
		{
			PropertyCount += Property.GetArrayDim();
			SerializableCount++;
		}

		Buffer.Write<uint16>(PropertyCount);
		Buffer.Write<uint16>(SerializableCount);

		SerializableCount = 0; // recycling for indices

		for (auto Property : Properties)
		{
			Buffer.Write<uint16>(SerializableCount); // Index
			Buffer.Write<uint8>(Property.GetArrayDim());
			Buffer.Write<int32>(NameIdxPairs[Property.GetFName().GetCompIdx()]);

			WriteProperty(Property, WriteProperty);

			SerializableCount++;
		}
	}

	uint32 DataSize = Buffer.GetSize() + 0xF;

	// HEADER
	MappingsStream.Write<uint16>(0x30C4); //MAGIC
	MappingsStream.Write<uint8>(0); // EUsmapVersion::Initial
	//only when versioning >= 1
	//MappingsStream.Write<int32>(false); // EUsmapVersion::PackageVersioning yes, someone decided int == bool in CUE4 Parser

	//Versioning info here

	MappingsStream.Write<uint8>(0); //CompressionMethode::None
	MappingsStream.Write<uint32>(DataSize); // CompressedSize
	MappingsStream.Write<uint32>(DataSize); // DecompressedSize
	
	MappingsStream.Write<uint32>(NameIndex); // NameCount
	
	//move data to other stream
	MappingsStream.CopyFromOtherBuffer(Buffer);
}

void Generator::HandlePackageGeneration(const fs::path* const SDKFolder, int32 PackageIndex, std::vector<int32>* MemberIndices)
{
	UEObject Object = ObjectArray::GetByIndex(PackageIndex);

	if (!Object)
		return;

	Package Pack(Object);

	PackageMutex.lock();
	Package::AddPackage(PackageIndex);
	Pack.GatherDependencies(*MemberIndices);
	PackageMutex.unlock();

	Pack.Process(*MemberIndices);

	if (!Pack.IsEmpty())
	{
		std::string PackageName = Object.GetName();
		std::string FileName = Settings::FilePrefix ? Settings::FilePrefix + PackageName : PackageName;

		if (fs::exists(*SDKFolder / (FileName + "_classes.hpp")))
			FileName += "_1";

		FileWriter ClassFile(*SDKFolder, FileName, FileWriter::FileType::Class);
		ClassFile.WriteClasses(Pack.AllClasses);
		ClassFile.Close();

		FileWriter StructsFile(*SDKFolder, FileName, FileWriter::FileType::Struct);
		StructsFile.WriteEnums(Pack.AllEnums);
		StructsFile.WriteStructs(Pack.AllStructs);
		ClassFile.Close();

		FileWriter FunctionFile(*SDKFolder, FileName, FileWriter::FileType::Function);
		FileWriter ParameterFile(*SDKFolder, FileName, FileWriter::FileType::Parameter);

		if (PackageName == "CoreUObject")
			FunctionFile.Write("\t//Initialize GObjects using InitGObjects()\n\tTUObjectArray* UObject::GObjects = nullptr;\n\n");

		for (auto& [ClassName, PackageFunctionsPairs] : Generator::PredefinedFunctions)
		{
			if (PackageFunctionsPairs.first != PackageName)
				continue;

			for (auto& PredefFunc : PackageFunctionsPairs.second)
			{
				if (!PredefFunc.DeclarationCPP.empty())
				{
					FunctionFile.Write(PredefFunc.DeclarationCPP);
					FunctionFile.Write(PredefFunc.Body);
					FunctionFile.Write("\n");
				}
			}
		}

		for (auto& Function : Pack.AllFunctions)
		{
			FunctionFile.WriteFunction(Function);
			ParameterFile.WriteParamStruct(Function.GetParamStruct());
		}
	}
	else
	{
		PackageMutex.lock();
		Package::PackageSorterClasses.RemoveDependant(PackageIndex);
		Package::PackageSorterStructs.RemoveDependant(PackageIndex);
		PackageMutex.unlock();
	}
}

void Generator::GenerateSDK()
{
	std::unordered_map<int32_t, std::vector<int32_t>> ObjectPackages;

	ObjectArray::GetAllPackages(ObjectPackages);

	fs::path DumperFolder;
	fs::path GenFolder;
	fs::path SDKFolder;

	try
	{
		DumperFolder = Settings::SDKGenerationPath;
		GenFolder = DumperFolder / (Settings::GameVersion + '-' + Settings::GameName);
		SDKFolder = GenFolder / "SDK";

		if (!fs::exists(DumperFolder))
		{
			fs::create_directories(DumperFolder);
		}

		if (fs::exists(GenFolder))
		{
			fs::path Old = GenFolder.generic_string() + "_OLD";

			fs::remove_all(Old);

			fs::rename(GenFolder, Old);
		}

		fs::create_directory(GenFolder);
		fs::create_directory(SDKFolder);
	}
	catch (const std::filesystem::filesystem_error& fe)
	{
		std::cout << "Could not create required folders! Info: \n"; 
		std::cout << fe.what() << std::endl;
		return;
	}

	ObjectArray::DumpObjects();

	Package::InitAssertionStream(GenFolder);


	// Determine main-package of the game
	int32 IndexOfBiggestPackage = 0;
	int32 SizeOfBiggestPackage = 0;

	for (const auto& [PackageIdx, DependencyVector] : ObjectPackages)
	{
		if (DependencyVector.size() > SizeOfBiggestPackage)
		{
			SizeOfBiggestPackage = DependencyVector.size();
			IndexOfBiggestPackage = PackageIdx;
		}
	}

	_setmaxstdio(0x400); // set number of files which can be open simultaneously

	Futures.reserve(ObjectPackages.size());

	for (auto& [PackageIndex, MemberIndices] : ObjectPackages)
	{
		Futures.push_back(std::async(std::launch::async, HandlePackageGeneration, &SDKFolder, PackageIndex, &MemberIndices));
	}

	for (auto& Future : Futures)
	{
		Future.wait();
	}

	Futures.clear();

	Package::CloseAssertionStream();

	GenerateSDKHeader(GenFolder, IndexOfBiggestPackage);
	GenerateFixupFile(GenFolder);
	GenerateBasicFile(SDKFolder);

	std::cout << "\n\n[=] Done [=]\n\n";
}

void Generator::GenerateSDKHeader(const fs::path& SdkPath, int32 BiggestPackageIdx)
{
	std::ofstream HeaderStream(SdkPath / "SDK.hpp");

	HeaderStream << "#pragma once\n\n";
	HeaderStream << "// Made with <3 by me [Encryqed] && you [Fischsalat]\n\n";

	HeaderStream << std::format("// {}\n", Settings::GameName);
	HeaderStream << std::format("// {}\n\n", Settings::GameVersion);
	HeaderStream << std::format("// Main-package: {}\n\n", ObjectArray::GetByIndex(BiggestPackageIdx).GetValidName());

	HeaderStream << "#define WINDOWS_IGNORE_PACKING_MISMATCH\n\n";

	HeaderStream << "#include <string>\n";
	HeaderStream << "#include <Windows.h>\n";
	HeaderStream << "#include <iostream>\n";
	HeaderStream << "#include <type_traits>\n\n";

	HeaderStream << "typedef __int8 int8;\n";
	HeaderStream << "typedef __int16 int16;\n";
	HeaderStream << "typedef __int32 int32;\n";
	HeaderStream << "typedef __int64 int64;\n\n";

	HeaderStream << "typedef unsigned __int8 uint8;\n";
	HeaderStream << "typedef unsigned __int16 uint16;\n";
	HeaderStream << "typedef unsigned __int32 uint32;\n";
	HeaderStream << "typedef unsigned __int64 uint64;\n";

	HeaderStream << std::format(
		R"(
namespace Offsets
{{
	inline int32 GObjects          = 0x{:08X};
	inline int32 AppendString      = 0x{:08X};
	inline int32 ProcessEvent      = 0x{:08X};
}}
)", Off::InSDK::GObjects, Off::InSDK::AppendNameToString, Off::InSDK::PEOffset);

	if (Settings::bShouldXorStrings)
		HeaderStream << "#define XORSTR(str) str\n";

	HeaderStream << "\n#include \"PropertyFixup.hpp\"\n";
	HeaderStream << "\n#include \"SDK/" << (Settings::FilePrefix ? Settings::FilePrefix : "") << "Basic.hpp\"\n";

	if (Settings::bIncludeOnlyRelevantPackages)
	{
		std::string IncludesString;

		Package::PackageSorterStructs.GetIncludesForPackage(BiggestPackageIdx, EIncludeFileType::Struct, IncludesString, false);
		HeaderStream << IncludesString;

		IncludesString.clear();

		Package::PackageSorterClasses.GetIncludesForPackage(BiggestPackageIdx, EIncludeFileType::Class, IncludesString, false, &Package::PackageSorterStructs, EIncludeFileType::Struct);
		HeaderStream << IncludesString;
		
		IncludesString.clear();
		
		Package::PackageSorterParams.GetIncludesForPackage(BiggestPackageIdx, EIncludeFileType::Params, IncludesString, false);
		HeaderStream << IncludesString;
	}

	for (auto& Pack : Package::PackageSorterStructs.AllDependencies)
	{
		std::string IncludesString;
		Package::PackageSorterStructs.GetIncludesForPackage(Pack.first, EIncludeFileType::Struct, IncludesString, Settings::bIncludeOnlyRelevantPackages);

		HeaderStream << IncludesString;
	}

	for (auto& Pack : Package::PackageSorterClasses.AllDependencies)
	{
		std::string IncludesString;
		Package::PackageSorterClasses.GetIncludesForPackage(Pack.first, EIncludeFileType::Class, IncludesString, Settings::bIncludeOnlyRelevantPackages, &Package::PackageSorterStructs, EIncludeFileType::Struct);
	
		HeaderStream << IncludesString;
	}
	
	for (auto& Pack : Package::PackageSorterParams.AllDependencies)
	{
		std::string IncludesString;
		Package::PackageSorterParams.GetIncludesForPackage(Pack.first, EIncludeFileType::Params, IncludesString, Settings::bIncludeOnlyRelevantPackages);
	
		HeaderStream << IncludesString;
	}

	HeaderStream.close();
}

void Generator::GenerateFixupFile(const fs::path& SdkPath)
{
	std::ofstream FixupStream(SdkPath / "PropertyFixup.hpp");

	FixupStream << "#pragma once\n\n";

	FixupStream << "// Definitions for missing Properties\n\n";

	for (auto& Property : UEProperty::UnknownProperties)
	{
		FixupStream << std::format("class {}\n{{\n\tunsigned __int8 Pad[0x{:X}];\n}};\n\n", Property.first, Property.second);
	}
}

void Generator::InitPredefinedMembers()
{
	auto PrefixPropertyName = [](std::string&& Name) -> std::string
	{
		return (Settings::Internal::bUseFProperty ? 'F' : 'U') + std::move(Name);
	};
	
	PredefinedMembers["UObject"] =
	{
		{ "static class TUObjectArray*", "GObjects", 0x00, 0x00},
		{ "void*", "Vft", Off::UObject::Vft, 0x08 },
		{ "int32 ", "Flags", Off::UObject::Flags, 0x04 },
		{ "int32", "Index", Off::UObject::Index, 0x04 },
		{ "class UClass*", "Class", Off::UObject::Class, 0x08 },
		{ "class FName", "Name", Off::UObject::Name, Off::InSDK::FNameSize },
		{ "class UObject*", "Outer", Off::UObject::Outer, 0x08 }
	};

	PredefinedMembers["UField"] =
	{
		{ "class UField*", "Next", Off::UField::Next, 0x08 }
	};

	PredefinedMembers["UEnum"] =
	{
		{ "class TArray<class TPair<class FName, int64>>", "Names", Off::UEnum::Names, 0x10 }
	};

	PredefinedMembers["UStruct"] =
	{
		{ "class UStruct*", "Super", Off::UStruct::SuperStruct, 0x08 },
		{ "class UField*", "Children", Off::UStruct::Children, 0x08 },
		{ "int32", "Size", Off::UStruct::Size, 0x04 }
	};

	if (Settings::Internal::bUseFProperty)
		PredefinedMembers["UStruct"].emplace(PredefinedMembers["UStruct"].begin() + 2, "class FField*", "ChildProperties", Off::UStruct::ChildProperties, 0x08);

	PredefinedMembers["UFunction"] =
	{
		{ "uint32", "FunctionFlags", Off::UFunction::FunctionFlags, 0x08 }
	};

	PredefinedMembers["UClass"] =
	{
		{ "enum class EClassCastFlags", "CastFlags", Off::UClass::CastFlags, 0x08 },
		{ "class UObject*", "DefaultObject", Off::UClass::ClassDefaultObject, 0x08 }
	};


	PredefinedMembers[PrefixPropertyName("Property")] =
	{
		{ "int32", "ElementSize", Off::UProperty::ElementSize, 0x04 },
		{ "uint64", "PropertyFlags", Off::UProperty::PropertyFlags, 0x08 },
		{ "int32", "Offset", Off::UProperty::Offset_Internal, 0x04 }
	};

	PredefinedMembers[PrefixPropertyName("ByteProperty")] =
	{
		{ "class UEnum*", "Enum", Off::UByteProperty::Enum, 0x08 }
	};

	PredefinedMembers[PrefixPropertyName("BoolProperty")] =
	{
		{ "uint8", "FieldSize", Off::UBoolProperty::Base, 0x01 },
		{ "uint8", "ByteOffset", Off::UBoolProperty::Base + 0x1, 0x01 },
		{ "uint8", "ByteMask", Off::UBoolProperty::Base + 0x2, 0x01 },
		{ "uint8", "FieldMask", Off::UBoolProperty::Base + 0x3, 0x01 }
	};

	PredefinedMembers[PrefixPropertyName("ObjectProperty")] =
	{
		{ "class UClass*", "PropertyClass", Off::UObjectProperty::PropertyClass, 0x08 }
	};

	PredefinedMembers[PrefixPropertyName("ClassProperty")] =
	{
		{ "class UClass*", "MetaClass", Off::UClassProperty::MetaClass, 0x08 }
	};

	PredefinedMembers[PrefixPropertyName("StructProperty")] =
	{
		{ "class UStruct*", "Struct", Off::UStructProperty::Struct, 0x08 }
	};

	std::string PrefixedPropertyPtr = std::format("class {}*", PrefixPropertyName("Property"));

	PredefinedMembers[PrefixPropertyName("ArrayProperty")] =
	{
		{ PrefixedPropertyPtr, "InnerProperty", Off::UArrayProperty::Inner, 0x08}
	};

	PredefinedMembers[PrefixPropertyName("MapProperty")] =
	{
		{ PrefixedPropertyPtr, "KeyProperty", Off::UMapProperty::Base, 0x08 },
		{ PrefixedPropertyPtr, "ValueProperty", Off::UMapProperty::Base + 0x08, 0x08 }
	};

	PredefinedMembers[PrefixPropertyName("SetProperty")] =
	{
		{ PrefixedPropertyPtr, "ElementProperty", Off::USetProperty::ElementProp, 0x08 }
	};

	PredefinedMembers[PrefixPropertyName("EnumProperty")] =
	{
		{ PrefixedPropertyPtr, "UnderlayingProperty", Off::UEnumProperty::Base, 0x08 },
		{ "class UEnum*", "Enum", Off::UEnumProperty::Base + 0x08, 0x08 }
	};

	if (!Settings::Internal::bUseFProperty)
		return;

	int FNameSize = (Settings::Internal::bUseCasePreservingName ? 0x10 : 0x8);
	int FFieldVariantSize = (!Settings::Internal::bUseMaskForFieldOwner ? 0x10 : 0x8);
	std::string UObjectIdentifierType = (Settings::Internal::bUseMaskForFieldOwner ? "static constexpr uint64" : "bool");
	std::string UObjectIdentifierName = (Settings::Internal::bUseMaskForFieldOwner ? "UObjectMask = 0x1" : "bIsUObject");

	PredefinedMembers["FFieldClass"] =
	{
		{ "FName", "Name", Off::FFieldClass::Name, FNameSize },
		{ "uint64", "Id", Off::FFieldClass::Id, 0x8 },
		{ "uint64", "CastFlags", Off::FFieldClass::CastFlags, 0x8 },
		{ "EClassFlags", "ClassFlags", Off::FFieldClass::ClassFlags, 0x4 },
		{ "FFieldClass*", "SuperClass", Off::FFieldClass::SuperClass, 0x8 },
	};

	PredefinedMembers["FFieldVariant"] =
	{
		{ "union { class FField* Field; class UObject* Object; }", "Container", 0x0, 0x8 },
		{ UObjectIdentifierType, UObjectIdentifierName, 0x0, !Settings::Internal::bUseMaskForFieldOwner }
	};

	PredefinedMembers["FField"] =
	{
		{ "void*", "Vft", Off::FField::Vft, 0x8 },
		{ "FFieldClass*", "Class", Off::FField::Class, 0x8 },
		{ "FFieldVariant", "Owner", Off::FField::Owner, FFieldVariantSize },
		{ "FField*", "Next", Off::FField::Next, 0x8 },
		{ "FName", "Name", Off::FField::Name, FNameSize },
		{ "int32", "Flags", Off::FField::Flags, 0x4 }
	};
}

void Generator::InitPredefinedFunctions()
{
	PredefinedFunctions["UObject"] =
	{
		"CoreUObject",
		{
			{
				"\tbool HasTypeFlag(EClassCastFlags TypeFlag) const;",
				"\tbool UObject::HasTypeFlag(EClassCastFlags TypeFlag) const",
R"(
	{
		return TypeFlag != EClassCastFlags::None ? Class->CastFlags & TypeFlag : true;
	}
)"
			},
			{
				"\tbool IsDefaultObject() const", "",
R"(
	{
		return (Flags & 0x10) == 0x10;
	}
)"
			},
			{
				"\tstd::string GetName() const;",
				"\tstd::string UObject::GetName() const",
R"(
	{
		return this ? Name.ToString() : "None";
	}
)"
			},
			{
				"\tstd::string GetFullName() const;",
				"\tstd::string UObject::GetFullName() const",
R"(
	{
		if (Class)
		{
			std::string Temp;

			for (UObject* NextOuter = Outer; NextOuter; NextOuter = NextOuter->Outer)
			{
				Temp = NextOuter->GetName() + "." + Temp;
			}

			std::string Name = Class->GetName();
			Name += " ";
			Name += Temp;
			Name += GetName();

			return Name;
		}

		return "None";
	}
)"
			},
			{
				"\ttemplate<typename UEType = UObject>\n\tstatic UEType* FindObject(const std::string& FullName, EClassCastFlags RequiredType = EClassCastFlags::None)", "",
R"(
	{
		for (int i = 0; i < GObjects->Num(); ++i)
		{
			UObject* Object = GObjects->GetByIndex(i);
	
			if (!Object)
				continue;
			
			if (Object->HasTypeFlag(RequiredType) && Object->GetFullName() == FullName)
			{
				return static_cast<UEType*>(Object);
			}
		}

		return nullptr;
	}
)"
			},
			{
				"\ttemplate<typename UEType = UObject>\n\tstatic UEType* FindObjectFast(const std::string& Name, EClassCastFlags RequiredType = EClassCastFlags::None)", "",
R"(
	{
		for (int i = 0; i < GObjects->Num(); ++i)
		{
			UObject* Object = GObjects->GetByIndex(i);
	
			if (!Object)
				continue;
			
			if (Object->HasTypeFlag(RequiredType) && Object->GetName() == Name)
			{
				return static_cast<UEType*>(Object);
			}
		}

		return nullptr;
	}
)"
			},
			{
				"\tstatic class UClass* FindClass(const std::string& ClassFullName)", "",
R"(
	{
		return FindObject<class UClass>(ClassFullName, EClassCastFlags::Class);
	}
)"
			},
			{
				"\tstatic class UClass* FindClassFast(const std::string& ClassName)", "",
R"(
	{
		return FindObjectFast<class UClass>(ClassName, EClassCastFlags::Class);
	}
)"
			},
			{
				"\tbool IsA(class UClass* Clss) const;",
				"\tbool UObject::IsA(class UClass* Clss) const",
R"(
	{
		for (UStruct* Super = Class; Super; Super = Super->Super)
		{
			if (Super == Clss)
			{
				return true;
			}
		}

		return false;
	}
)"
			},
			{
				"\tinline void ProcessEvent(class UFunction* Function, void* Parms) const", "",
				std::format(
R"(
	{{
		return GetVFunction<void(*)(const UObject*, class UFunction*, void*)>(this, 0x{:X} /*0x{:X}*/)(this, Function, Parms);
	}}
)", Off::InSDK::PEIndex, Off::InSDK::PEOffset)
			}
		}
	};

	PredefinedFunctions["UClass"] =
	{
		"CoreUObject",
		{
			{
				"\tclass UFunction* GetFunction(const std::string& ClassName, const std::string& FuncName);",
				"\tclass UFunction* UClass::GetFunction(const std::string& ClassName, const std::string& FuncName)",
R"(
	{
		for(UStruct* Clss = this; Clss; Clss = Clss->Super)
		{
			if (Clss->GetName() == ClassName)
			{
				for (UField* Field = Clss->Children; Field; Field = Field->Next)
				{
					if(Field->HasTypeFlag(EClassCastFlags::Function) && Field->GetName() == FuncName)
					{
						return static_cast<class UFunction*>(Field);
					}	
				}
			}
		}
		return nullptr;
	}
)"
			}
		}
	};

	PredefinedFunctions["FGuid"] =
	{
		"CoreUObject",
		{
			{
				"\tinline bool operator==(const FGuid& Other) const",
				"",
R"(
	{
		return A == Other.A && B == Other.B && C == Other.C && D == Other.D;
	})"
			},
			{
				"\tinline bool operator!=(const FGuid& Other) const",
				"",
R"(
	{
		return A != Other.A || B != Other.B || C != Other.C || D != Other.D;
	}
)"
			}
		}
	};

	PredefinedFunctions["FVector"] =
	{
		"CoreUObject",
		{
			{
				"\tinline bool operator==(const FVector& Other) const",
				"",
R"(
	{
		return X == Other.X && Y == Other.Y && Z == Other.Z;
	})"
			},
			{
				"\tinline bool operator!=(const FVector& Other) const",
				"",
R"(
	{
		return X != Other.X || Y != Other.Y || Z != Other.Z;
	}
)"
			}
		}
	};
	
}

void Generator::GenerateBasicFile(const fs::path& SdkPath)
{
	FileWriter BasicHeader(SdkPath, Settings::FilePrefix ? Settings::FilePrefix + std::string("Basic") : "Basic", FileWriter::FileType::Header);
	FileWriter BasicSource(SdkPath, Settings::FilePrefix ? Settings::FilePrefix + std::string("Basic") : "Basic", FileWriter::FileType::Source);

	BasicHeader.Write(
		R"(
void InitGObjects();
)");

	BasicSource.Write(
		R"(
void InitGObjects()
{
	UObject::GObjects = reinterpret_cast<TUObjectArray*>(uintptr_t(GetModuleHandle(0)) + Offsets::GObjects);
}		
)");

	BasicHeader.Write(
		R"(
template<typename Fn>
inline Fn GetVFunction(const void* Instance, std::size_t Index)
{
	auto Vtable = *reinterpret_cast<const void***>(const_cast<void*>(Instance));
	return reinterpret_cast<Fn>(Vtable[Index]);
}
)");

	if (Off::InSDK::ChunkSize <= 0)
	{
		BasicHeader.Write(
			std::format(R"(
class TUObjectArray
{{
public:

	struct FUObjectItem
	{{
		class UObject* Object;
		uint8 Pad[0x{:02X}];
	}};

	FUObjectItem* Objects;
	int32 MaxElements;
	int32 NumElements;

	// Call InitGObjects() before using these functions

	inline int Num() const
	{{
		return NumElements;
	}}

	inline class UObject* GetByIndex(const int32 Index) const
	{{
		if (Index < 0 || Index > NumElements)
			return nullptr;

		return Objects[Index].Object;
	}}
}};
)", Off::InSDK::FUObjectItemSize - 0x8));
	}
	else
	{
		BasicHeader.Write(
			std::format(R"(
class TUObjectArray
{{
public:

	enum
	{{
		ElementsPerChunk = 0x{:X},
	}};

	struct FUObjectItem
	{{
		class UObject* Object;
		uint8 Pad[0x{:02X}];
	}};

	FUObjectItem** Objects;
	uint8 Pad_0[0x08];
	int32 MaxElements;
	int32 NumElements;
	int32 MaxChunks;
	int32 NumChunks;

	// Call InitGObjects() before using these functions

	inline int32 Num() const
	{{
		return NumElements;
	}}

	inline class UObject* GetByIndex(const int32 Index) const
	{{
		if (Index < 0 || Index > NumElements)
			return nullptr;

		const int32 ChunkIndex = Index / ElementsPerChunk;
		const int32 InChunkIdx = Index % ElementsPerChunk;

		return Objects[ChunkIndex][InChunkIdx].Object;
	}}
}};
)", Off::InSDK::ChunkSize, Off::InSDK::FUObjectItemSize - 0x8));
	}

	BasicHeader.Write(
		R"(
template<class T>
class TArray
{
protected:
	T* Data;
	int32 NumElements;
	int32 MaxElements;

public:

	TArray() = default;

	inline TArray(int32 Size)
		:NumElements(0), MaxElements(Size), Data(reinterpret_cast<T*>(malloc(sizeof(T) * Size)))
	{
	}

	inline T& operator[](uint32 Index)
	{
		return Data[Index];
	}
	inline const T& operator[](uint32 Index) const
	{
		return Data[Index];
	}

	inline int32 Num()
	{
		return NumElements;
	}

	inline int32 Max()
	{
		return MaxElements;
	}

	inline int32 GetSlack()
	{
		return MaxElements - NumElements;
	}

	inline bool IsValid()
	{
		return Data != nullptr;
	}

	inline bool IsValidIndex(int32 Index)
	{
		return Index >= 0 && Index < NumElements;
	}

	inline void ResetNum()
	{
		NumElements = 0;
	}
};
)");

	BasicHeader.Write(
		R"(
class FString : public TArray<wchar_t>
{
public:
	inline FString() = default;

	using TArray::TArray;

	inline FString(const wchar_t* WChar)
	{
		MaxElements = NumElements = *WChar ? std::wcslen(WChar) + 1 : 0;

		if (NumElements)
		{
			Data = const_cast<wchar_t*>(WChar);
		}
	}

	inline FString operator=(const wchar_t*&& Other)
	{
		return FString(Other);
	}

	inline std::wstring ToWString()
	{
		if (IsValid())
		{
			return Data;
		}

		return L"";
	}

	inline std::string ToString()
	{
		if (IsValid())
		{
			std::wstring WData(Data);
			return std::string(WData.begin(), WData.end());
		}

		return "";
	}
};
)");

	BasicHeader.Write(
		R"(
class FName
{
public:)");

	if (Off::InSDK::FNameSize == 0x8)
	{
		BasicHeader.Write(
			R"(
	int32 ComparisonIndex;
	int32 Number;
)");
	}
	else
	{
		BasicHeader.Write(
			R"(
	int32 ComparisonIndex;
	int32 Number;
	int32 DisplayIndex;
	uint8 Pad[0x4];
)");
	}

	BasicHeader.Write(
		std::format(R"(
	
	inline std::string ToString() const
	{{
		static FString TempString(1024);
		static auto AppendString = reinterpret_cast<void(*)(const FName*, FString&)>(uintptr_t(GetModuleHandle(0)) + Offsets::AppendString);

		AppendString(this, TempString);

		std::string OutputString = TempString.ToString();
		TempString.ResetNum();

		size_t pos = OutputString.rfind('/');

		if (pos == std::string::npos)
			return OutputString;

		return OutputString.substr(pos + 1);
	}}

	inline bool operator==(const FName& Other)
	{{
		return ComparisonIndex == Other.ComparisonIndex;
	}}

	inline bool operator!=(const FName& Other)
	{{
		return ComparisonIndex != Other.ComparisonIndex;
	}}
}};
)", Off::InSDK::AppendNameToString));

	BasicHeader.Write(
		R"(
template<typename ClassType>
class TSubclassOf
{
	class UClass* ClassPtr;

public:
	TSubclassOf() = default;

	inline TSubclassOf(UClass* Class)
		: ClassPtr(Class)
	{
	}

	inline UClass* Get()
	{
		return ClassPtr;
	}
	inline UClass* operator->()
	{
		return ClassPtr;
	}
	inline TSubclassOf& operator=(UClass* Class)
	{
		ClassPtr = Class;

		return *this;
	}

	inline bool operator==(const TSubclassOf& Other) const
	{
		return ClassPtr == Other.ClassPtr;
	}
	inline bool operator!=(const TSubclassOf& Other) const
	{
		return ClassPtr != Other.ClassPtr;
	}
	inline bool operator==(UClass* Other) const
	{
		return ClassPtr == Other;
	}
	inline bool operator!=(UClass* Other) const
	{
		return ClassPtr != Other;
	}
};
)");

	BasicHeader.Write(
		R"(
template<typename ValueType, typename KeyType>
class TPair
{
public:
	ValueType First;
	KeyType Second;
};
)");

	BasicHeader.Write(
		R"(
class FText
{
public:
	FString TextData;
	uint8 IdkTheRest[0x8];
};
)");

	BasicHeader.Write(
		R"(
template<typename ElementType>
class TSet
{
	uint8 WaitTillIImplementIt[0x50];
};
)");

	BasicHeader.Write(
		R"(
template<typename KeyType, typename ValueType>
class TMap
{
	uint8 WaitTillIImplementIt[0x50];
};
)");

	BasicHeader.Write(
		R"(
class FWeakObjectPtr
{
protected:
	int32		ObjectIndex;
	int32		ObjectSerialNumber;

public:
	class UObject* Get() const;

	class UObject* operator->() const;

	bool operator==(const FWeakObjectPtr& Other) const;
	bool operator!=(const FWeakObjectPtr& Other) const;

	bool operator==(const class UObject* Other) const;
	bool operator!=(const class UObject* Other) const;
};
)");

	BasicHeader.Write(
		R"(
template<typename UEType>
class TWeakObjectPtr : FWeakObjectPtr
{
public:

	UEType* Get() const
	{
		return static_cast<UEType*>(FWeakObjectPtr::Get());
	}

	UEType* operator->() const
	{
		return static_cast<UEType*>(FWeakObjectPtr::Get());
	}
};
)");

	BasicHeader.Write(
		R"(

struct FUniqueObjectGuid
{
	uint32 A;
	uint32 B;
	uint32 C;
	uint32 D;
};
)");

	BasicHeader.Write(
		R"(
template<typename TObjectID>
class TPersistentObjectPtr
{
public:
	FWeakObjectPtr WeakPtr;
	int32 TagAtLastTest;
	TObjectID ObjectID;

	class UObject* Get() const
	{
		return WeakPtr.Get();
	}
	class UObject* operator->() const
	{
		return WeakPtr.Get();
	}
};
)");

	BasicHeader.Write(
		R"(
template<typename UEType>
class TLazyObjectPtr : public TPersistentObjectPtr<FUniqueObjectGuid>
{
public:

	UEType* Get() const
	{
		return static_cast<UEType*>(TPersistentObjectPtr::Get());
	}

	UEType* operator->() const
	{
		return static_cast<UEType*>(TPersistentObjectPtr::Get());
	}
};
)");

	BasicHeader.Write(
		R"(
class FSoftObjectPath_
{
public:
	FName AssetPathName;
	FString SubPathString;
};
)");

	BasicHeader.Write(
		R"(
class alignas(8) FSoftObjectPtr : public TPersistentObjectPtr<FSoftObjectPath_>
{
public:

	FName GetAssetPathName();
	FString GetSubPathString();

	std::string GetAssetPathNameStr();
	std::string GetSubPathStringStr();
};
)");


	BasicHeader.Write(
		R"(
template<typename UEType>
class TSoftObjectPtr : public FSoftObjectPtr
{
public:

	UEType* Get() const
	{
		return static_cast<UEType*>(TPersistentObjectPtr::Get());
	}

	UEType* operator->() const
	{
		return static_cast<UEType*>(TPersistentObjectPtr::Get());
	}
};
)");

	BasicHeader.Write(
		R"(
template<typename UEType>
class TSoftClassPtr : public FSoftObjectPtr
{
public:

	UEType* Get() const
	{
		return static_cast<UEType*>(TPersistentObjectPtr::Get());
	}

	UEType* operator->() const
	{
		return static_cast<UEType*>(TPersistentObjectPtr::Get());
	}
};
)");

	BasicSource.Write(
		R"(
FName FSoftObjectPtr::GetAssetPathName()
{
	return ObjectID.AssetPathName;
}
FString FSoftObjectPtr::GetSubPathString()
{
	return ObjectID.SubPathString;
}

std::string FSoftObjectPtr::GetAssetPathNameStr()
{
	return ObjectID.AssetPathName.ToString();
}
std::string FSoftObjectPtr::GetSubPathStringStr()
{
	return ObjectID.SubPathString.ToString();
}
)");


	BasicSource.Write(
		R"(
class UObject* FWeakObjectPtr::Get() const
{
	return UObject::GObjects->GetByIndex(ObjectIndex);
}

class UObject* FWeakObjectPtr::operator->() const
{
	return UObject::GObjects->GetByIndex(ObjectIndex);
}

bool FWeakObjectPtr::operator==(const FWeakObjectPtr& Other) const
{
	return ObjectIndex == Other.ObjectIndex;
}
bool FWeakObjectPtr::operator!=(const FWeakObjectPtr& Other) const
{
	return ObjectIndex != Other.ObjectIndex;
}

bool FWeakObjectPtr::operator==(const class UObject* Other) const
{
	return ObjectIndex == Other->Index;
}
bool FWeakObjectPtr::operator!=(const class UObject* Other) const
{
	return ObjectIndex != Other->Index;
}
)");

	BasicHeader.Write(
		R"(

enum class EClassCastFlags : uint64_t
{
	None = 0x0000000000000000,

	Field							= 0x0000000000000001,
	Int8Property					= 0x0000000000000002,
	Enum							= 0x0000000000000004,
	Struct							= 0x0000000000000008,
	ScriptStruct					= 0x0000000000000010,
	Class							= 0x0000000000000020,
	ByteProperty					= 0x0000000000000040,
	IntProperty						= 0x0000000000000080,
	FloatProperty					= 0x0000000000000100,
	UInt64Property					= 0x0000000000000200,
	ClassProperty					= 0x0000000000000400,
	UInt32Property					= 0x0000000000000800,
	InterfaceProperty				= 0x0000000000001000,
	NameProperty					= 0x0000000000002000,
	StrProperty						= 0x0000000000004000,
	Property						= 0x0000000000008000,
	ObjectProperty					= 0x0000000000010000,
	BoolProperty					= 0x0000000000020000,
	UInt16Property					= 0x0000000000040000,
	Function						= 0x0000000000080000,
	StructProperty					= 0x0000000000100000,
	ArrayProperty					= 0x0000000000200000,
	Int64Property					= 0x0000000000400000,
	DelegateProperty				= 0x0000000000800000,
	NumericProperty					= 0x0000000001000000,
	MulticastDelegateProperty		= 0x0000000002000000,
	ObjectPropertyBase				= 0x0000000004000000,
	WeakObjectProperty				= 0x0000000008000000,
	LazyObjectProperty				= 0x0000000010000000,
	SoftObjectProperty				= 0x0000000020000000,
	TextProperty					= 0x0000000040000000,
	Int16Property					= 0x0000000080000000,
	DoubleProperty					= 0x0000000100000000,
	SoftClassProperty				= 0x0000000200000000,
	Package							= 0x0000000400000000,
	Level							= 0x0000000800000000,
	Actor							= 0x0000001000000000,
	PlayerController				= 0x0000002000000000,
	Pawn							= 0x0000004000000000,
	SceneComponent					= 0x0000008000000000,
	PrimitiveComponent				= 0x0000010000000000,
	SkinnedMeshComponent			= 0x0000020000000000,
	SkeletalMeshComponent			= 0x0000040000000000,
	Blueprint						= 0x0000080000000000,
	DelegateFunction				= 0x0000100000000000,
	StaticMeshComponent				= 0x0000200000000000,
	MapProperty						= 0x0000400000000000,
	SetProperty						= 0x0000800000000000,
	EnumProperty					= 0x0001000000000000,
};
)");

	BasicHeader.Write(
		R"(
inline constexpr EClassCastFlags operator|(EClassCastFlags Left, EClassCastFlags Right)
{				
	using CastFlagsType = std::underlying_type<EClassCastFlags>::type;
	return static_cast<EClassCastFlags>(static_cast<CastFlagsType>(Left) | static_cast<CastFlagsType>(Right));
}

inline bool operator&(EClassCastFlags Left, EClassCastFlags Right)
{
	using CastFlagsType = std::underlying_type<EClassCastFlags>::type;
	return (static_cast<CastFlagsType>(Left) & static_cast<CastFlagsType>(Right)) == static_cast<CastFlagsType>(Right);
}
)");


	BasicHeader.Write(
		R"(

enum EClassFlags
{
	CLASS_None					= 0x00000000u,
	Abstract					= 0x00000001u,
	DefaultConfig				= 0x00000002u,
	Config						= 0x00000004u,
	Transient					= 0x00000008u,
	Parsed						= 0x00000010u,
	MatchedSerializers			= 0x00000020u,
	ProjectUserConfig			= 0x00000040u,
	Native						= 0x00000080u,
	NoExport					= 0x00000100u,
	NotPlaceable				= 0x00000200u,
	PerObjectConfig				= 0x00000400u,
	ReplicationDataIsSetUp		= 0x00000800u,
	EditInlineNew				= 0x00001000u,
	CollapseCategories			= 0x00002000u,
	Interface					= 0x00004000u,
	CustomConstructor			= 0x00008000u,
	Const						= 0x00010000u,
	LayoutChanging				= 0x00020000u,
	CompiledFromBlueprint		= 0x00040000u,
	MinimalAPI					= 0x00080000u,
	RequiredAPI					= 0x00100000u,
	DefaultToInstanced			= 0x00200000u,
	TokenStreamAssembled		= 0x00400000u,
	HasInstancedReference		= 0x00800000u,
	Hidden						= 0x01000000u,
	Deprecated					= 0x02000000u,
	HideDropDown				= 0x04000000u,
	GlobalUserConfig			= 0x08000000u,
	Intrinsic					= 0x10000000u,
	Constructed					= 0x20000000u,
	ConfigDoNotCheckDefaults	= 0x40000000u,
	NewerVersionExists			= 0x80000000u,
};
)");

	BasicHeader.Write(
		R"(
inline constexpr EClassFlags operator|(EClassFlags Left, EClassFlags Right)
{
	using ClassFlagsType = std::underlying_type<EClassFlags>::type;
	return static_cast<EClassFlags>(static_cast<ClassFlagsType>(Left) | static_cast<ClassFlagsType>(Right));
}

inline bool operator&(EClassFlags Left, EClassFlags Right)
{
	using ClassFlagsType = std::underlying_type<EClassFlags>::type;
	return ((static_cast<ClassFlagsType>(Left) & static_cast<ClassFlagsType>(Right)) == static_cast<ClassFlagsType>(Right));
}
)");

	BasicHeader.Write(
		R"(
class FScriptInterface
{
public:
	UObject* ObjectPointer = nullptr;
	void* InterfacePointer = nullptr;

	inline UObject* GetObjectRef()
	{
		return ObjectPointer;
	}
};

template<class InterfaceType>
class TScriptInterface : public FScriptInterface
{
public:
};


)");

	if (!Settings::Internal::bUseFProperty)
		return;

	// const std::string& SuperName, Types::Struct& Struct, int32 StructSize, int32 SuperSize

	struct FPropertyPredef
	{
		const char* ClassName;
		const char* SuperName;
	};

	std::unordered_map<std::string, int32> ClassSizePairs;

	constexpr std::array<FPropertyPredef, 0xD> FPropertyClassSuperPairs =
	{{
		{ "FFieldClass", "" },
		{ "FFieldVariant", "" },
		{ "FField", "" },
		{ "FProperty", "FField" },
		{ "FByteProperty", "FProperty" },
		{ "FBoolProperty", "FProperty" },
		{ "FObjectProperty", "FProperty" },
		{ "FClassProperty", "FObjectProperty" },
		{ "FStructProperty", "FProperty" },
		{ "FArrayProperty", "FProperty" },
		{ "FMapProperty", "FProperty" },
		{ "FSetProperty", "FProperty" },
		{ "FEnumProperty", "FProperty" }
	}};
	//bool Package::GeneratePredefinedMembers(const std::string& SuperName, Types::Struct& Struct, int32 StructSize, int32 SuperSize)

	ClassSizePairs.reserve(FPropertyClassSuperPairs.size());
	for (auto& [ClassName, SuperName] : FPropertyClassSuperPairs)
	{
		int32 SuperSize = 0;

		if (SuperName && strcmp(SuperName, "") != 0)
			SuperSize = ClassSizePairs[SuperName];

		Types::Struct NewStruct(ClassName, true, SuperName);
		ClassSizePairs[ClassName] = Package::GeneratePredefinedMembers(ClassName, NewStruct, 0, SuperSize); // fix supersize

		BasicHeader.Write(NewStruct.GetGeneratedBody());
	}


	//-TUObjectArray
	//-TArray
	//-FString
	//-FName [AppendString()]
	//-TSubclassOf<Type>
	// TSet<Type>
	// TMap<Type>
	//-FText
}

//class FWeakObjectPtr
//{
//protected:
//	int32		ObjectIndex;
//	int32		ObjectSerialNumber;
//
//public:
//	class UObject* Get() const;
//
//	class UObject* operator->() const;
//
//	bool operator==(const FWeakObjectPtr& Other) const;
//	bool operator!=(const FWeakObjectPtr& Other) const;
//
//	bool operator==(const class UObject* Other) const;
//	bool operator!=(const class UObject* Other) const;
//};
//
//template<typename UEType>
//struct TWeakObjectPtr : FWeakObjectPtr
//{
//public:
//	class UEType* Get() const;
//
//	class UEType* operator->() const;
//};
//
//class UObject* FWeakObjectPtr::Get() const
//{
//	return UObject::GObjects->GetByIndex(ObjectIndex);
//}
//
//class UObject* FWeakObjectPtr::operator->() const
//{
//	return UObject::GObjects->GetByIndex(ObjectIndex);
//}
//
//bool FWeakObjectPtr::operator==(const FWeakObjectPtr& Other) const
//{
//	return ObjectIndex == Other.ObjectIndex;
//}
//bool FWeakObjectPtr::operator!=(const FWeakObjectPtr& Other) const
//{
//	return ObjectIndex != Other.ObjectIndex;
//}
//
//bool FWeakObjectPtr::operator==(const class UObject* Other) const
//{
//	return ObjectIndex == Obj->Index;
//}
//bool FWeakObjectPtr::operator!=(const class UObject* Other) const
//{
//	return ObjectIndex != Obj->Index;
//}
//
//
//class UEType* TWeakObjectPtr<UEType>::Get() const
//{
//	return UObject::GObjects->GetByIndex(ObjectIndex);
//}
//
//class UEType* TWeakObjectPtr<UEType>::operator->() const
//{
//	return UObject::GObjects->GetByIndex(ObjectIndex);
//}


//class TUObjectArray
//{
//	struct FUObjectItem
//	{
//		class UObject* Object;
//		uint8 Pad[0x10];
//	};
//
//	FUObjectItem* Objects;
//	int32_t MaxElements;
//	int32_t NumElements;
//
//public:
//	inline int Num() const
//	{
//		return NumElements;
//	}
//
//	inline class UObject* GetByIndex(const int32 Index) const
//	{
//		if (Index < 0 || Index > NumElements)
//			return nullptr;
//
//		return Objects[Index].Object;
//	}
//};
//
//class TUObjectArray
//{
//	enum
//	{
//		ElementsPerChunk = 0x{:X},
//	};
//
//	struct FUObjectItem
//	{
//		class UObject* Object;
//		uint8 Pad[0x10];
//	};
//
//	FUObjectItem** Objects;
//	uint8_t Pad_0[0x08];
//	int32_t MaxElements;
//	int32_t NumElements;
//	int32_t MaxChunks;
//	int32_t NumChunks;
//
//	inline int Num() const
//	{
//		return NumElements;
//	}
//
//	inline class UObject* GetByIndex(const int32 Index) const
//	{
//		if (Index < 0 || Index > NumElements)
//			return nullptr;
//
//		const int32 ChunkIndex = Index / ElementsPerChunk;
//		const int32 InChunkIdx = Index % ElementsPerChunk;
//
//		return Objects[ChunkIndex][InChunkIdx].Object;
//	}
//};

struct FText
{
	FString TextData;
	uint8 IdkTheRest[0x8];
};

template<typename ClassType>
class TSubclassOf
{
	class UClass* ClassPtr;

public:
	inline UClass* Get()
	{
		return ClassPtr;
	}
	inline UClass* operator->()
	{
		return ClassPtr;
	}
	inline TSubclassOf& operator=(UClass* Class)
	{
		ClassPtr = Class;

		return *this;
	}

	inline bool operator==(const TSubclassOf& Other) const
	{
		return ClassPtr == Other.ClassPtr;
	}
	inline bool operator!=(const TSubclassOf& Other) const
	{
		return ClassPtr != Other.ClassPtr;
	}
	inline bool operator==(UClass* Other) const
	{
		return ClassPtr == Other;
	}
	inline bool operator!=(UClass* Other) const
	{
		return ClassPtr != Other;
	}
};


//template<class T>
//class TArray
//{
//protected:
//	T* Data;
//	int32 NumElements;
//	int32 MaxElements;
//
//public:
//
//	TArray() = default;
//
//	inline TArray(int32 Size)
//		:NumElements(0), MaxElements(Size), Data(reinterpret_cast<T*>(malloc(sizeof(T) * Size)))
//	{
//	}
//
//	inline T& operator[](uint32 Index)
//	{
//		return Data[Index];
//	}
//	inline const T& operator[](uint32 Index) const
//	{
//		return Data[Index];
//	}
//
//	inline int32 Num()
//	{
//		return NumElements;
//	}
//
//	inline int32 Max()
//	{
//		return MaxElements;
//	}
//
//	inline int32 GetSlack()
//	{
//		return MaxElements - NumElements;
//	}
//
//	inline bool IsValid()
//	{
//		return Data != nullptr;
//	}
//
//	inline bool IsValidIndex(int32 Index)
//	{
//		return Index >= 0 && Index < NumElements;
//	}
//
//	inline bool IsValidIndex(uint32 Index)
//	{
//		return Index < NumElements;
//	}
//
//	inline void ResetNum()
//	{
//		NumElements = 0;
//	}
//};

//class FString : public TArray<wchar_t>
//{
//public:
//	inline FString() = default;
//
//	using TArray::TArray;
//
//	inline FString(const wchar_t* WChar)
//	{
//		MaxElements = NumElements = *WChar ? std::wcslen(WChar) + 1 : 0;
//
//		if (NumElements)
//		{
//			Data = const_cast<wchar_t*>(WChar);
//		}
//	}
//
//	inline FString operator=(const wchar_t*&& Other)
//	{
//		return FString(Other);
//	}
//
//	inline std::wstring ToWString()
//	{
//		if (IsValid())
//		{
//			return Data;
//		}
//
//		return L"";
//	}
//
//	inline std::string ToString()
//	{
//		if (IsValid())
//		{
//			std::wstring WData(Data);
//			return std::string(WData.begin(), WData.end());
//		}
//
//		return "";
//	}
//};

//class FName
//{
//public:
//	int32 ComparisonIndex;
//	int32 Number;
//
//	inline std::string ToString()
//	{
//		static FString TempString(1024);
//		static auto AppendString = reinterpret_cast<void(*)(FName*, FString&)>(uintptr_t(GetModuleHandle(0)) + OFFSET));
//
//		AppendString(this, TempString);
//
//		std::string OutputString = TempString.ToString();
//		TempString.ResetNum();
//
//		size_t pos = OutputString.rfind('/');
//
//		if (pos == std::string::npos)
//			return OutputString;
//
//		return OutputString.substr(pos + 1);
//	}
//
//	inline bool operator==(const FName& Other)
//	{
//		return ComparisonIndex == Other.ComparisonIndex;
//	}
//
//	inline bool operator!=(const FName& Other)
//	{
//		return ComparisonIndex != Other.ComparisonIndex;
//	}
//};