#include "Wrappers/StructWrapper.h"
#include "Managers/MemberManager.h"

StructWrapper::StructWrapper(const PredefinedStruct* const Predef)
    : PredefStruct(Predef), InfoHandle()
{
}

StructWrapper::StructWrapper(const UEStruct Str)
    : Struct(Str), InfoHandle(StructManager::GetInfo(Str)), bIsUnrealStruct(true)
{
}

UEStruct StructWrapper::GetUnrealStruct() const
{
    assert(bIsUnrealStruct && "StructWrapper doesn't contain UnrealStruct. Illegal call to 'GetUnrealStruct()'.");

    return bIsUnrealStruct ? Struct : nullptr;
}

std::string StructWrapper::GetName() const
{
    return bIsUnrealStruct ? Struct.GetValidName() : PredefStruct->UniqueName;
}

std::string StructWrapper::GetRawName() const
{
    return bIsUnrealStruct ? Struct.GetName() : PredefStruct->UniqueName;
}

std::string StructWrapper::GetFullName() const
{
    return bIsUnrealStruct ? Struct.GetFullName() : "Predefined struct " + PredefStruct->UniqueName;
}

StructWrapper StructWrapper::GetSuper() const
{
    return bIsUnrealStruct ? StructWrapper(Struct.GetSuper()) : PredefStruct->Super;
}

MemberManager StructWrapper::GetMembers() const
{
    return bIsUnrealStruct ? MemberManager(Struct) : MemberManager(PredefStruct);
}


/* Name, bIsUnique */
std::pair<std::string, bool> StructWrapper::GetUniqueName() const
{
    return { bIsUnrealStruct ? InfoHandle.GetName().GetName() : PredefStruct->UniqueName, bIsUnrealStruct ? InfoHandle.GetName().IsUnique() : true };
}

int32 StructWrapper::GetLastMemberEnd() const
{
    return bIsUnrealStruct ? InfoHandle.GetLastMemberEnd() : 0x0;
}

int32 StructWrapper::GetAlignment() const
{
    return bIsUnrealStruct ? InfoHandle.GetAlignment() : PredefStruct->Alignment;
}

int32 StructWrapper::GetSize() const
{
    return bIsUnrealStruct ? InfoHandle.GetSize() : Align(PredefStruct->Size, PredefStruct->Alignment);
}

int32 StructWrapper::GetUnalignedSize() const
{
    return bIsUnrealStruct ? InfoHandle.GetUnalignedSize() : PredefStruct->Size;
}

bool StructWrapper::ShouldUseExplicitAlignment() const
{
    return bIsUnrealStruct ? InfoHandle.ShouldUseExplicitAlignment() : PredefStruct->bUseExplictAlignment;
}

bool StructWrapper::HasReusedTrailingPadding() const
{
    return bIsUnrealStruct && InfoHandle.HasReusedTrailingPadding();
}

bool StructWrapper::IsFinal() const
{
    return bIsUnrealStruct ? InfoHandle.IsFinal() : PredefStruct->bIsFinal;
}

bool StructWrapper::IsClass() const
{
    return bIsUnrealStruct ? Struct.IsA(EClassCastFlags::Class) : PredefStruct->bIsClass;
}

bool StructWrapper::IsUnion() const
{
    return !bIsUnrealStruct && PredefStruct->bIsUnion;
}

bool StructWrapper::IsFunction() const
{
    return bIsUnrealStruct && Struct.IsA(EClassCastFlags::Function);
}

bool StructWrapper::IsInterface() const
{
    static UEClass InterfaceClass = ObjectArray::FindClassFast("Interface");

    return bIsUnrealStruct && Struct.IsA(EClassCastFlags::Class) && Struct.HasType(InterfaceClass);
}

bool StructWrapper::IsAClassWithType(UEClass TypeClass) const
{
    return IsUnrealStruct() && IsClass() && Struct.Cast<UEClass>().IsA(TypeClass);
}


bool StructWrapper::IsValid() const
{
    // Struct and PredefStruct share the same memory location, if Struct is nullptr so is PredefStruct
    return PredefStruct != nullptr;
}

bool StructWrapper::IsUnrealStruct() const
{
    return bIsUnrealStruct;
}

bool StructWrapper::IsCyclicWithPackage(int32 PackageIndex) const
{
    if (!bIsUnrealStruct || PackageIndex == -1)
        return false;

    if (!InfoHandle.IsPartOfCyclicPackage())
        return false;

    return StructManager::IsStructCyclicWithPackage(Struct.GetIndex(), PackageIndex);
}

bool StructWrapper::HasCustomTemplateText() const
{
    return !IsUnrealStruct() && !PredefStruct->CustomTemplateText.empty();
}

std::string StructWrapper::GetCustomTemplateText() const
{
    assert(!IsUnrealStruct() && "StructWrapper doesn't contain PredefStruct. Illegal call to 'GetCustomTemplateText()'.");

    return PredefStruct->CustomTemplateText;
}
