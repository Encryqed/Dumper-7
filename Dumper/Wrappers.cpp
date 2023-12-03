#include "Wrappers.h"
#include "StructManager.h"
#include "MemberManager.h"

StructWrapper::StructWrapper(const PredefinedStructBase* const Predef)
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
    return bIsUnrealStruct ? Struct.GetName() : "INVALID";
}

std::string StructWrapper::GetFullName() const
{
    return bIsUnrealStruct ? Struct.GetFullName() : "INVALID2";
}

StructWrapper StructWrapper::GetSuper() const
{
    return bIsUnrealStruct ? StructWrapper(Struct.GetSuper()) : PredefStruct->Super;
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

bool StructWrapper::IsValid() const
{
    return Struct != nullptr;
}

PropertyWrapper::PropertyWrapper(const StructWrapper& Str, const PredefinedMember* Predef)
    : PredefProperty(Predef), Struct(&Str), Name()
{
}

PropertyWrapper::PropertyWrapper(const StructWrapper& Str, UEProperty Prop)
    : Property(Prop), Name(/* to do*/), Struct(&Str), bIsUnrealProperty(true)
{
}

std::string PropertyWrapper::GetName() const
{
    return bIsUnrealProperty ? MemberManager::StringifyName(Struct->GetUnrealStruct(), Name) : PredefProperty->Name;
}

NameInfo PropertyWrapper::GetNameCollisionInfo() const
{
    assert(bIsUnrealProperty && "PropertyWrapper doesn't contian UnrealProperty. Illegal call to 'GetNameCollisionInfo()'.");

    return Name;
}

int32 PropertyWrapper::GetArrayDim() const
{
    return bIsUnrealProperty ? Property.GetArrayDim() : PredefProperty->ArrayDim;
}

int32 PropertyWrapper::GetSize() const
{
    return bIsUnrealProperty ? Property.GetSize() : PredefProperty->Size;
}

int32 PropertyWrapper::GetOffset() const
{
    return bIsUnrealProperty ? Property.GetOffset() : PredefProperty->Offset;
}

EPropertyFlags PropertyWrapper::GetPropertyFlags() const
{
    return bIsUnrealProperty ? Property.GetPropertyFlags() : EPropertyFlags::None;
}

bool PropertyWrapper::IsReturnParam() const
{
    return bIsUnrealProperty && Property.HasPropertyFlags(EPropertyFlags::ReturnParm);
}

std::string PropertyWrapper::StringifyFlags() const
{
    return bIsUnrealProperty ? Property.StringifyFlags() : "NoFlags";
}

FunctionWrapper::FunctionWrapper(const StructWrapper& Str, const PredefinedFunction* Predef)
    : PredefFunction(Predef), Struct(&Str), Name()
{
}

FunctionWrapper::FunctionWrapper(const StructWrapper& Str, UEFunction Func)
    : Function(Func), Name(/* to do*/), Struct(&Str), bIsUnrealFunction(true)
{
}

StructWrapper FunctionWrapper::AsStruct() const
{
    return bIsUnrealFunction ? StructWrapper(Function) : StructWrapper(PredefFunction);
}

std::string FunctionWrapper::GetName() const
{
    return bIsUnrealFunction ? MemberManager::StringifyName(Struct->GetUnrealStruct(), Name) : PredefFunction->UniqueName;
}

NameInfo FunctionWrapper::GetNameCollisionInfo() const
{
    assert(bIsUnrealFunction && "PropertyWrapper doesn't contian UnrealProperty. Illegal call to 'GetNameCollisionInfo()'.");

    return Name;
}

PropertyWrapper FunctionWrapper::GetReturnProperty() const
{

}

EFunctionFlags FunctionWrapper::GetFunctionFlags() const
{

}

std::string FunctionWrapper::StringifyFlags() const
{

}
std::string FunctionWrapper::GetParamStructName() const
{

}
