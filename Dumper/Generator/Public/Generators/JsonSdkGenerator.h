#pragma once

#include "Unreal/ObjectArray.h"

#include "Managers/PackageManager.h"

#include "Wrappers/EnumWrapper.h"
#include "Wrappers/StructWrapper.h"

#include "Dumpspace/DSGen.h"


class JsonSdkGenerator
{
private:
	friend class Generator;

private:
	using StreamType = std::ofstream;

public:
	static inline PredefinedMemberLookupMapType PredefinedMembers;

	static inline std::string MainFolderName = "SDK";
	static inline std::string SubfolderName = "";

	static inline fs::path MainFolder;
	static inline fs::path Subfolder;

private:
	static nlohmann::json MemberTypeToJson(const DSGen::MemberType& Type);
	static nlohmann::json MemberToJson(const DSGen::MemberDefinition& Member);
	static nlohmann::json FunctionToJson(const DSGen::FunctionHolder& Func);
	static nlohmann::json StructOrClassToJson(const DSGen::ClassHolder& Holder, bool bIsClass);
	static nlohmann::json EnumToJson(const DSGen::EnumHolder& Holder);

public:
	static void Generate();

	static void InitPredefinedMembers() { }
	static void InitPredefinedFunctions() { }
};
