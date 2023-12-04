#pragma once
#include <unordered_map>
#include "ObjectArray.h"
#include "HashStringTable.h"
#include "NameCollisionHandler.h"
#include "PredefinedMembers.h"

template<typename T, bool bSkipStatics>
class MemberIterator
{
private:
	friend [[maybe_unused]] void TemplateTypeCreationForMemberIterator(void);

private:
	static constexpr bool bIsProperty = std::is_same_v<T, UEProperty>;
	static constexpr bool bIsFunction = std::is_same_v<T, UEFunction>;

	static_assert(bIsProperty || bIsFunction, "Invalid type for 'T', only 'UEProperty' and 'UEFunction' are allowed!");

private:
	using PredefType = std::conditional_t<bIsProperty, PredefinedMember, PredefinedFunction>;
	using DereferenceType = std::conditional_t<bIsProperty, class PropertyWrapper, class FunctionWrapper>;

private:
	const UEStruct Struct;

	const std::vector<T>& Members;
	const std::vector<PredefType>* PredefElements;

	int32 CurrentIdx = 0x0;
	int32 CurrentPredefIdx = 0x0;

	int32 CurrentOffset = 0x0;
	bool bIsCurrentlyPredefined = false;

private:
	inline MemberIterator(const std::vector<T>& M)
		: Members(M)
	{
	}

public:
	inline MemberIterator(const UEStruct Str, const std::vector<T>& Mbr, const std::vector<PredefType>* const Predefs = nullptr, int32 StartIdx = 0x0, int32 PredefStart = 0x0)
		: Struct(Str), Members(Mbr), PredefElements(Predefs), CurrentIdx(StartIdx), CurrentPredefIdx(PredefStart)
	{
	}

private:
	inline bool HasMoreUnrealMembers() const { return (CurrentIdx + 1) < Members.size(); }
	inline bool HasMorePredefMembers() const { return PredefElements ? (CurrentPredefIdx + 1) < PredefElements->size() : false; }

	int32 GetNextUnrealMemberOffset() const;
	int32 GetNextPredefMemberOffset() const;

public:
	inline DereferenceType operator*() const;

	inline MemberIterator& operator++()
	{
		if constexpr (bIsProperty)
		{
			const int32 NextUnrealOffset = GetNextUnrealMemberOffset();
			const int32 NextPredefOffset = GetNextPredefMemberOffset();

			if (NextUnrealOffset < NextPredefOffset)
			{
				bIsCurrentlyPredefined = false;
				CurrentIdx++;
			}
			else
			{
				bIsCurrentlyPredefined = true;
				CurrentPredefIdx++;
			}
		}
		else
		{
			HasMorePredefMembers() ? CurrentPredefIdx++ : CurrentIdx++;
		}
		
		return *this;
	}

public:
	inline bool operator==(const MemberIterator& Other) const { return CurrentIdx == Other.CurrentIdx && CurrentPredefIdx == Other.CurrentPredefIdx; }
	inline bool operator!=(const MemberIterator& Other) const { return CurrentIdx != Other.CurrentIdx || CurrentPredefIdx != Other.CurrentPredefIdx; }

public:
	inline MemberIterator begin() const { return *this; }

	inline MemberIterator end() const { return MemberIterator(Struct, Members, PredefElements, (Members.size() - 1), PredefElements ? (PredefElements->size() - 1) : 0x0);  }
};


class MemberManager
{
private:
	friend class StructWrapper;

private:
	using NameInfoMapType = std::unordered_map<int32 /* UniqueId/Index */, NameContainer /* NameInfos */>;
	using NameInfoTranslationMapType = std::unordered_map<uint64 /* UniquePropertyId */, uint64 /* Index into NameInfos */>;

private: /* NEW*/
	/* Nametable used for storing the string-names of member-/function-names contained by NameInfos */
	static inline HashStringTable MemberNames;

	/* Member-names and name-collision info*/
	static inline NameInfoMapType NameInfos;

	/* Map to translation from UEProperty/UEFunction to Index in NameContainer */
	static inline NameInfoTranslationMapType TranslationMap;

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
	MemberManager(UEStruct Str)
		: Struct(Str)
		, Functions(Struct.GetFunctions())
		, Members(Struct.GetProperties())
	{
		if (!PredefinedMemberLookup)
			return;

		auto It = PredefinedMemberLookup->find(Struct.GetIndex());
		if (It != PredefinedMemberLookup->end())
		{
			PredefMembers = &It->second.Members;
			PredefFunctions = &It->second.Functions;

			auto FindFirstStaticMember = [](auto& ElementVector) -> int32
			{
				for (int i = 0; i < ElementVector.size(); i++)
				{
					if (ElementVector[i].bIsStatic)
						return i + 1;
				}

				return 0x0;
			};

			NumStaticPredefMembers = FindFirstStaticMember(*PredefMembers);
			NumStaticPredefFunctions = FindFirstStaticMember(*PredefFunctions);
		}
	}

public:
	inline int32 GetNumFunctions() const
	{
		return Functions.size();
	}

	inline int32 GetNumPredefFunctions() const
	{
		return PredefFunctions ? PredefFunctions->size() : 0x0;
	}

	inline int32 GetNumMembers() const
	{
		return Members.size();
	}

	inline int32 GetNumPredefMembers() const
	{
		return PredefMembers ? PredefMembers->size() : 0x0;
	}

	inline bool HasFunctions() const
	{
		return GetNumFunctions() > 0x0 && GetNumPredefMembers() > 0x0;
	}

	inline bool HasMembers() const
	{
		return GetNumMembers() > 0x0 && GetNumPredefMembers() > 0x0;
	}

	template<bool bSkipStatics = false>
	inline MemberIterator<UEProperty, bSkipStatics> IterateMembers() const
	{
		return MemberIterator<UEProperty, bSkipStatics>(Struct, Members, PredefMembers);
	}

	template<bool bSkipStatics = false>
	inline MemberIterator<UEFunction, bSkipStatics> IterateFunctions() const
	{
		return MemberIterator<UEFunction, bSkipStatics>(Struct, Functions, PredefFunctions);
	}

public:

private: /* For internal translation between property and unique name */
	static inline uint64 GetKeyForCollisionInfo(UEProperty Member)
	{
		uint64 Key = 0x0;

		FName Name = Member.GetFName();
		Key += Name.GetCompIdx();
		Key += Name.GetNumber();

		Key <<= 32;
		Key |= Member.GetOffset();

		Key ^= static_cast<uint64>(Member.GetPropertyFlags());

		return Key;
	}

	static inline uint64 GetKeyForCollisionInfo(UEFunction Member)
	{
		uint64 Key = 0x0;

		FName Name = Member.GetFName();
		Key += Name.GetCompIdx();
		Key += Name.GetNumber();

		Key <<= 32;
		Key |= Member.GetIndex();

		return Key;
	}

public:
	static void SetPredefinedMemberLookup(const PredefinedMemberLookupMapType* Lookup)
	{
		PredefinedMemberLookup = Lookup;
	}

	static void InitMemberNameCollisions()
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

	static void AddStructToNameContainer(UEStruct Struct)
	{
		CollisionManager::AddStructToNameContainer(NameInfos, MemberNames, Struct);
	}

	static inline HashStringTable& GetStringTable()
	{
		return MemberNames;
	}

	template<typename UEType>
	static inline NameInfo GetNameCollisionInfo(UEStruct Struct, UEType Member)
	{
		static_assert(std::is_same_v<UEType, UEProperty> || std::is_same_v<UEType, UEFunction>, "Type arguement in 'GetNameCollisionInfo' is of invalid type!");

		assert(Struct && "'GetNameCollisionInfo()' called with 'Struct' == nullptr");
		assert(Member && "'GetNameCollisionInfo()' called with 'Member' == nullptr");

		NameContainer& InfosForStruct = NameInfos[Struct.GetIndex()];
		uint64 NameInfoIndex = TranslationMap[GetKeyForCollisionInfo(Member)];

		return InfosForStruct[NameInfoIndex];
	}

	static inline std::string StringifyName(UEStruct Struct, NameInfo Name)
	{
		return CollisionManager::StringifyName(MemberNames, Struct, Name);
	}
};
