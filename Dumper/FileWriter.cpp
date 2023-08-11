#include "FileWriter.h"
#include "Settings.h"

FileWriter::FileWriter(const fs::path& FilePath)
{
	FileStream.open(FilePath);
	SetFileHeader();
}

FileWriter::FileWriter(const fs::path& FilePath, const std::string& FileName, FileType Type)
{
	CurrentFileType = Type;
	CurrentFile = FileName;
	SetFileType(Type);
	FileStream.open(FilePath / CurrentFile);
	if (!FileStream.is_open()) std::cout << "Couldn't open \"" << CurrentFile << "\"" << std::endl;
	SetFileHeader();
}

FileWriter::FileWriter(const std::string& FileName)
{
	CurrentFile = FileName;
	Open(CurrentFile);
	SetFileHeader();
}

FileWriter::FileWriter(const std::string& FileName, FileType Type)
{
	CurrentFile = FileName;
	CurrentFileType = Type;
	SetFileType(Type);
	Open(CurrentFile);
	SetFileHeader();
}

FileWriter::~FileWriter()
{
	Close();
}

void FileWriter::Open(std::string FileName)
{
	Close();
	CurrentFile = Settings::FilePrefix != nullptr ? Settings::FilePrefix + FileName : FileName;
	FileStream.open(CurrentFile);
}

void FileWriter::Open(std::string FileName, FileType Type)
{
	Close();
	CurrentFile = Settings::FilePrefix != nullptr ? Settings::FilePrefix + FileName : FileName;
	SetFileType(Type);
	Open(CurrentFile);
}

void FileWriter::Close()
{
	SetFileEnding();
	FileStream.flush();
	FileStream.close();
}

void FileWriter::Write(const std::string& Text)
{
	FileStream << Text;
}
void FileWriter::Write(std::string&& Text)
{
	FileStream << std::move(Text);
}

void FileWriter::WriteIncludes(Types::Includes& Includes)
{
	Write(Includes.GetGeneratedBody());
}

void FileWriter::WriteParamStruct(Types::Struct& Struct)
{
	if (!bWroteParametersBefore)
	{
		bWroteParametersBefore = true;

		Write(
			R"(//---------------------------------------------------------------------------------------------------------------------
// PARAMETERS
//---------------------------------------------------------------------------------------------------------------------

)");
	}
	Write(Struct.GetGeneratedBody());
}

void FileWriter::WriteStruct(Types::Struct& Struct)
{
	Write(Struct.GetGeneratedBody());
}

void FileWriter::WriteStructs(std::vector<Types::Struct>& Structs)
{
	Write(
		R"(//---------------------------------------------------------------------------------------------------------------------
// STRUCTS
//---------------------------------------------------------------------------------------------------------------------

)");

	for (Types::Struct& Struct : Structs)
	{
		WriteStruct(Struct);
	}
}

void FileWriter::WriteEnum(Types::Enum& Enum)
{
	Write(Enum.GetGeneratedBody());
}

void FileWriter::WriteEnums(std::vector<Types::Enum>& Enums)
{
	Write(
		R"(//---------------------------------------------------------------------------------------------------------------------
// ENUMS
//---------------------------------------------------------------------------------------------------------------------

)");

	for (Types::Enum& Enum : Enums)
	{
		WriteEnum(Enum);
	}

	Write("\n");
}

void FileWriter::WriteFunction(Types::Function& Function)
{
	if (!bWroteFunctionsBefore)
	{
		bWroteFunctionsBefore = true;

		Write(
			R"(//---------------------------------------------------------------------------------------------------------------------
// FUNCTIONS
//---------------------------------------------------------------------------------------------------------------------

)");
	}

	Write(Function.GetGeneratedBody());
}

void FileWriter::WriteFunctions(std::vector<Types::Function>& Functions)
{
	for (Types::Function& Function : Functions)
	{
		WriteFunction(Function);
	}
}

void FileWriter::WriteClass(Types::Class& Class)
{
	Write(Class.GetGeneratedBody());
}

void FileWriter::WriteClasses(std::vector<Types::Class>& Classes)
{
	Write(
		R"(//---------------------------------------------------------------------------------------------------------------------
// CLASSES
//---------------------------------------------------------------------------------------------------------------------

)");
	for (Types::Class& Class : Classes)
	{
		WriteClass(Class);
	}
}

void FileWriter::SetFileType(FileType& Type)
{
	switch (Type)
	{
	case FileWriter::FileType::Parameter:
		CurrentFile += "_parameters.hpp";
		break;
	case FileWriter::FileType::Function:
		CurrentFile += "_functions.cpp";
		break;
	case FileWriter::FileType::Struct:
		CurrentFile += "_structs.hpp";
		break;
	case FileWriter::FileType::Class:
		CurrentFile += "_classes.hpp";
		break;
	case FileWriter::FileType::Source:
		CurrentFile += ".cpp";
		break;
	case FileWriter::FileType::Header:
		CurrentFile += ".hpp";
		break;
	default:
		CurrentFile += ".hpp";
		break;
	}
}

void FileWriter::SetFileHeader()
{
	FileStream << R"(#pragma once

// Dumped with Dumper-7!

#ifdef _MSC_VER
	#pragma pack(push, 0x01)
#endif

)";

	if (CurrentFileType == FileType::Function || CurrentFileType == FileType::Parameter || CurrentFileType == FileType::Source)
		FileStream << "#include \"../SDK.hpp\"\n\n";

	if (Settings::bUseNamespaceForSDK)
		FileStream << std::format("namespace {}\n{{\n", Settings::SDKNamespaceName);

	if (CurrentFileType == FileType::Parameter && Settings::bUseNamespaceForParams)
		FileStream << std::format("namespace {}\n{{\n", Settings::ParamNamespaceName);
}

void FileWriter::SetFileEnding()
{
	if (Settings::bUseNamespaceForSDK)
		FileStream << "}\n";

	if (CurrentFileType == FileType::Parameter && Settings::bUseNamespaceForParams)
		FileStream << "}\n";

	FileStream << R"(
#ifdef _MSC_VER
	#pragma pack(pop)
#endif
)";
}


std::ofstream& FileWriter::DebugGetStream()
{
	return FileStream;
}
