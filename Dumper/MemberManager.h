#pragma once
#include <unordered_map>
#include <memory>
#include "ObjectArray.h"
#include "HashStringTable.h"
#include "CollisionManager.h"
#include "PredefinedMembers.h"


template<bool bIsDeferredTemplateCreation = true>
class MemberIterator
{
private:
	using PredefType = std::conditional_t<bIsDeferredTemplateCreation, PredefinedMember, void>;
	using DereferenceType = std::conditional_t<bIsDeferredTemplateCreation, class PropertyWrapper, void>;

private:
	const std::shared_ptr<class StructWrapper> Struct;

	const std::vector<UEProperty>& Members;
	const std::vector<PredefType>* PredefElements;

	int32 CurrentIdx = 0x0;
	int32 CurrentPredefIdx = 0x0;

	bool bIsCurrentlyPredefined = true;

public:
	inline MemberIterator(const std::shared_ptr<class StructWrapper>& Str, const std::vector<UEProperty>& Mbr, const std::vector<PredefType>* const Predefs = nullptr, int32 StartIdx = 0x0, int32 PredefStart = 0x0)
		: Struct(Str), Members(Mbr), PredefElements(Predefs), CurrentIdx(StartIdx), CurrentPredefIdx(PredefStart)
	{
		const int32 NextUnrealOffset = GetUnrealMemberOffset();
		const int32 NextPredefOffset = GetPredefMemberOffset();

		if (NextUnrealOffset < NextPredefOffset) [[likely]]
		{
			bIsCurrentlyPredefined = false;
		}
		else [[unlikely]]
		{
			bIsCurrentlyPredefined = true;
		}
	}

private:
	/* bIsProperty */
	inline bool IsValidUnrealMemberIndex() const { return CurrentIdx < Members.size(); }
	inline bool IsValidPredefMemberIndex() const { return PredefElements ? CurrentPredefIdx < PredefElements->size() : false; }

	int32 GetUnrealMemberOffset() const { return IsValidUnrealMemberIndex() ? Members.at(CurrentIdx).GetOffset() : 0xFFFFFFF; }
	int32 GetPredefMemberOffset() const { return IsValidPredefMemberIndex() ? PredefElements->at(CurrentPredefIdx).Offset : 0xFFFFFFF; }

public:
	DereferenceType operator*() const
	{
		return bIsCurrentlyPredefined ? DereferenceType(Struct, &PredefElements->at(CurrentPredefIdx)) : DereferenceType(Struct, Members.at(CurrentIdx));
	}

	inline MemberIterator& operator++()
	{
		bIsCurrentlyPredefined ? CurrentPredefIdx++ : CurrentIdx++;

		const int32 NextUnrealOffset = GetUnrealMemberOffset();
		const int32 NextPredefOffset = GetPredefMemberOffset();

		bIsCurrentlyPredefined = NextPredefOffset < NextUnrealOffset;
		
		return *this;
	}

public:
	inline bool operator==(const MemberIterator& Other) const { return CurrentIdx == Other.CurrentIdx && CurrentPredefIdx == Other.CurrentPredefIdx; }
	inline bool operator!=(const MemberIterator& Other) const { return CurrentIdx != Other.CurrentIdx || CurrentPredefIdx != Other.CurrentPredefIdx; }

public:
	inline MemberIterator begin() const { return *this; }

	inline MemberIterator end() const { return MemberIterator(Struct, Members, PredefElements, Members.size(), PredefElements ? PredefElements->size() : 0x0); }
};

template<bool bIsDeferredTemplateCreation = true>
class FunctionIterator
{
private:
	using PredefType = std::conditional_t<bIsDeferredTemplateCreation, PredefinedFunction, void>;
	using DereferenceType = std::conditional_t<bIsDeferredTemplateCreation, class FunctionWrapper, void> ;

private:
	const std::shared_ptr<StructWrapper> Struct;

	const std::vector<UEFunction>& Members;
	const std::vector<PredefType>* PredefElements;

	int32 CurrentIdx = 0x0;
	int32 CurrentPredefIdx = 0x0;

	bool bIsCurrentlyPredefined = true;

public:
	inline FunctionIterator(const std::shared_ptr<StructWrapper>& Str, const std::vector<UEFunction>& Mbr, const std::vector<PredefType>* const Predefs = nullptr, int32 StartIdx = 0x0, int32 PredefStart = 0x0)
		: Struct(Str), Members(Mbr), PredefElements(Predefs), CurrentIdx(StartIdx), CurrentPredefIdx(PredefStart)
	{
		bIsCurrentlyPredefined = bShouldNextMemberBePredefined();
	}

private:
	/* bIsFunction */
	inline bool IsNextPredefFunctionInline() const { return PredefElements ? PredefElements->at(CurrentPredefIdx).bIsBodyInline : false; }
	inline bool IsNextPredefFunctionStatic() const { return PredefElements ? PredefElements->at(CurrentPredefIdx).bIsStatic : false; }
	inline bool IsNextUnrealFunctionInline() const { return HasMoreUnrealMembers() ? Members.at(CurrentIdx).HasFlags(EFunctionFlags::Static) : false; }

	inline bool HasMorePredefMembers() const { return PredefElements ? CurrentPredefIdx < PredefElements->size() : false; }
	inline bool HasMoreUnrealMembers() const { return CurrentIdx < Members.size(); }

private:
	inline bool bShouldNextMemberBePredefined() const
	{
		const bool bHasMorePredefMembers = HasMorePredefMembers();
		const bool bHasMoreUnrealMembers = HasMoreUnrealMembers();

		if (!bHasMorePredefMembers)
			return false;

		if (PredefElements)
		{
			const PredefType& PredefFunc = PredefElements->at(CurrentPredefIdx);

			// Inline-body predefs are always last
			if (PredefFunc.bIsBodyInline && bHasMoreUnrealMembers)
				return false;

			// Non-inline static predefs are always first
			if (PredefFunc.bIsStatic)
				return true;

			// Switch from static predefs to static unreal functions
			if (bHasMoreUnrealMembers && Members.at(CurrentIdx).HasFlags(EFunctionFlags::Static))
				return false;

			return !PredefFunc.bIsBodyInline || !bHasMoreUnrealMembers;
		}

		return !bHasMoreUnrealMembers;
	}

public:
	inline DereferenceType operator*() const
	{
		return bIsCurrentlyPredefined ? DereferenceType(Struct, &PredefElements->at(CurrentPredefIdx)) : DereferenceType(Struct, Members.at(CurrentIdx));
	}

	inline FunctionIterator& operator++()
	{
		bIsCurrentlyPredefined ? CurrentPredefIdx++ : CurrentIdx++;

		bIsCurrentlyPredefined = bShouldNextMemberBePredefined();
		
		return *this;
	}

public:
	inline bool operator==(const FunctionIterator& Other) const { return CurrentIdx == Other.CurrentIdx && CurrentPredefIdx == Other.CurrentPredefIdx; }
	inline bool operator!=(const FunctionIterator& Other) const { return CurrentIdx != Other.CurrentIdx || CurrentPredefIdx != Other.CurrentPredefIdx; }

public:
	inline FunctionIterator begin() const { return *this; }

	inline FunctionIterator end() const { return FunctionIterator(Struct, Members, PredefElements, Members.size(), PredefElements ? PredefElements->size() : 0x0); }
};


class MemberManager
{
private:
	friend class StructWrapper;
	friend class FunctionWrapper;
	friend class MemberManagerTest;
	friend class CollisionManagerTest;

private:
	/* Map to lookup if a struct has predefined members */
	static inline const PredefinedMemberLookupMapType* PredefinedMemberLookup = nullptr;

	/* CollisionManager containing information on colliding member-/function-names */
	static inline CollisionManager MemberNames;

private:
	const std::shared_ptr<StructWrapper> Struct;

	std::vector<UEProperty> Members;
	std::vector<UEFunction> Functions;

	const std::vector<PredefinedMember>* PredefMembers = nullptr;
	const std::vector<PredefinedFunction>* PredefFunctions = nullptr;

private:
	MemberManager(UEStruct Str);
	MemberManager(const PredefinedStruct* Str);

public:
	int32 GetNumFunctions() const;
	int32 GetNumMembers() const;

	int32 GetNumPredefFunctions() const;
	int32 GetNumPredefMembers() const;

	bool HasFunctions() const;
	bool HasMembers() const;

	MemberIterator<true> IterateMembers() const;
	FunctionIterator<true> IterateFunctions() const;

public:
	static inline void SetPredefinedMemberLookupPtr(const PredefinedMemberLookupMapType* Lookup)
	{
		PredefinedMemberLookup = Lookup;
	}

	/* Add special names like "Class", "Flags, "Parms", etc. to avoid collisions on them */
	static inline void InitReservedNames()
	{
		/* UObject reserved names */
		MemberNames.AddReservedClassName("Flags", false);
		MemberNames.AddReservedClassName("Index", false);
		MemberNames.AddReservedClassName("Class", false);
		MemberNames.AddReservedClassName("Name", false);
		MemberNames.AddReservedClassName("Outer", false);

		/* UFunction reserved names */
		MemberNames.AddReservedClassName("FunctionFlags", false);

		/* Function-body reserved names */
		MemberNames.AddReservedClassName("Func", true);
		MemberNames.AddReservedClassName("Parms", true);
		MemberNames.AddReservedClassName("Params", true);
		MemberNames.AddReservedClassName("Flgs", true);


		/* Reserved C++ keywords, typedefs and macros */
		MemberNames.AddReservedName("byte");
		MemberNames.AddReservedName("short");
		MemberNames.AddReservedName("int");
		MemberNames.AddReservedName("float");
		MemberNames.AddReservedName("double");
		MemberNames.AddReservedName("long");
		MemberNames.AddReservedName("signed");
		MemberNames.AddReservedName("unsigned");
		MemberNames.AddReservedName("operator");
		MemberNames.AddReservedName("return");

		MemberNames.AddReservedName("or");
		MemberNames.AddReservedName("and");
		MemberNames.AddReservedName("xor");

		MemberNames.AddReservedName("struct");
		MemberNames.AddReservedName("class");
		MemberNames.AddReservedName("for");
		MemberNames.AddReservedName("while");
		MemberNames.AddReservedName("this");
		MemberNames.AddReservedName("private");
		MemberNames.AddReservedName("public");
		MemberNames.AddReservedName("const");

		MemberNames.AddReservedName("int8");
		MemberNames.AddReservedName("int16");
		MemberNames.AddReservedName("int32");
		MemberNames.AddReservedName("int64");
		MemberNames.AddReservedName("uint8");
		MemberNames.AddReservedName("uint16");
		MemberNames.AddReservedName("uint32");
		MemberNames.AddReservedName("uint64");

		MemberNames.AddReservedName("TRUE");
		MemberNames.AddReservedName("FALSE");
		MemberNames.AddReservedName("true");
		MemberNames.AddReservedName("false");

		MemberNames.AddReservedName("IN");
		MemberNames.AddReservedName("OUT");

		MemberNames.AddReservedName("min");
		MemberNames.AddReservedName("max");
	}

	static inline void Init()
	{
		static bool bInitialized = false;

		if (bInitialized)
			return;

		bInitialized = true;

		/* Adds special names first, to avoid name-collisions with predefined members */
		InitReservedNames();

		/* Initialize member-name collisions  */
		for (auto Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct) || Obj.IsA(EClassCastFlags::Function))
				continue;

			AddStructToNameContainer(Obj.Cast<UEStruct>());
		}
	}

	static inline void AddStructToNameContainer(UEStruct Struct)
	{
		MemberNames.AddStructToNameContainer(Struct, (!Struct.IsA(EClassCastFlags::Class) && !Struct.IsA(EClassCastFlags::Function)));
	}

	template<typename UEType>
	static inline NameInfo GetNameCollisionInfo(UEStruct Struct, UEType Member)
	{
		static_assert(std::is_same_v<UEType, UEProperty> || std::is_same_v<UEType, UEFunction>, "Type arguement in 'GetNameCollisionInfo' is of invalid type!");

		assert(Struct && "'GetNameCollisionInfo()' called with 'Struct' == nullptr");
		assert(Member && "'GetNameCollisionInfo()' called with 'Member' == nullptr");
		
		return MemberNames.GetNameCollisionInfoUnchecked(Struct, Member);
	}

	static inline std::string StringifyName(UEStruct Struct, NameInfo Name)
	{
		return MemberNames.StringifyName(Struct, Name);
	}
};
