#pragma once

#include <filesystem>
#include "Types.h"
#include "Settings.h"

class FileWriter
{
public:
	enum class FileType
	{
		Parameter,
		Function,
		Struct,
		Class,
		Source,
		Header
	};

private:
	std::ofstream FileStream;
	FileType CurrentFileType;
	std::string CurrentFile;

public:
	FileWriter(std::filesystem::path FilePath);
	FileWriter(std::filesystem::path FilePath, std::string FileName, FileType Type);
	FileWriter(std::string FileName);
	FileWriter(std::string FileName, FileType Type);
	~FileWriter();

	void Open(std::string FileName);
	void Open(std::string FileName, FileType Type);
	void Close();
	void Write(std::string Text);
	void WriteIncludes(Types::Includes& Includes);
	void WriteStruct(Types::Struct& Struct);
	void WriteStructs(std::vector<Types::Struct>& Structs);
	void WriteEnum(Types::Enum& Enum);
	void WriteEnums(std::vector<Types::Enum>& Enums);
	void WriteFunction(Types::Function& Function);
	void WriteFunctions(std::vector<Types::Function>& Functions);
	void WriteClass(Types::Class& Class);
	void WriteClasses(std::vector<Types::Class>& Classes);
	void SetFileType(FileType& Type);
	void SetFileHeader();
	void SetFileEnding();
};
