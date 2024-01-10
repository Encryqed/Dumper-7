#include "EnumWrapper.h"
#include "EnumManager.h"

EnumWrapper::EnumWrapper(const UEEnum Enm)
    : Enum(Enm), InfoHandle(EnumManager::GetInfo(Enm))
{
}

UEEnum EnumWrapper::GetUnrealEnum() const
{
    return Enum;
}

std::string EnumWrapper::GetName() const
{
    return Enum.GetEnumPrefixedName();
}

std::string EnumWrapper::GetFullName() const
{
    return Enum.GetFullName();
}

std::pair<std::string, bool> EnumWrapper::GetUniqueName() const
{
    const StringEntry& Name = InfoHandle.GetName();

    return { Name.GetName(), Name.IsUnique() };
}

uint8 EnumWrapper::GetUnderlyingTypeSize() const
{
    return InfoHandle.GetUnderlyingTypeSize();
}

CollisionInfoIterator EnumWrapper::GetMembers() const
{
    return InfoHandle.GetMemberCollisionInfoIterator();
}

bool EnumWrapper::IsValid() const
{
    return Enum != nullptr;
}

