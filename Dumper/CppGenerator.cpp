#include "CppGenerator.h"
#include "ObjectArray.h"


std::string CppGenerator::MakeMemberString(const std::string& Type, const std::string& Name, std::string&& Comment)
{
	return std::format("\t{:{}}{:{}} // {}\n", Type, 45, Name + ";", 50, std::move(Comment));
}

std::string CppGenerator::GenerateBytePadding(const int32 Offset, const int32 PadSize, std::string&& Reason)
{
	static uint32 PadNum = 0;

	return MakeMemberString("uint8", std::format("Pad_{:X}[0x{:X}]", PadNum++, PadSize), std::format("0x{:04X}(0x{:04X})({})", Offset, PadSize, std::move(Reason)));
}

std::string CppGenerator::GenerateBitPadding(const int32 Offset, const int32 PadSize, std::string&& Reason)
{
	static uint32 BitPadNum = 0;

	return MakeMemberString("uint8", std::format("BitPad_{:X} : {:X}", BitPadNum++, PadSize), std::format("0x{:04X}(0x{:04X})({})", Offset, PadSize, std::move(Reason)));
}

std::string CppGenerator::GenerateMembers(const StructWrapper& Struct, const std::vector<UEProperty>& Members, int32 SuperSize)
{
	static bool bDidThingOnce = false;

	if (!bDidThingOnce)
	{
		//bDidThingOnce = true;
		//void* UObject = ObjectArray::FindClassFast("Object").GetAddress();
		//std::vector<TestPredefMember>& PredefsVector = MemberManager::Predefs[UObject];
		//HashStringTable& PropertyNames = MemberManager::GetStringTable();
		//PredefsVector.emplace_back("void*", PropertyNames.FindOrAdd("Vft", true).first, Off::UObject::Vft, 0x8, 0x1);
		//PredefsVector.emplace_back("int32", PropertyNames.FindOrAdd("Flags", true).first, Off::UObject::Flags, 0x4, 0x1);
		//PredefsVector.emplace_back("int32", PropertyNames.FindOrAdd("Index",true).first, Off::UObject::Index, 0x4, 0x1);
		//PredefsVector.emplace_back("class UClass*", PropertyNames.FindOrAdd("Class", true).first, Off::UObject::Class, 0x8, 0x1);
		//PredefsVector.emplace_back("class FName", PropertyNames.FindOrAdd("Name", true).first, Off::UObject::Name, Off::InSDK::FNameSize, 0x1);
		//PredefsVector.emplace_back("class UObject*", PropertyNames.FindOrAdd("Outer", true).first, Off::UObject::Outer, 0x8, 0x1);
	}


	if (Members.empty())
		return "";

	constexpr int EstimatedCharactersPerLine = 0x80;
	constexpr int NumBitsInBytePlusOne = 0x9;

	std::string OutMembers;
	OutMembers.reserve(Members.size() * EstimatedCharactersPerLine);

	bool bLastPropertyWasBitField = false;

	int PrevPropertyEnd = SuperSize;
	int PrevBoolPropertyEnd = 0;
	int PrevBoolPropertyBit = 1;

	for (auto Member : Members)
	{
		int32 MemberOffset = Member.GetOffset();
		int32 MemberSize = Member.GetSize();

		std::string Comment = std::format("0x{:04X}(0x{:04X})({})", MemberOffset, MemberSize, StringifyPropertyFlags(Member.GetPropertyFlags()));

		if (MemberOffset >= PrevPropertyEnd && bLastPropertyWasBitField && PrevBoolPropertyBit != NumBitsInBytePlusOne)
			OutMembers += GenerateBitPadding(MemberOffset, NumBitsInBytePlusOne - PrevBoolPropertyBit, "Fixing Bit-Field Size [ Dumper-7 ]");

		if (MemberOffset > PrevPropertyEnd)
			OutMembers += GenerateBytePadding(PrevPropertyEnd, MemberOffset - PrevPropertyEnd, "Fixing Size After Last Property [ Dumper-7 ]");

		const bool bIsBitField = Member.IsA(EClassCastFlags::BoolProperty) && !Member.Cast<UEBoolProperty>().IsNativeBool();

		if (bIsBitField)
		{
			uint8 BitFieldIndex = Member.Cast<UEBoolProperty>().GetBitIndex();

			Comment = std::format("Mask: 0x{:02X}, PropSize: 0x{:04X} {}", Member.Cast<UEBoolProperty>().GetFieldMask(), MemberSize, Comment);

			if (PrevBoolPropertyEnd < MemberOffset)
				PrevBoolPropertyBit = 1;

			if (PrevBoolPropertyBit < BitFieldIndex)
				OutMembers += GenerateBitPadding(MemberOffset, BitFieldIndex - PrevBoolPropertyBit, "Fixing Bit-Field Size  [ Dumper-7 ]");

			PrevBoolPropertyBit = BitFieldIndex + 1;
			PrevBoolPropertyEnd = MemberOffset;

			bLastPropertyWasBitField = true;
		}

		PrevPropertyEnd = MemberOffset + MemberSize;

		std::string MemberName = "hardcoded, lol";// StructManager::GetMemberUniqueName(Struct, Member);

		if (Member.GetArrayDim() > 1)
			MemberName += std::format("[0x{:X}]", Member.GetArrayDim());

		OutMembers += MakeMemberString(GetMemberTypeString(Member), MemberName, std::move(Comment));
	}

	return OutMembers;
}

struct FuncInfo
{
	std::string RetValue;
	std::string Declaration;

};

std::string CppGenerator::GenerateFunctionInHeader(const std::vector<UEFunction>& Functions, const StructManager& Manager)
{
	constexpr int32 AverageNumCharactersPerFunction = 0x50;

	std::string StaticFunctions;
	std::string NonStaticFunctions;
	NonStaticFunctions.reserve(Functions.size() * AverageNumCharactersPerFunction);

	for (UEFunction Func : Functions)
	{
		std::string FuncText;





		std::string& StrToAppendto = Func.HasFlags(EFunctionFlags::Static) ? StaticFunctions : NonStaticFunctions;
		StrToAppendto += FuncText;
	}

	return "CppGenerator::GenerateFunctionInClass";
}

void CppGenerator::GenerateStruct(StreamType& StructFile, const StructManager& Manager, const StructWrapper& Struct)
{
	std::string UniqueName = GetStructPrefixedName(Struct);
	std::string UniqueSuperName;

	int32 StructSize = Struct.GetSize();
	int32 SuperSize = 0x0;

	StructWrapper Super = Struct.GetSuper();

	if (Super.IsValid())
	{
		UniqueSuperName = GetStructPrefixedName(Super);
		SuperSize = Super.GetSize();
	}

	StructFile << std::format(R"(
// {}
// 0x{:04X} (0x{:04X} - 0x{:04X})
struct {}{}{}{}
{{
)", Struct.GetFullName()
  , StructSize - SuperSize
  , StructSize
  , SuperSize
  , Struct.ShouldUseExplicitAlignment() ? std::format("alignas({:02X}) ", Struct.GetAlignment()) : ""
  , UniqueName
  , Struct.IsFinal() ? " final " : ""
  , Super.IsValid() ? (" : public " + UniqueSuperName) : "");

	// replace with 'PropertyManager' or similar class
	std::vector<UEProperty> Members = Struct.GetStruct().GetProperties();

	const bool bHasMembers = !Members.empty();
	const bool bHasFunctions = !Members.empty();

	if (bHasMembers || bHasFunctions)
		StructFile << "public:\n";

	if (bHasMembers)
	{
		StructFile << GenerateMembers(Struct, Members, SuperSize);

		if (bHasFunctions)
			StructFile << "\n\n";
	}

	if (bHasFunctions)
		StructFile << GenerateFunctionInHeader(*reinterpret_cast<const std::vector<UEFunction>*>(0), Manager);

	StructFile << "};\n";
}

void CppGenerator::GenerateClass(StreamType& ClassFile, const StructManager& Manager, UEClass Class)
{

}

void CppGenerator::GenerateFunctionInCppFile(StreamType& FunctionFile, std::ofstream& ParamFile, const UEFunction& Function)
{

}


std::string CppGenerator::GetStructPrefixedName(const StructWrapper& Struct)
{
	auto [ValidName, bIsUnique] = Struct.GetUniqueName();

	// 'GetStruct()' is just a temporary "fix"
	return (bIsUnique ? "" : (Struct.GetStruct().GetOutermost().GetValidName() + "::")) + Struct.GetName();
}

void CppGenerator::Generate(const std::unordered_map<int32, PackageInfo>& Dependencies)
{
	// Launch NumberOfProcessorCores threads

	// Generate Basic.hpp and Basic.cpp files

	// Generate SDK.hpp with sorted packages and 

	// Generate NameCollisions.inl file containing forward declarations for classes in namespaces (potentially requires lock)

}

void CppGenerator::InitPredefinedMembers()
{

}

void CppGenerator::InitPredefinedFunctions()
{

}

std::string CppGenerator::GetMemberTypeString(UEProperty Member)
{
	auto [Class, FieldClass] = Member.GetClass();
	EClassCastFlags Flags = Class ? Class.GetCastFlags() : FieldClass.GetCastFlags();

	if (Flags & EClassCastFlags::ByteProperty)
	{
		if (UEEnum Enum = Member.Cast<UEByteProperty>().GetEnum())
			return Enum.GetEnumTypeAsStr();

		return "uint8";
	}
	else if (Flags & EClassCastFlags::UInt16Property)
	{
		return "uint16";
	}
	else if (Flags & EClassCastFlags::UInt32Property)
	{
		return "uint32";
	}
	else if (Flags & EClassCastFlags::UInt64Property)
	{
		return "uint64";
	}
	else if (Flags & EClassCastFlags::Int8Property)
	{
		return "int8";
	}
	else if (Flags & EClassCastFlags::Int16Property)
	{
		return "int16";
	}
	else if (Flags & EClassCastFlags::IntProperty)
	{
		return "int32";
	}
	else if (Flags & EClassCastFlags::Int64Property)
	{
		return "int64";
	}
	else if (Flags & EClassCastFlags::FloatProperty)
	{
		return "float";
	}
	else if (Flags & EClassCastFlags::DoubleProperty)
	{
		return "double";
	}
	else if (Flags & EClassCastFlags::ClassProperty)
	{
		if (Member.GetPropertyFlags() & EPropertyFlags::UObjectWrapper)
			return std::format("TSubclassOf<class {}>", GetStructPrefixedName(Member.Cast<UEClassProperty>().GetMetaClass()));

		return "class UClass*";
	}
	else if (Flags & EClassCastFlags::NameProperty)
	{
		return "class FName";
	}
	else if (Flags & EClassCastFlags::StrProperty)
	{
		return "class FString";
	}
	else if (Flags & EClassCastFlags::TextProperty)
	{
		return "class FText";
	}
	else if (Flags & EClassCastFlags::BoolProperty)
	{
		return Member.Cast<UEBoolProperty>().IsNativeBool() ? "bool" : "uint8";
	}
	else if (Flags & EClassCastFlags::StructProperty)
	{
		return std::format("struct {}", GetStructPrefixedName(Member.Cast<UEStructProperty>().GetUnderlayingStruct()));
	}
	else if (Flags & EClassCastFlags::ArrayProperty)
	{
		return std::format("TArray<{}>", GetMemberTypeString(Member.Cast<UEArrayProperty>().GetInnerProperty()));
	}
	else if (Flags & EClassCastFlags::WeakObjectProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UEWeakObjectProperty>().GetPropertyClass())
			return std::format("TWeakObjectPtr<class {}>", GetStructPrefixedName(PropertyClass));

		return "TWeakObjectPtr<class UObject>";
	}
	else if (Flags & EClassCastFlags::LazyObjectProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UELazyObjectProperty>().GetPropertyClass())
			return std::format("TLazyObjectPtr<class {}>", GetStructPrefixedName(PropertyClass));

		return "TLazyObjectPtr<class UObject>";
	}
	else if (Flags & EClassCastFlags::SoftClassProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UESoftClassProperty>().GetPropertyClass())
			return std::format("TSoftClassPtr<class {}>", GetStructPrefixedName(PropertyClass));

		return "TSoftClassPtr<class UObject>";
	}
	else if (Flags & EClassCastFlags::SoftObjectProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UESoftObjectProperty>().GetPropertyClass())
			return std::format("TSoftObjectPtr<class {}>", GetStructPrefixedName(PropertyClass));

		return "TSoftObjectPtr<class UObject>";
	}
	else if (Flags & EClassCastFlags::ObjectProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UEObjectProperty>().GetPropertyClass())
			return std::format("class {}*", GetStructPrefixedName(PropertyClass));

		return "class UObject*";
	}
	else if (Flags & EClassCastFlags::MapProperty)
	{
		UEMapProperty MemberAsMapProperty = Member.Cast<UEMapProperty>();

		return std::format("TMap<{}, {}>", GetMemberTypeString(MemberAsMapProperty.GetKeyProperty()), GetMemberTypeString(MemberAsMapProperty.GetValueProperty()));
	}
	else if (Flags & EClassCastFlags::SetProperty)
	{
		return std::format("TSet<{}>", GetMemberTypeString(Member.Cast<UESetProperty>().GetElementProperty()));
	}
	else if (Flags & EClassCastFlags::EnumProperty)
	{
		if (UEEnum Enum = Member.Cast<UEEnumProperty>().GetEnum())
			return std::format("enum class {}", Enum.GetEnumTypeAsStr());

		return GetMemberTypeString(Member.Cast<UEEnumProperty>().GetUnderlayingProperty());
	}
	else if (Flags & EClassCastFlags::InterfaceProperty)
	{
		if (UEClass PropertyClass = Member.Cast<UEInterfaceProperty>().GetPropertyClass())
			return std::format("TScriptInterface<class {}>", GetStructPrefixedName(PropertyClass));

		return "TScriptInterface<class IInterface>";
	}
	else
	{
		return (Class ? Class.GetCppName() : FieldClass.GetCppName()) + "_";
	}
}