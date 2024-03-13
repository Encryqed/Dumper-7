#include "DSGen.h"

#include <fstream>

DSGen::DSGen()
{
}

void DSGen::setDirectory(const std::filesystem::path& directory)
{
	DSGen::directory = directory / "Dumpspace";

	dumpTimeStamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());


	if(!create_directories(DSGen::directory))
	{
		remove_all(DSGen::directory);
	}
}

void DSGen::addOffset(const std::string& name, uintptr_t offset)
{
	offsets.push_back(std::pair(name, offset));
}

DSGen::ClassHolder DSGen::createStructOrClass(const std::string& name, bool isClass, int size,
                                              const std::vector<std::string>& inheritedClasses)
{
	ClassHolder c;
	c.className = name;
	c.classType = isClass ? ET_Class : ET_Struct;
	c.classSize = size;
	//inherited types are all a class/struct if the current generated type is a class/struct
	if (!inheritedClasses.empty())
		c.interitedTypes.insert(c.interitedTypes.begin(), inheritedClasses.begin(), inheritedClasses.end());

	return c;
}

DSGen::MemberType DSGen::createMemberType(EType type, const std::string& typeName, const std::string& extendedType,
	const std::vector<MemberType>& subTypes, bool isReference)
{
	MemberType t;
	t.type = type;
	t.typeName = typeName;
	t.extendedType = extendedType;
	t.reference = isReference;
	if (!subTypes.empty())
		t.subTypes.insert(t.subTypes.begin(), subTypes.begin(), subTypes.end());
	return t;
}

void DSGen::addMemberToStructOrClass(ClassHolder& classHolder, const std::string& memberName, EType type,
	const std::string& typeName, const std::string& extendedType, int offset, int size, int bitOffset)
{
	MemberType t;
	t.type = type;
	t.typeName = typeName;
	t.extendedType = extendedType;

	MemberDefinition m;
	m.memberType = t;
	m.memberName = memberName;
	m.offset = offset;
	m.size = size;
	m.bitOffset = bitOffset;

	classHolder.members.push_back(m);
}

void DSGen::addMemberToStructOrClass(ClassHolder& classHolder, const std::string& memberName, const MemberType& memberType,
	int offset, int size, int bitOffset)
{
	MemberDefinition m;
	m.memberType = memberType;
	m.memberName = memberName;
	m.offset = offset;
	m.size = size;
	m.bitOffset = bitOffset;

	classHolder.members.push_back(m);
}

DSGen::EnumHolder DSGen::createEnum(const std::string& enumName, const std::string& enumType,
	const std::vector<std::pair<std::string, int>>& enumMembers)
{
	EnumHolder e;
	e.enumName = enumName;
	e.enumType = enumType;
	if (!enumMembers.empty())
		e.enumMembers.insert(e.enumMembers.begin(), enumMembers.begin(), enumMembers.end());

	return e;
}



void DSGen::createFunction(ClassHolder& owningClass, const std::string& functionName,
                                            const std::string& functionFlags, uintptr_t functionOffset, const MemberType& returnType,
                                            const std::vector<std::pair<MemberType, std::string>>& functionParams)
{
	FunctionHolder f;
	f.functionName = functionName;
	f.functionFlags = functionFlags;
	f.functionOffset = functionOffset;
	f.returnType = returnType;
	if (!functionParams.empty())
		f.functionParams.insert(f.functionParams.begin(), functionParams.begin(), functionParams.end());

	owningClass.functions.push_back(f);
}

void DSGen::bakeStructOrClass(ClassHolder& classHolder)
{
	nlohmann::json jClass;

	nlohmann::json membersArray = nlohmann::json::array();

	nlohmann::json inheritInfo = nlohmann::json::array();
	for (auto& super : classHolder.interitedTypes)
		inheritInfo.push_back(super);

	nlohmann::json InheritInfo;
	InheritInfo["__InheritInfo"] = inheritInfo;
	membersArray.push_back(InheritInfo);

	nlohmann::json gSize;
	gSize["__MDKClassSize"] = classHolder.classSize;
	membersArray.push_back(gSize);

	for(auto& member : classHolder.members)
	{
		nlohmann::json jmember;


		if (member.bitOffset > -1)
			jmember[member.memberName + " : 1"] = std::make_tuple(member.memberType.jsonify(), member.offset, member.size, member.bitOffset);
		else
			jmember[member.memberName] = std::make_tuple(member.memberType.jsonify(), member.offset, member.size);
		membersArray.push_back(jmember);
	}
	nlohmann::json j;

	j[classHolder.className] = membersArray;

	if (classHolder.classType == ET_Class)
		classes.push_back(j);
	else
		structs.push_back(j);

	if(!classHolder.functions.empty())
	{
		nlohmann::json classFunctions = nlohmann::json::array();
		for (auto& func : classHolder.functions)
		{
			nlohmann::json functionParams = nlohmann::json::array();

			for (const auto& param : func.functionParams)
				functionParams.push_back(std::make_tuple(param.first.jsonify(), param.first.reference ? "&" : "", param.second));

			nlohmann::json j;
			j[func.functionName] = std::make_tuple(func.returnType.jsonify(), functionParams, func.functionOffset, func.functionFlags);
			classFunctions.push_back(j);
		}
		nlohmann::json f;

		f[classHolder.className] = classFunctions;

		functions.push_back(f);
	}

}

void DSGen::bakeEnum(EnumHolder& enumHolder)
{
	nlohmann::json members = nlohmann::json::array();
	for(const auto& member : enumHolder.enumMembers)
	{
		nlohmann::json m;
		m[member.first] = member.second;
		members.push_back(m);
	}
	nlohmann::json j;
	j[enumHolder.enumName] = std::make_tuple(members, enumHolder.enumType);
	enums.push_back(j);
}

void DSGen::dump()
{
	if (directory.empty())
		throw std::exception("Please initialize a directory first!");

	auto saveToDisk = [&](const nlohmann::json& json, const std::string& fileName)
	{
		nlohmann::json j;
		j["updated_at"] = dumpTimeStamp;
		j["data"] = json;

		std::ofstream file(directory / fileName);
		file << j.dump();
	};

	saveToDisk(nlohmann::json(nlohmann::json(offsets)), "OffsetsInfo.json");
	saveToDisk(nlohmann::json(classes), "ClassesInfo.json");
	saveToDisk(nlohmann::json(functions), "FunctionsInfo.json");
	saveToDisk(nlohmann::json(structs), "StructsInfo.json");
	saveToDisk(nlohmann::json(enums), "EnumsInfo.json");


}
