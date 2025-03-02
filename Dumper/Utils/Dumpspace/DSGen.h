#pragma once

#include <string>
#include <filesystem>
#include "../Json/json.hpp"

class DSGen
{
public:

	// The EType enum describes the type of the member of a class, struct, enum or param of a function
	// or the class, struct, function, enum itself.
	// Setting the correct type is important for the dumspace website in order to know where to redirect
	// when clicking on the type.
	// Pointers or references dont play a role here, for example is the member UWorld* owningWorld a ET_Class.
	// More examples:
	// int itemCount -> ET_Default
	// EToastType type -> ET_Enum (if we can confirm EToastType is a enum)
	// FVector location -> ET_Struct (if we can confirm FVector is a struct)
	// AWeaponDef* weaponDefinition -> ET_Class (if we can confirm AWeaponDef is a class)
	// Template examples:
	// TMap<int, FVector> -> ET_Class, we ignore what the templates (int, FVector) are
	//
	// Keep in mind, if you have a member and its type is UNKNOWN, so not defined, mark the member as ET_Default.
	// Otherwise, the website will search for the type, resulting in a missing definition error.
	enum EType
	{
		ET_Default, // all types that are either undefined or default (int, bool, char,..., FType (undefined))
		ET_Struct, // all types that are a struct (FVector, Fquat)
		ET_Class, // all types that are a class (Uworld, UWorld*)
		ET_Enum, // all types that are a enum (EToastType)
		ET_Function //not needed, only needed for function definition


	};

	static std::string getTypeShort(EType type)
	{
		if (type == ET_Default)
			return "D";
		if (type == ET_Struct)
			return "S";
		if (type == ET_Class)
			return "C";
		if (type == ET_Enum)
			return "E";
		if (type == ET_Function)
			return "F";
		return {};
	}

	// MemberType struct holds information about the members type
	struct MemberType
	{
		EType type; // the EType of the membertype
		std::string typeName; // the name of the type, e.g UClass (this should not contain any * or & !!)
		std::string extendedType; // if the type is a UClass* make sure the * is in here!! If not, this can be left empty
		bool reference = false; // only needed in function parameters. Ingore otherwise
		std::vector<MemberType> subTypes; // most of the times empty, just needed if the MemberType is a template, e.g TArray<abc>

		/**
		 * \brief creates a JSON with all the information about the MemberType and SubTypes
		 * \return returns a JSON with all the information about the MemberType and SubTypes
		 */
		nlohmann::json jsonify() const
		{
			//create a array for the memberType
			nlohmann::json arr = nlohmann::json::array();
			arr.push_back(typeName); //first the typeName
			arr.push_back(getTypeShort(type)); //then the short Type
			arr.push_back(extendedType); //then any extended type
			nlohmann::json subTypeArr = nlohmann::json::array();
			for (auto& subType : subTypes)
				subTypeArr.push_back(subType.jsonify());
			
			arr.push_back(subTypeArr);

			return arr;
		}
	};

	// MemberDefinition contains all the information of the member needed
	struct MemberDefinition
	{
		MemberType memberType;
		std::string memberName;
		int offset;
		int bitOffset;
		int size;
		int arrayDim;

	};

	struct FunctionHolder
	{
		std::string functionName; // the name of the function e.g getWeapon
		std::string functionFlags; // flags how to call the function e.g Blueprint|Static|abc
		uintptr_t functionOffset; // offset of the function within the binary
		MemberType returnType; // the return type, if there's no, pass void
		std::vector<std::pair<MemberType, std::string>> functionParams; // the function params with their type and name
	};

	// Classholder can be for classes and structs, they are theoretically the same. The struct holds all information about the clas
	struct ClassHolder
	{
		int classSize;
		EType classType;
		std::string className;
		std::vector<std::string> interitedTypes;
		std::vector<MemberDefinition> members;
		std::vector<FunctionHolder> functions;
	};

	struct EnumHolder
	{
		std::string enumType; // enum type, uint32_t or int or uint64_t
		std::string enumName; // name of the enum 
		std::vector<std::pair<std::string, int>> enumMembers; //enum members, their name and representative number (abc = 5)
	};



private:
	static inline std::string dumpTimeStamp{};

	static inline std::filesystem::path directory{};

	static inline std::vector<std::tuple<std::string, uintptr_t>> offsets{};

	static inline nlohmann::json classes = nlohmann::json::array();
	static inline nlohmann::json structs = nlohmann::json::array();
	static inline nlohmann::json functions = nlohmann::json::array();
	static inline nlohmann::json enums = nlohmann::json::array();

public:
	//redundant constructor
	DSGen();

	/**
	 * \brief sets the directory path. The dumpspace files will be under directory/dumpspace
	 * \param directory valid directory
	 */
	static void setDirectory(const std::filesystem::path& directory);

	/**
	 * \brief 
	 * \param name the name of the offset
	 * \param offset the offset (base address subtracted!)
	 */
	static void addOffset(const std::string& name, uintptr_t offset);

	/**
	 * \brief creates a class or struct you can use to add members. If size or inherits are unknown, you can add them later on.
	 * \param name the name of the class or struct
	 * \param isClass whether its a class or struct
	 * \param size the sizeof the class
	 * \param inheritedClasses the inherited classes or structs, ordered from last to first
	 * (ClassC : ClassB, ClassB : ClassA, ClassA : Root -> {ClassB, ClassA, Root})
	 * \return a ClassHolder struct for you to add members
	 */
	static ClassHolder createStructOrClass(
		const std::string& name, 
		bool isClass = true, 
		int size = 0, 
		const std::vector<std::string>& inheritedClasses = {});

	/**
	 * \brief creates a MemberType struct. Creating a MemberType is only really needed if the Member is a template type and has subTypes or if the Member is a subtype
	 * \param type EType (e.g TArray<int> -> EType::ET_Class)
	 * \param typeName the name (e.g TArray)
	 * \param extendedType the extended type (e.g TArray<abc>& -> &, TArray<def>* -> *, or empty if there's no extended type)
	 * \param subTypes any subtypes (e.g TArray<int> -> "int" is the subtype, or empty if there are no subtypes)
	 * \param isReference leave this false, unless you create a memberType for a function which is a reference
	 * \return the MemberType struct
	 */
	static MemberType createMemberType(
		EType type, 
		const std::string& typeName, 
		const std::string& extendedType = "", 
		const std::vector<MemberType>& subTypes = {},
		bool isReference = false
	);

	/**
	 * \brief adds a new member to the class or struct
	 * \param classHolder ClassHolder struct where the member gets added to
	 * \param memberName the name of the member
	 * \param type the EType of the member (e.g Ulevel* -> ET_Class)
	 * \param typeName the type name of the member (e.g level)
	 * \param extendedType the extended type, if any (e,g ULevel* -> *, int& -> &, or empty)
	 * \param offset the offset of the member within the struct or class
	 * \param size the size of the member
	 * \param arrayDim the array dimension of the member, default 1 (int foo -> 1, int foo[123] -> 123)
	 * \param bitOffset the bit Offset of the member, leave -1 if member has no bitOffset
	 */
	static void addMemberToStructOrClass(
		ClassHolder& classHolder, 
		const std::string& memberName, 
		EType type, 
		const std::string& typeName, 
		const std::string& extendedType, 
		int offset, 
		int size,
		int arrayDim = 1,
		int bitOffset = -1
	);

	/**
	 * \brief adds a new member to the class or struct
	 * \param classHolder ClassHolder struct where the member gets added to
	 * \param memberName the name of the member
	 * \param memberType memberType struct
	 * \param offset the offset of the member within the struct or class
	 * \param size the size of the member
	 * \param arrayDim the array dimension of the member, default 1 (int foo -> 1, int foo[123] -> 123)
	 * \param bitOffset the bit Offset of the member, leave -1 if member has no bitOffset
	 */
	static void addMemberToStructOrClass(
		ClassHolder& classHolder,
		const std::string& memberName,
		const MemberType& memberType,
		int offset,
		int size,
		int arrayDim = 1,
		int bitOffset = -1
	);

	/**
	 * \brief creates a EnumHolder
	 * \param enumName the name of the enum
	 * \param enumType the type of the enum (int, uint32_t, uint64_t,...)
	 * \param enumMembers enum members, their name and representative number (abc = 5)
	 * \return the EnumHolder struct
	 */
	static EnumHolder createEnum(
		const std::string& enumName, 
		const std::string& enumType, 
		const std::vector<std::pair<std::string, int>>& 
		enumMembers
	);

	/**
	 * \brief creates a FunctionHolder 
	 * \param owningClass the owning class' or struct name this function resides in
	 * \param functionName the name of the function e.g getWeapon
	 * \param functionFlags flags how to call the function e.g Blueprint|Static|abc
	 * \param functionOffset offset of the function within the binary
	 * \param returnType the return type, if there's no, pass void
	 * \param functionParams the function params with their type and name
	 */
	static void createFunction(
		ClassHolder& owningClass,
		const std::string& functionName,
		const std::string& functionFlags,
		uintptr_t functionOffset,
		const MemberType& returnType,
		const std::vector<std::pair<MemberType,
		std::string>>&functionParams = {}
	);

	/**
	 * \brief bakes a ClassHolder which gets later dumped
	 * \param classHolder the classHolder that should get baked
	 */
	static void bakeStructOrClass(ClassHolder& classHolder);

	/**
	 * \brief bakes a EnumHolder which gets later dumped
	 * \param enumHolder the enumHolder that should get baked
	 */
	static void bakeEnum(EnumHolder& enumHolder);


	/**
	 * \brief dumps all baked information to disk. This should be the final step
	 */
	static void dump();
};