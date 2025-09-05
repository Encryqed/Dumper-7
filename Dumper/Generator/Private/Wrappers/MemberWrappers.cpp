#include "Wrappers/MemberWrappers.h"


PropertyWrapper::PropertyWrapper(const std::shared_ptr<StructWrapper>& Str, const PredefinedMember* Predef)
    : PredefProperty(Predef), Struct(Str), Name()
{
}

PropertyWrapper::PropertyWrapper(const std::shared_ptr<StructWrapper>& Str, UEProperty Prop)
    : Property(Prop), Name(MemberManager::GetNameCollisionInfo(Str->GetUnrealStruct(), Prop)), Struct(Str), bIsUnrealProperty(true)
{
}

std::string PropertyWrapper::GetName() const
{
    return bIsUnrealProperty ? MemberManager::StringifyName(Struct->GetUnrealStruct(), Name) : PredefProperty->Name;
}

std::string PropertyWrapper::GetType() const
{
    assert(!bIsUnrealProperty && "PropertyWrapper doesn't contain UnrealProperty. Illegal call to 'GetNameCollisionInfo()'.");

    return PredefProperty->Type;
}

NameInfo PropertyWrapper::GetNameCollisionInfo() const
{
    assert(bIsUnrealProperty && "PropertyWrapper doesn't contain UnrealProperty. Illegal call to 'GetNameCollisionInfo()'.");

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

std::string PropertyWrapper::GetDefaultValue() const
{
    assert(!bIsUnrealProperty && "PropertyWrapper doesn't contain PredefiendMember. Illegal call to 'GetDefaultValue()'.");

    return PredefProperty->DefaultValue;
}

bool PropertyWrapper::IsType(EClassCastFlags CombinedFlags) const
{
    if (!bIsUnrealProperty)
        return false;

    uint64 CastFlags = static_cast<uint64>(Property.GetCastFlags());

    return (CastFlags & static_cast<uint64>(CombinedFlags)) > 0x0;
}

bool PropertyWrapper::HasPropertyFlags(EPropertyFlags Flags) const
{
    if (!bIsUnrealProperty)
        return false;

    return Property.HasPropertyFlags(Flags);
}

bool PropertyWrapper::IsBitField() const
{
    if (bIsUnrealProperty)
        return Property.IsA(EClassCastFlags::BoolProperty) && !Property.Cast<UEBoolProperty>().IsNativeBool();

    return PredefProperty->bIsBitField;
}

bool PropertyWrapper::HasDefaultValue() const
{
    return !bIsUnrealProperty && !PredefProperty->DefaultValue.empty();
}


uint8 PropertyWrapper::GetBitIndex() const
{
    assert(IsBitField() && "'GetBitIndex' was called on non-bitfield member!");

    return bIsUnrealProperty ? Property.Cast<UEBoolProperty>().GetBitIndex() : PredefProperty->BitIndex;
}

uint8 PropertyWrapper::GetBitCount() const
{
    assert(IsBitField() && "'GetBitSize' was called on non-bitfield member!");

    return bIsUnrealProperty ? 0x1 : PredefProperty->BitCount;
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
    if (bIsUnrealProperty)
    {
        UEStruct UnderlayingStruct = nullptr;

        if (Property.IsA(EClassCastFlags::StructProperty) && (UnderlayingStruct = Property.Cast<UEStructProperty>().GetUnderlayingStruct()))
        {
            const int32 Size = StructManager::GetInfo(UnderlayingStruct).GetSize();

            return Size > 0x0 ? Size : 0x1;
        }

        return Property.GetSize();
    }

    return PredefProperty->Size;
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

std::string PropertyWrapper::GetFlagsOrCustomComment() const
{
    return bIsUnrealProperty ? Property.StringifyFlags() : PredefProperty->Comment;
}

bool PropertyWrapper::IsUnrealProperty() const
{
    return bIsUnrealProperty;
}

bool PropertyWrapper::IsStatic() const
{
    return bIsUnrealProperty ? false : PredefProperty->bIsStatic;
}

bool PropertyWrapper::IsZeroSizedMember() const
{
    return bIsUnrealProperty ? false : PredefProperty->bIsZeroSizeMember;
}


FunctionWrapper::FunctionWrapper(const std::shared_ptr<StructWrapper>& Str, const PredefinedFunction* Predef)
    : PredefFunction(Predef), Struct(Str), Name()
{
}

FunctionWrapper::FunctionWrapper(const std::shared_ptr<StructWrapper>& Str, UEFunction Func)
    : Function(Func), Name(Str ? MemberManager::GetNameCollisionInfo(Str->GetUnrealStruct(), Func) : NameInfo()), Struct(Str), bIsUnrealFunction(true)
{
}

StructWrapper FunctionWrapper::AsStruct() const
{
    return StructWrapper(bIsUnrealFunction ? Function : nullptr);
}

std::string FunctionWrapper::GetName() const
{
    if (bIsUnrealFunction)
    {
        if (Struct) [[likely]]
            return MemberManager::StringifyName(Struct->GetUnrealStruct(), Name);

        return Function.GetValidName();
    }

    return PredefFunction->NameWithParams.substr(0, PredefFunction->NameWithParams.find_first_of('('));
}

NameInfo FunctionWrapper::GetNameCollisionInfo() const
{
    assert(bIsUnrealFunction && "FunctionWrapper doesn't contain UnrealFunction. Illegal call to 'GetNameCollisionInfo()'.");

    return Name;
}

EFunctionFlags FunctionWrapper::GetFunctionFlags() const
{
    return bIsUnrealFunction ? Function.GetFunctionFlags() : EFunctionFlags::None;
}

MemberManager FunctionWrapper::GetMembers() const
{
    assert(bIsUnrealFunction && "FunctionWrapper doesn't contain UnrealFunction. Illegal call to 'GetMembers()'.");

    return MemberManager(Function);
}

std::string FunctionWrapper::StringifyFlags(const char* Seperator) const
{
    return bIsUnrealFunction ? Function.StringifyFlags(Seperator) : "NoFlags";
}

std::string FunctionWrapper::GetParamStructName() const
{
    assert(bIsUnrealFunction && "FunctionWrapper doesn't contain UnrealFunction. Illegal call to 'GetParamStructName()'.");

    return Struct->GetName() + "_" + GetName();
}

int32  FunctionWrapper::GetParamStructSize() const
{
    return bIsUnrealFunction ? Function.GetStructSize() : 0x0;
}

std::string FunctionWrapper::GetPredefFunctionCustomComment() const
{
    assert(!bIsUnrealFunction && "FunctionWrapper doesn't contain PredefinedFunction. Illegal call to 'GetPredefFunctionCustomComment()'.");

    return PredefFunction->CustomComment;
}

std::string FunctionWrapper::GetPredefFunctionCustomTemplateText() const
{
    assert(!bIsUnrealFunction && "FunctionWrapper doesn't contain PredefinedFunction. Illegal call to 'GetPredefFunctionCustomTemplateText()'.");

    return PredefFunction->CustomTemplateText;
}

std::string FunctionWrapper::GetPredefFuncNameWithParams() const
{
    assert(!bIsUnrealFunction && "FunctionWrapper doesn't contain PredefinedFunction. Illegal call to 'GetPredefFuncNameWithParams()'.");

    return PredefFunction->NameWithParams;
}

std::string FunctionWrapper::GetPredefFuncNameWithParamsForCppFile() const
{
    assert(!bIsUnrealFunction && "FunctionWrapper doesn't contain PredefinedFunction. Illegal call to 'GetPredefFuncNameWithParamsForCppFile()'.");

    return !PredefFunction->NameWithParamsWithoutDefaults.empty() ? PredefFunction->NameWithParamsWithoutDefaults : PredefFunction->NameWithParams;
}

std::string FunctionWrapper::GetPredefFuncReturnType() const
{
    assert(!bIsUnrealFunction && "FunctionWrapper doesn't contain PredefinedFunction. Illegal call to 'GetPredefFuncReturnType()'.");

    return PredefFunction->ReturnType;
}

std::string FunctionWrapper::GetPredefFunctionBody() const
{
    assert(!bIsUnrealFunction && "FunctionWrapper doesn't contain PredefinedFunction. Illegal call to 'GetPredefFunctionBodyRef()'.");

    return PredefFunction->Body;
}

std::string FunctionWrapper::GetPredefFunctionInlineBody() const
{
    assert(!bIsUnrealFunction && "FunctionWrapper doesn't contain PredefinedFunction. Illegal call to 'GetPredefFunctionBodyRef()'.");

    std::string BodyCopy = PredefFunction->Body;

    for (int i = 0; i < BodyCopy.size(); i++)
    {
        if (BodyCopy[i] == '\n')
            BodyCopy.insert(++i, "\t");
    }

    return BodyCopy;
}

uintptr_t FunctionWrapper::GetExecFuncOffset() const
{
    if (!bIsUnrealFunction)
        return 0x0;

    return Platform::GetOffset(Function.GetExecFunction());
}

UEFunction FunctionWrapper::GetUnrealFunction() const
{
    assert(bIsUnrealFunction && "FunctionWrapper doesn't contain UnrealFunction. Illegal call to 'GetUnrealFunction()'.");

    return Function;
}

bool FunctionWrapper::IsPredefined() const
{
    return !bIsUnrealFunction;
}

bool FunctionWrapper::IsStatic() const
{
    return bIsUnrealFunction ? Function.HasFlags(EFunctionFlags::Static) : PredefFunction->bIsStatic;
}

bool FunctionWrapper::IsConst() const
{
    return bIsUnrealFunction ? Function.HasFlags(EFunctionFlags::Const) : PredefFunction->bIsConst;
}

bool FunctionWrapper::IsInInterface() const
{
    return bIsUnrealFunction && Struct->IsInterface();
}

bool FunctionWrapper::HasInlineBody() const
{
    return !bIsUnrealFunction && PredefFunction->bIsBodyInline;
}

bool FunctionWrapper::HasCustomTemplateText() const
{
    return !bIsUnrealFunction && !PredefFunction->CustomTemplateText.empty();
}

bool FunctionWrapper::HasFunctionFlag(EFunctionFlags Flag) const
{
    return bIsUnrealFunction && Function.HasFlags(Flag);
}

