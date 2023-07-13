#pragma once

#include <filesystem>
#include "Types.h"
#include "Settings.h"

namespace fs = std::filesystem;

class FileWriter
{
	friend class Generator;

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
	bool bWroteFunctionsBefore;
	bool bWroteParametersBefore;

public:
	FileWriter(const fs::path& FilePath);
	FileWriter(const fs::path& FilePath, const std::string& FileName, FileType Type);
	FileWriter(const std::string& FileName);
	FileWriter(const std::string& FileName, FileType Type);
	~FileWriter();

	void Open(std::string FileName);
	void Open(std::string FileName, FileType Type);
	void Close();
	void Write(std::string& Text);
	void Write(std::string&& Text);
	void WriteIncludes(Types::Includes& Includes);
	void WriteParamStruct(Types::Struct& Struct);
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

	std::ofstream& DebugGetStream();
};

template<class StreamType>
class MappingFileWriter
{
private:
	StreamType Stream;

public:
	MappingFileWriter() = default;

	MappingFileWriter(fs::path FilePath)
		: Stream(FilePath, std::ios::binary)
	{
	}

	template<typename T>
	inline void Write(T Data)
	{
		Stream.write(reinterpret_cast<const char*>(&Data), sizeof(Data));
	}

	inline void WriteStr(const std::string& Data)
	{
		Stream << Data;
	}

	inline uint32 GetSize()
	{
		Stream.flush();

		// yes, I pasted this from shade
		auto Pos = Stream.tellp();
		Stream.seekp(0, SEEK_END);
		auto Ret = Stream.tellp();
		Stream.seekp(Pos, SEEK_SET);

		return Ret;
	}

	inline auto Rdbuf()
	{
		return Stream.rdbuf();
	}

	template<class T>
	inline void CopyFromOtherBuffer(MappingFileWriter<T>& Other)
	{
		Stream.flush();

		Stream << Other.Rdbuf();
	}

	~MappingFileWriter()
	{
		Stream.flush();
	}
};
