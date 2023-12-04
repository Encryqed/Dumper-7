#include "MemberWrappers.h"


PropertyWrapper::PropertyWrapper(const StructWrapper& Str, const PredefinedMember* Predef)
    : PredefProperty(Predef), Struct(&Str), Name()
{
}

PropertyWrapper::PropertyWrapper(const StructWrapper& Str, UEProperty Prop)
    : Property(Prop), Name(MemberManager::GetNameCollisionInfo(Str.GetUnrealStruct(), Prop)), Struct(&Str), bIsUnrealProperty(true)
{
}

std::string PropertyWrapper::GetName() const
{
    return bIsUnrealProperty ? MemberManager::StringifyName(Struct->GetUnrealStruct(), Name) : PredefProperty->Name;
}

std::string PropertyWrapper::GetType() const
{
    assert(!bIsUnrealProperty && "PropertyWrapper doesn't contian UnrealProperty. Illegal call to 'GetNameCollisionInfo()'.");

    return PredefProperty->Type;
}

NameInfo PropertyWrapper::GetNameCollisionInfo() const
{
    assert(bIsUnrealProperty && "PropertyWrapper doesn't contian UnrealProperty. Illegal call to 'GetNameCollisionInfo()'.");

    return Name;
}

bool PropertyWrapper::IsReturnParam() const
{
    return bIsUnrealProperty && Property.HasPropertyFlags(EPropertyFlags::ReturnParm);
}

UEProperty PropertyWrapper::GetUnrealProperty() const
{
    return Property;
}

bool PropertyWrapper::IsBitField() const
{
    if (bIsUnrealProperty)
        return Property.IsA(EClassCastFlags::BoolProperty) && !Property.Cast<UEBoolProperty>().IsNativeBool();

    return PredefProperty->bIsBitField;
}


uint8 PropertyWrapper::GetBitIndex() const
{
    assert(IsBitField() && "'GetBitIndex' was called on non-bitfield member!");

    return bIsUnrealProperty ? Property.Cast<UEBoolProperty>().GetBitIndex() : PredefProperty->BitIndex;
}

uint8 PropertyWrapper::GetFieldMask() const
{
    assert(IsBitField() && "'GetFieldMask' was called on non-bitfield member!");

    return bIsUnrealProperty ? Property.Cast<UEBoolProperty>().GetFieldMask() : (1 << PredefProperty->BitIndex);
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

std::string PropertyWrapper::StringifyFlags() const
{
    return bIsUnrealProperty ? Property.StringifyFlags() : "NoFlags";
}



bool PropertyWrapper::IsUnrealProperty() const
{
    return bIsUnrealProperty;
}

FunctionWrapper::FunctionWrapper(const StructWrapper& Str, const PredefinedFunction* Predef)
    : PredefFunction(Predef), Struct(&Str), Name()
{
}

FunctionWrapper::FunctionWrapper(const StructWrapper& Str, UEFunction Func)
    : Function(Func), Name(MemberManager::GetNameCollisionInfo(Str.GetUnrealStruct(), Func)), Struct(&Str), bIsUnrealFunction(true)
{
}

StructWrapper FunctionWrapper::AsStruct() const
{
    return StructWrapper(bIsUnrealFunction ? Function : nullptr);
}

std::string FunctionWrapper::GetName() const
{
    return bIsUnrealFunction ? MemberManager::StringifyName(Struct->GetUnrealStruct(), Name) : PredefFunction->Name;
}

NameInfo FunctionWrapper::GetNameCollisionInfo() const
{
    assert(bIsUnrealFunction && "FunctionWrapper doesn't contian UnrealFunction. Illegal call to 'GetNameCollisionInfo()'.");

    return Name;
}

PropertyWrapper FunctionWrapper::GetReturnParam() const
{
    assert(bIsUnrealFunction && "FunctionWrapper doesn't contian UnrealFunction. Illegal call to 'GetReturnParam()'.");

    for (auto Param : Function.GetProperties())
    {
        if (Param.HasPropertyFlags(EPropertyFlags::ReturnParm))
            return PropertyWrapper(*Struct, Param);
    }
}

EFunctionFlags FunctionWrapper::GetFunctionFlags() const
{
    return bIsUnrealFunction ? Function.GetFunctionFlags() : EFunctionFlags::None;
}

std::string FunctionWrapper::StringifyFlags() const
{
    return bIsUnrealFunction ? Function.StringifyFlags() : "NoFlags";
}

std::string FunctionWrapper::GetParamStructName() const
{
    assert(bIsUnrealFunction && "FunctionWrapper doesn't contian UnrealFunction. Illegal call to 'GetParamStructName()'.");

    return Function.GetOuter().GetName() + "_" + Function.GetName();
}