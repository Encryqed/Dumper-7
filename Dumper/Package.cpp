#include "Package.h"


void Package::Process(std::vector<int32_t>& PackageMembers)
{
	for (int32_t Index : PackageMembers)
	{
		UEObject Object = ObjectArray::GetByIndex(Index);

		if (!Object || Object.HasAnyFlags(EObjectFlags::RF_ClassDefaultObject))
		{
			continue;
		}

		if (Object.IsA(EClassCastFlags::UEnum))
		{
			GenerateEnum(Object.Cast<UEEnum&>());
		}
		else if (Object.IsA(EClassCastFlags::UClass))
		{
			GenerateClass(Object.Cast<UEClass&>());
		}
		else if (Object.IsA(EClassCastFlags::UStruct) && !Object.IsA(EClassCastFlags::UFunction))
		{
			GenerateStruct(Object.Cast<UEStruct&>());
		}
	}
}

void Package::GenerateMembers(std::vector<UEProperty>& MemberVector, UEStruct& Super, std::vector<Types::Member>& OutMembers)
{
	int PrevPropertyEnd = 0;
	int PrevBoolPropertyEnd = 0;
	int PrevBoolPropertyBit = 1;

	for (auto& Property : MemberVector)
	{
		std::string CppType = Property.GetCppType();
		std::string Name = Property.GetValidName();

		const int Offset = Property.GetOffset();
		const int Size = Property.GetSize();

		std::string Comment = std::format("0x{:X}(0x{:X})({})", Offset, Size, Property.StringifyFlags());

		if (Offset > PrevPropertyEnd)
		{
			OutMembers.push_back(GenerateBytePadding(PrevPropertyEnd, Offset - PrevPropertyEnd, "Fixing Size after Last Property  [ Dumper-7 ]"));
		}

		if (Property.IsA(EClassCastFlags::UBoolProperty) &&  !Property.Cast<UEBoolProperty>().IsNativeBool())
		{
			Name += " : 1";

			const uint8 BitIndex = Property.Cast<UEBoolProperty>().GetBitIndex();

			Comment = std::format("Mask : 0x{:X} {}", Property.Cast<UEBoolProperty>().GetFieldMask(), Comment);

			if (PrevBoolPropertyEnd < Offset)
				PrevBoolPropertyBit = 1;

			if (PrevBoolPropertyBit < BitIndex)
			{
				OutMembers.push_back(GenerateBitPadding(Offset, BitIndex - PrevBoolPropertyBit, "Fixing Bit-Field Size  [ Dumper-7 ]"));
			}

			PrevBoolPropertyBit = BitIndex + 1;
			PrevBoolPropertyEnd = Offset;
		}

		Types::Member Member(CppType, Name, Comment);

		PrevPropertyEnd = Offset + Size;

		OutMembers.push_back(Member);
	}
}

Types::Function Package::GenerateFunction(UEFunction & Function, UEStruct & Super)
{
}

Types::Struct Package::GenerateStruct(UEStruct& Struct, bool bIsFunction)
{
}

Types::Class Package::GenerateClass(UEClass& Class)
{
}

Types::Enum Package::GenerateEnum(UEEnum& Enum)
{
}

Types::Member Package::GenerateBytePadding(int32 Offset, int32 PadSize, std::string&& Reason)
{
	static uint32 PadNum = 0;

	return Types::Member("uint8", std::format("Pad_{:X}[0x{:X}]", PadNum++, PadSize), Reason);
}

Types::Member Package::GenerateBitPadding(int32 Offset, int32 PadSize, std::string&& Reason)
{
	return Types::Member("uint8", std::format(": {:X}", PadSize), Reason);
}