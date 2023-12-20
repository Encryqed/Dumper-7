#include "StructWrapper.h"
#include "StructManager.h"
#include "MemberManager.h"

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
    assert(bIsUnrealStruct && "StructWrapper doesn't contian UnrealStruct. Illegal call to 'GetUnrealStruct()'.");

    return bIsUnrealStruct ? Struct : nullptr;
}

std::string StructWrapper::GetName() const
{
    return bIsUnrealStruct ? InfoHandle.GetName().GetName() : PredefStruct->UniqueName;
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

int32 StructWrapper::GetAlignment() const
{
    return bIsUnrealStruct ? InfoHandle.GetAlignment() : PredefStruct->Alignment;
}

int32 StructWrapper::GetSize() const
{
    return bIsUnrealStruct ? InfoHandle.GetSize() : PredefStruct->Size;
}

bool StructWrapper::ShouldUseExplicitAlignment() const
{
    return bIsUnrealStruct ? InfoHandle.ShouldUseExplicitAlignment() : PredefStruct->bUseExplictAlignment;
}

bool StructWrapper::IsFinal() const
{
    return bIsUnrealStruct ? InfoHandle.IsFinal() : PredefStruct->bIsFinal;
}

bool StructWrapper::IsClass() const
{
    return bIsUnrealStruct ? Struct.IsA(EClassCastFlags::Class) : PredefStruct->bIsClass;
}

bool StructWrapper::IsValid() const
{
    return Struct != nullptr;
}
