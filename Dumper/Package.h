#pragma once
#include "Types.h"
#include "ObjectArray.h"

class Package
{
private:
	UEObject PackageObject;

public:
	std::vector<Types::Enum> AllEnums;
	std::vector<Types::Struct> AllStructs;
	std::vector<Types::Class> AllClasses;
	std::vector<Types::Function> AllFunctions;

	Package(UEObject _Object)
		: PackageObject(_Object)
	{
	}

	void Process(std::vector<int32_t>& PackageMembers);

	void GenerateMembers(std::vector<UEProperty>& MemberVector, UEStruct& Super, Types::Struct& Struct);
	Types::Function GenerateFunction(UEFunction& Function, UEStruct& Super);
	Types::Struct GenerateStruct(UEStruct& Struct, bool bIsFunction = false);
	Types::Class GenerateClass(UEClass& Class);
	Types::Enum GenerateEnum(UEEnum& Enum);

	Types::Member GenerateBytePadding(int32 Offset, int32 PadSize, std::string&& Reason);
	Types::Member GenerateBitPadding(int32 Offset, int32 PadSize, std::string&& Reason);

	inline bool IsEmpty()
	{
		return AllEnums.empty() && AllClasses.empty() && AllStructs.empty() && AllFunctions.empty();
	}
};

