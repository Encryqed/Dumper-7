#pragma once

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
	FileWriter(std::string FileName);
	FileWriter(std::string FileName, FileType Type);
	~FileWriter();

	void Open(std::string FileName);
	void Open(std::string FileName, FileType Type);
	void Close();
	void Write(std::string Text);
	void WriteIncludes(Types::Includes& Includes);
	void WriteStruct(Types::Struct& Struct);
	void WriteEnum(Types::Enum& Enum);
	void WriteClass(Types::Class& Class);
	void SetFileType(FileType& Type);
	void SetFileHeader();
	void SetFileEnding();
};
