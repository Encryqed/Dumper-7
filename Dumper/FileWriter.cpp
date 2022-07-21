#include "FileWriter.h"
#include "Generator.h"

FileWriter::FileWriter(fs::path FilePath)
{
	FileStream.open(FilePath);
	SetFileHeader();
}

FileWriter::FileWriter(fs::path FilePath, std::string FileName, FileType Type)
{
	CurrentFileType = Type;
	CurrentFile = FileName;
	SetFileType(Type);
	FileStream.open(FilePath / CurrentFile);
	SetFileHeader();
}

FileWriter::FileWriter(std::string FileName)
{
	CurrentFile = FileName;
	Open(CurrentFile);
	SetFileHeader();
}

FileWriter::FileWriter(std::string FileName, FileType Type)
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
	CurrentFile = FileName;
	FileStream.open(CurrentFile);
}

void FileWriter::Open(std::string FileName, FileType Type)
{
	Close();
	CurrentFile = FileName;
	SetFileType(Type);
	Open(CurrentFile);
}

void FileWriter::Close()
{
	SetFileEnding();
	FileStream.flush();
	FileStream.close();
}

void FileWriter::Write(std::string& Text)
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

void FileWriter::WriteStruct(Types::Struct& Struct)
{
	Write(Struct.GetGeneratedBody());
}

void FileWriter::WriteStructs(std::vector<Types::Struct>& Structs)
{
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
	for (Types::Enum& Enum : Enums)
	{
		WriteEnum(Enum);
	}
}

void FileWriter::WriteFunction(Types::Function& Function)
{
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
	#pragma pack(push, 0x08)
#endif

#include "../SDK.hpp"

)";

	if (Settings::bUseNamespaceForSDK)
		FileStream << std::format("namespace {}\n{{\n\n\n", Settings::SDKNamespaceName);

	if (CurrentFileType == FileType::Parameter && Settings::bUseNamespaceForParams)
		FileStream << std::format("namespace {}\n{{\n\n\n", Settings::ParamNamespaceName);
}

void FileWriter::SetFileEnding()
{
	if (Settings::bUseNamespaceForSDK)
		FileStream << "\n}\n";

	if (CurrentFileType == FileType::Parameter && Settings::bUseNamespaceForParams)
		FileStream << "\n}\n";

	FileStream << R"(
#ifdef _MSC_VER
	#pragma pack(pop)
#endif
)";
}
