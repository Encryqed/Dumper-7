#pragma once
#include "Enums.h"
#include "Offsets.h"
#include "UnrealTypes.h"
#include <vector>

class UEClass;

class UEObject
{
protected:
	void* Object;

public:
	void* GetAddress();

	EObjectFlags GetFlags();
	bool HasAnyFlags(EObjectFlags Flags);
	int32 GetIndex();
	UEClass GetClass();
	FName GetFName();
	UEObject GetOuter();

	bool IsA(EClassCastFlags TypeFlags);

	template<typename UEType> 
	bool IsA();

	UEObject GetOutermost();

	std::string GetName();
	std::string GetValidName();
	std::string GetCppName();
	std::string GetFullName();
};

class UEField : public UEObject
{
public:
	bool IsNextValid();
	UEField GetNext();
};

class UEnum : public UEField
{
public:
	std::string GetSingleName(int32_t Index);
	std::vector<std::string> GetAllNames();
	std::string GetEnumTypeAsStr();

	std::string GetCppType();
};

class UEStruct : public UEField
{
public:
	UEStruct GetSuper();
	UEField GetChild();
	int32 GetStructSize();
};

class UEClass : public UEStruct
{
public:
	EClassCastFlags GetFlags();
	bool IsType(EClassCastFlags TypeFlag);
	UEObject GetDefaultObject();
};

class UEFunction : public UEStruct
{
public:
	EFunctionFlags GetFunctionFlags();
	bool HasFlags(EFunctionFlags Flags);
};

class UEProperty : public UEField
{
public:
	int32 GetSize();
	int32 GetOffset();
	EPropertyFlags GetFlags();
	bool HasFlags(EPropertyFlags Flags);
};