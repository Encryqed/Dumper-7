#pragma once
#include <unordered_map>
#include "ObjectArray.h"
#include "HashStringTable.h"
#include "NameCollisionHandler.h"
#include "PredefinedMembers.h"


template<bool bIsDeferredTemplateCreation = true>
class MemberIterator
{
private:
	using PredefType = std::conditional_t<bIsDeferredTemplateCreation, PredefinedMember, void>;
	using DereferenceType = std::conditional_t<bIsDeferredTemplateCreation, class PropertyWrapper, void>;

private:
	const UEStruct Struct;

	const std::vector<UEProperty>& Members;
	const std::vector<PredefType>* PredefElements;

	int32 CurrentIdx = 0x0;
	int32 CurrentPredefIdx = 0x0;

	bool bIsCurrentlyPredefined = true;

public:
	inline MemberIterator(const UEStruct Str, const std::vector<UEProperty>& Mbr, const std::vector<PredefType>* const Predefs = nullptr, int32 StartIdx = 0x0, int32 PredefStart = 0x0)
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
	const UEStruct Struct;

	const std::vector<UEFunction>& Members;
	const std::vector<PredefType>* PredefElements;

	int32 CurrentIdx = 0x0;
	int32 CurrentPredefIdx = 0x0;

	bool bIsCurrentlyPredefined = true;

public:
	inline FunctionIterator(const UEStruct Str, const std::vector<UEFunction>& Mbr, const std::vector<PredefType>* const Predefs = nullptr, int32 StartIdx = 0x0, int32 PredefStart = 0x0)
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
	friend class MemberManagerTest;

private:
	/* Nametable used for storing the string-names of member-/function-names contained by NameInfos */
	static inline HashStringTable MemberNames;

	/* Member-names and name-collision info*/
	static inline CollisionManager::NameInfoMapType NameInfos;

	/* Map to translation from UEProperty/UEFunction to Index in NameContainer */
	static inline CollisionManager::TranslationMapType TranslationMap;

	/* Map to lookup if a struct has predefined members */
	static inline const PredefinedMemberLookupMapType* PredefinedMemberLookup = nullptr;

private:
	UEStruct Struct;

	std::vector<UEProperty> Members;
	std::vector<UEFunction> Functions;

	const std::vector<PredefinedMember>* PredefMembers = nullptr;
	const std::vector<PredefinedFunction>* PredefFunctions = nullptr;

	int32 NumStaticPredefMembers = 0x0;
	int32 NumStaticPredefFunctions = 0x0;

private:
	MemberManager(UEStruct Str);

public:
	int32 GetNumFunctions() const;
	int32 GetNumMembers() const;

	int32 GetNumPredefFunctions() const;
	int32 GetNumPredefMembers() const;

	bool HasFunctions() const;
	bool HasMembers() const;

	inline MemberIterator<true> IterateMembers() const
	{
		return MemberIterator<true>(Struct, Members, PredefMembers);
	}

	inline FunctionIterator<true> IterateFunctions() const
	{
		return FunctionIterator<true>(Struct, Functions, PredefFunctions);
	}

public:
	static inline void SetPredefinedMemberLookupPtr(const PredefinedMemberLookupMapType* Lookup)
	{
		PredefinedMemberLookup = Lookup;
	}

	static inline void InitMemberNameCollisions()
	{
		static bool bInitialized = false;

		if (bInitialized)
			return;

		bInitialized = true;

		for (auto Obj : ObjectArray())
		{
			if (!Obj.IsA(EClassCastFlags::Struct) || Obj.IsA(EClassCastFlags::Function))
				continue;

			AddStructToNameContainer(Obj.Cast<UEStruct>());
		}
	}

	static inline void AddStructToNameContainer(UEStruct Struct)
	{
		CollisionManager::AddStructToNameContainer(NameInfos, TranslationMap, MemberNames, Struct);
	}

	template<typename UEType>
	static inline NameInfo GetNameCollisionInfo(UEStruct Struct, UEType Member)
	{
		static_assert(std::is_same_v<UEType, UEProperty> || std::is_same_v<UEType, UEFunction>, "Type arguement in 'GetNameCollisionInfo' is of invalid type!");

		assert(Struct && "'GetNameCollisionInfo()' called with 'Struct' == nullptr");
		assert(Member && "'GetNameCollisionInfo()' called with 'Member' == nullptr");

		CollisionManager::NameContainer& InfosForStruct = NameInfos[Struct.GetIndex()];
		uint64 NameInfoIndex = TranslationMap[CollisionManager::KeyFunctions::GetKeyForCollisionInfo(Struct, Member)];

		return InfosForStruct[NameInfoIndex];
	}

	static inline std::string StringifyName(UEStruct Struct, NameInfo Name)
	{
		return CollisionManager::StringifyName(MemberNames, Struct, Name);
	}
};
