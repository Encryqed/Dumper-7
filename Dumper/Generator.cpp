#include "Generator.h"


Generator::FunctionsMap Generator::PredefinedFunctions;
Generator::MemberMap Generator::PredefinedMembers;

Generator::Generator()
{
	ObjectArray::Init();
	FName::Init();
	Off::Init();

	InitPredefinedMembers();
	InitPredefinedFunctions();
}

void Generator::GenerateSDK()
{
	Generator SDKGen;

	auto ObjectPackages = ObjectArray::GetAllPackages();

	std::cout << "Started Generation [Dumper-7]!\n";
	std::cout << "Total Packages: " << ObjectPackages.size() << "\n\n";

	fs::path GenFolder(Settings::SDKGenerationPath);
	fs::path SDKFolder = GenFolder / "SDK";

	if (fs::exists(GenFolder))
		std::cout << "Removed: " << fs::remove_all(GenFolder) << " Files!\n";
	
	fs::create_directory(GenFolder);
	fs::create_directory(SDKFolder);

	for (auto& Pair : ObjectPackages)
	{
		UEObject Object = ObjectArray::GetByIndex(Pair.first);

		Package Pack(Object);
		Pack.Process(Pair.second);

		if (!Pack.IsEmpty())
		{
			bool bPackageHasPredefinedFunctions = false;

			std::string PackageName = Object.GetName();

			FileWriter ClassFile(SDKFolder, PackageName, FileWriter::FileType::Class);
			FileWriter StructsFile(SDKFolder, PackageName, FileWriter::FileType::Struct);
			FileWriter FunctionFile(SDKFolder, PackageName, FileWriter::FileType::Function);
			FileWriter ParameterFile(SDKFolder, PackageName, FileWriter::FileType::Parameter);

			ClassFile.WriteClasses(Pack.AllClasses);
			StructsFile.WriteStructs(Pack.AllStructs);
			//FunctionFile.WriteFunctions(Pack.AllFunctions);

			for (auto& Function : Pack.AllFunctions)
			{
				for (auto& Pairs : Generator::PredefinedFunctions)
				{
					if (Pairs.second.first == PackageName)
					{
						for (auto& PredefFunc : Pairs.second.second)
						{
							if (PredefFunc.DeclarationCPP != "")
							{
								FunctionFile.Write(PredefFunc.DeclarationCPP);
								FunctionFile.Write(PredefFunc.Body);
								FunctionFile.Write("\n");
							}
						}
					}
				}

				FunctionFile.WriteFunction(Function);
				ParameterFile.WriteStruct(Function.GetParamStruct());
			}
		}
		else
		{
			ObjectPackages.erase(Pair.first);
		}
	}

	SDKGen.GenerateSDKHeader(GenFolder, ObjectPackages);
	SDKGen.GenerateFixupFile(GenFolder);

	std::cout << "\n\n[=] Done [=]\n\n";
}

void Generator::GenerateSDKHeader(fs::path& SdkPath, std::unordered_map<int32_t, std::vector<int32_t>>& Packages)
{
	std::ofstream HeaderStream(SdkPath / "SDK.hpp");

	HeaderStream << "#pragma once\n\n";
	HeaderStream << "// Made with <3 by Encryqed && me\n";
	HeaderStream << std::format("// {} SDK\n\n", Settings::GameName);

	HeaderStream << "#include <string>\n";
	HeaderStream << "#include <Windows.h>\n";
	HeaderStream << "#include <iostream>\n\n";

	HeaderStream << "typedef __int8 int8;\n";
	HeaderStream << "typedef __int16 int16;\n";
	HeaderStream << "typedef __int32 int32;\n";
	HeaderStream << "typedef __int64 int64;\n\n";
	
	HeaderStream << "typedef unsigned __int8 uint8;\n";
	HeaderStream << "typedef unsigned __int16 uint16;\n";
	HeaderStream << "typedef unsigned __int32 uint32;\n";
	HeaderStream << "typedef unsigned __int64 uint64;\n\n\n";
	
	HeaderStream << "#include \"PropertyFixup.hpp\"\n\n";
	HeaderStream << "#include \"SDK/Basic.hpp\"\n";

	for (auto& Pair : Packages)
	{
		std::string PackageName = ObjectArray::GetByIndex(Pair.first).GetName();

		HeaderStream << std::format("#include \"SDK/{}_structs.hpp\"\n", PackageName);
		HeaderStream << std::format("#include \"SDK/{}_classes.hpp\"\n", PackageName);
		HeaderStream << std::format("#include \"SDK/{}_parameters.hpp\"\n", PackageName);
	}
}
void Generator::GenerateFixupFile(fs::path& SdkPath)
{
	std::ofstream FixupStream(SdkPath / "PropertyFixup.hpp");

	FixupStream << "#pragma once\n\n";

	FixupStream << "//Definitions for missing Properties\n\n";

	for (auto& Property : UEProperty::UnknownProperties)
	{
		FixupStream << std::format("class {}\n{{\n\tunsigned __int8 Pad[0x{:X}];\n}}\n\n", Property.first, Property.second);
	}
}

void Generator::InitPredefinedMembers()
{
	PredefinedMembers["UObject"] =
	{
		{ "static class TUObjectArray*", "GObjects", 0x00, 0x00 },
		{ "void*", "Vft", Off::UObject::Vft, 0x08 },
		{ "int32 ", "Flags", Off::UObject::Flags, 0x04 },
		{ "int32", "Index", Off::UObject::Index, 0x04 },
		{ "class UClass*", "Class", Off::UObject::Class, 0x08 },
		{ "struct FName", "Name", Off::UObject::Name, 0x08 },
		{ "class UObject*", "Outer", Off::UObject::Outer, 0x08 }
	};

	PredefinedMembers["UField"] =
	{
		{ "class UField*", "Next", Off::UField::Next, 0x08 }
	};

	PredefinedMembers["UEnum"] =
	{
		{ "class TArray<class TPair<struct FName, int64>>", "Names", Off::UEnum::Names, 0x0C }
	};

	PredefinedMembers["UStruct"] =
	{
		{ "class UStruct*", "Super", Off::UStruct::SuperStruct, 0x08 },
		{ "class UField*", "Children", Off::UStruct::Children, 0x08 },
		{ "int32", "Size", Off::UStruct::Size, 0x04 }
	};

	PredefinedMembers["UFunction"] =
	{
		{ "uint32", "FunctionFlags", Off::UFunction::FunctionFlags, 0x08 }
	};

	PredefinedMembers["UClass"] =
	{
		{ "enum class EClassCastFlags", "CastFlags", Off::UClass::ClassFlags, 0x08 },
		{ "class UObject*", "DefaultObject", Off::UClass::ClassDefaultObject, 0x08 }
	};

	PredefinedMembers["UProperty"] =
	{
		{ "int32", "ElementSize", Off::UProperty::ElementSize, 0x04 },
		{ "uint64", "PropertyFlags", Off::UProperty::PropertyFlags, 0x08 },
		{ "int32", "Offset", Off::UProperty::Offset_Internal, 0x04 }
	};

	PredefinedMembers["UByteProperty"] =
	{
		{ "class UEnum*", "Enum", Off::UByteProperty::Enum, 0x08 }
	};

	PredefinedMembers["UBoolProperty"] =
	{
		{ "uint8", "FieldSize", Off::UBoolProperty::Base, 0x01 },
		{ "uint8", "ByteOffset", Off::UBoolProperty::Base + 0x1, 0x01 },
		{ "uint8", "ByteMask", Off::UBoolProperty::Base + 0x2, 0x01 },
		{ "uint8", "FieldMask", Off::UBoolProperty::Base + 0x3, 0x01 }
	};

	PredefinedMembers["UObjectProperty"] =
	{
		{ "class UClass*", "PropertyClass", Off::UObjectProperty::PropertyClass, 0x08 }
	};

	PredefinedMembers["UClassProperty"] =
	{
		{ "class UClass*", "MetaClass", Off::UClassProperty::MetaClass, 0x08 }
	};

	PredefinedMembers["UStructProperty"] =
	{
		{ "class UStruct*", "Struct", Off::UStructProperty::Struct, 0x08 }
	};

	PredefinedMembers["UArrayProperty"] =
	{
		{ "class UProperty*", "InnerProperty", Off::UArrayProperty::Inner, 0x08 }
	};

	PredefinedMembers["UMapProperty"] =
	{
		{ "class UProperty*", "KeyProperty", Off::UMapProperty::Base, 0x08 },
		{ "class UProperty*", "ValueProperty", Off::UMapProperty::Base + 0x08, 0x08 }
	};

	PredefinedMembers["USetProperty"] =
	{
		{ "class UProperty*", "ElementProperty", Off::USetProperty::ElementProp, 0x08 }
	};

	PredefinedMembers["USetProperty"] =
	{
		{ "class UProperty*", "UnderlayingProperty", Off::UEnumProperty::Base, 0x08 },
		{ "class UEnum*", "Enum", Off::UEnumProperty::Base + 0x08, 0x08 }
	};
}

void Generator::InitPredefinedFunctions()
{
	PredefinedFunctions["UObject"] =
	{
		"CoreUObject",
		{
			{
				"\tbool HasTypeFlag(EClassCastFlags TypeFlag) const", "",
R"(
	{
		return TypeFlag != EClassCastFlags::None ? Class->CastFlags & TypeFlag : true;
	}
)"
			},
			{
				"\tstd::string GetName() const;",
				"\tstd::string UObject::GetName() const",
R"(
	{
		return this ? ToString() : "None";
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

			for (UObject NextOuter = Outer; NextOuter; NextOuter = Outer.Outer)
			{
				Temp = NextOuter.GetName() + "." + Temp;
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
				return static_cast<T*>(object);
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
				return static_cast<T*>(object);
			}
		}

		return nullptr;
	}
)"
			},
			{
				"\tclass UClass* FindClass(const std::string& ClassFullName)", "",
R"(
	{
		return FindObject<class UClass>(ClassName, EClassCastFlags::UClass);
	}
)"
			},
			{
				"\tclass UClass* FindClassFast(const std::string& ClassName)", "",
R"(
	{
		return FindObjectFast<class UClass>(ClassName, EClassCastFlags::UClass);
	}
)"
			},
			{
				"\tbool IsA(class UClass* Clss) const;",
				"\tbool UObject::IsA(class UClass* Clss) const",
R"(
	{
		for (UStruct Super = Class; Super; Super = Super->Super)
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
				"\tinline ProcessEvent(class UFunction* Function, void* Parms) const", "",
				std::format("\n\t{{\n\t\treturn GetVFunction<void(*)(UObject*, class UFunction*, void*)>(this, {})(this, Function, Parms);\n\t}}", Off::InSDK::PEIndex)
			}
		}
	};

	PredefinedFunctions["UClass"] =
	{
		"CoreUObject",
		{
			{
				"\tclass UFunction* GetFunction(std::string& ClassName, std::string& FuncName);",
				"\tclass UFunction* UClass::GetFunction(std::string& ClassName, std::string& FuncName)",
R"(
	{
		for(UStruct* Clss = Class; Clss; Clss = Clss->Super)
		{
			if (Clss->GetName() == ClassName)
			{
				for (UField* Field = Clss->Children; Field; Field = Field->Next)
				{
					if(Field->HasTypeFlag(EClassCastFlags::UFunction) && Field->GetName() == FuncName)
					{
						return Field;
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
}