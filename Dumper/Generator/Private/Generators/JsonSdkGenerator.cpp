#include "JsonSdkGenerator.h"
#include "DumpspaceGenerator.h"

#include "Unreal/ObjectArray.h"
#include "Managers/DependencyManager.h"

#include "../Settings.h"
#include "Utils/Json/json.hpp"

#include <fstream>

nlohmann::json JsonSdkGenerator::MemberTypeToJson(const DSGen::MemberType& Type)
{
	nlohmann::json j;
	j["type"] = Type.typeName;
	j["extended"] = Type.extendedType;
	j["ref"] = Type.reference;
	if (!Type.subTypes.empty())
	{
		nlohmann::json arr = nlohmann::json::array();
		for (const auto& st : Type.subTypes)
			arr.push_back(MemberTypeToJson(st));
		j["subtypes"] = arr;
	}
	return j;
}

nlohmann::json JsonSdkGenerator::MemberToJson(const DSGen::MemberDefinition& Member)
{
	nlohmann::json j;
	j["name"] = Member.memberName;
	j["type"] = MemberTypeToJson(Member.memberType);
	j["offset"] = Member.offset;
	j["size"] = Member.size;
	j["arrayDim"] = Member.arrayDim;
	if (Member.bitOffset >= 0)
		j["bitOffset"] = Member.bitOffset;
	return j;
}

nlohmann::json JsonSdkGenerator::FunctionToJson(const DSGen::FunctionHolder& Func)
{
	nlohmann::json j;
	j["name"] = Func.functionName;
	j["flags"] = Func.functionFlags;
	j["offset"] = Func.functionOffset;
	j["returnType"] = MemberTypeToJson(Func.returnType);
	nlohmann::json params = nlohmann::json::array();
	for (const auto& p : Func.functionParams)
	{
		nlohmann::json pj;
		pj["type"] = MemberTypeToJson(p.first);
		pj["name"] = p.second;
		params.push_back(pj);
	}
	j["params"] = params;
	return j;
}

nlohmann::json JsonSdkGenerator::StructOrClassToJson(const DSGen::ClassHolder& Holder, bool bIsClass)
{
	nlohmann::json j;
	j["name"] = Holder.className;
	j["size"] = Holder.classSize;
	j["inherits"] = Holder.interitedTypes;
	nlohmann::json members = nlohmann::json::array();
	for (const auto& m : Holder.members)
		members.push_back(MemberToJson(m));
	j["members"] = members;
	if (bIsClass && !Holder.functions.empty())
	{
		nlohmann::json funcs = nlohmann::json::array();
		for (const auto& f : Holder.functions)
			funcs.push_back(FunctionToJson(f));
		j["functions"] = funcs;
	}
	return j;
}

nlohmann::json JsonSdkGenerator::EnumToJson(const DSGen::EnumHolder& Holder)
{
	nlohmann::json j;
	j["name"] = Holder.enumName;
	j["underlying"] = Holder.enumType;
	nlohmann::json vals = nlohmann::json::array();
	for (const auto& v : Holder.enumMembers)
		vals.push_back({ v.first, v.second });
	j["values"] = vals;
	return j;
}

void JsonSdkGenerator::Generate()
{
	nlohmann::json sdk;
	sdk["game"] = Settings::Generator::GameName;
	sdk["version"] = Settings::Generator::GameVersion;

	nlohmann::json structsArr = nlohmann::json::array();
	nlohmann::json enumsArr = nlohmann::json::array();
	nlohmann::json classesArr = nlohmann::json::array();

	for (PackageInfoHandle Package : PackageManager::IterateOverPackageInfos())
	{
		if (Package.IsEmpty())
			continue;

		for (int32 EnumIdx : Package.GetEnums())
		{
			DSGen::EnumHolder Enum = DumpspaceGenerator::GenerateEnum(ObjectArray::GetByIndex<UEEnum>(EnumIdx));
			enumsArr.push_back(EnumToJson(Enum));
		}

		DependencyManager::OnVisitCallbackType GenerateCallback = [&](int32 Index) -> void
		{
			DSGen::ClassHolder StructOrClass = DumpspaceGenerator::GenerateStruct(ObjectArray::GetByIndex<UEStruct>(Index));
			if (StructOrClass.classType == DSGen::ET_Class)
				classesArr.push_back(StructOrClassToJson(StructOrClass, true));
			else
				structsArr.push_back(StructOrClassToJson(StructOrClass, false));
		};

		if (Package.HasStructs())
		{
			const DependencyManager& Structs = Package.GetSortedStructs();
			Structs.VisitAllNodesWithCallback(GenerateCallback);
		}

		if (Package.HasClasses())
		{
			const DependencyManager& Classes = Package.GetSortedClasses();
			Classes.VisitAllNodesWithCallback(GenerateCallback);
		}
	}

	sdk["structs"] = structsArr;
	sdk["enums"] = enumsArr;
	sdk["classes"] = classesArr;

	std::ofstream file(MainFolder / "SDK.json");
	file << sdk.dump(-1, ' ', false, nlohmann::detail::error_handler_t::replace);
}
