#include "Types.h"
#include "Enums.h"
#include "Generator.h"


Types::Struct::Struct(std::string Name, bool bIsClass, std::string Super)
{
	StructMembers.reserve(50);

	CppName = Name;

	if (!bIsClass)
	{
		Declaration = (Super == "" ? std::format("struct {}\n", Name) : std::format("struct {} : public {}\n", Name, Super));
	}
	else
	{
		Declaration = (Super == "" ? std::format("class {}\n", Name) : std::format("class {} : public {}\n", Name, Super));
	}

	InnerBody = "{\npublic:\n";
}

void Types::Struct::AddComment(std::string Comment)
{
	Comments += "// " + Comment + "\n";
}

void Types::Struct::AddMember(Member& NewMember)
{
	StructMembers.push_back(NewMember);
}
void Types::Struct::AddMember(Member&& NewMember)
{
	StructMembers.emplace_back(std::move(NewMember));
}

void Types::Struct::AddMembers(std::vector<Member>& NewMembers)
{
	for (Member NewMember : NewMembers)
	{
		StructMembers.push_back(NewMember);
	}
}

std::string Types::Struct::GetGeneratedBody()
{
	for (Member StructMember : StructMembers)
	{
		InnerBody += StructMember.GetGeneratedBody();
	}

	return Comments + Declaration + InnerBody + "};\n\n";
}


void Types::Class::AddFunction(Function& NewFunction)
{
	ClassFunctions.push_back(NewFunction);
}

void Types::Class::AddFunction(Function&& NewFunction)
{
	ClassFunctions.emplace_back(std::move(NewFunction));
}

std::string Types::Class::GetGeneratedBody()
{
	for (Member ClassMember : StructMembers)
	{
		InnerBody += ClassMember.GetGeneratedBody();
	}

	if (Settings::bShouldXorStrings)
	{
		InnerBody += std::format("\n\tstatic class UClass* StaticClass()\n\t{{\n\t\tstatic class UClass* Clss = UObject::FindClassFast({}(\"{}\"));\n\t\treturn Clss;\n\t}}\n\n", Settings::XORString, RawName);
	}
	else
	{
		InnerBody += std::format("\n\tstatic class UClass* StaticClass()\n\t{{\n\t\tstatic class UClass* Clss = UObject::FindClassFast(\"{}\");\n\t\treturn Clss;\n\t}}\n\n", RawName);
	}
	
	if (Generator::PredefinedFunctions.find(CppName) != Generator::PredefinedFunctions.end())
	{
		for (auto& PredefFunc : Generator::PredefinedFunctions[CppName].second)
		{
			InnerBody += "\n" + PredefFunc.DeclarationH;

			if (PredefFunc.DeclarationCPP == "")
				InnerBody += PredefFunc.Body;
			
			InnerBody += "\n";
		}
	}

	for (Function ClassFunction : ClassFunctions)
	{
		InnerBody += std::format("\t{};\n", ClassFunction.GetDeclaration());
	}

	return Comments + Declaration + InnerBody + "};\n\n";
}

Types::Includes::Includes(std::vector<std::pair<std::string, bool>> HeaderFiles)
{
	this->HeaderFiles = HeaderFiles;

	for (auto Pair : HeaderFiles)
	{
		if (Pair.second)
		{
			Declaration += std::format("#include <{}>\n", Pair.first);
		}
		else
		{
			Declaration += std::format("#include \"{}\"\n", Pair.first);
		}
	}
}


std::string Types::Includes::GetGeneratedBody()
{
	return Declaration + "\n";
}

Types::Member::Member(std::string Type, std::string Name, std::string Comment)
{
	this->Type = Type + " ";
	this->Name = Name;
	this->Comment = Comment != "" ? "// " + Comment : "";
}


void Types::Member::AddComment(std::string Comment)
{
	this->Comment = "// " + Comment;
}

std::string Types::Member::GetGeneratedBody()
{
	return std::format("\t{:{}}{:{}} {}\n", Type, 40, Name + ";", 55, Comment);
}

Types::Function::Function(std::string Type, std::string Name, std::string SuperName, std::vector<Parameter> Parameters)
{
	this->Parameters = Parameters;

	std::string ParamStr = GetParametersAsString();

	DeclarationH = std::format("{} {}({})", Type, Name, ParamStr);
	DeclarationCPP = std::format("{} {}::{}({})", Type, SuperName, Name, ParamStr);

	Body = "{";
}

std::vector<Types::Parameter>& Types::Function::GetParameters()
{
	return Parameters;
}

std::string Types::Function::GetParametersAsString()
{
	if (Parameters.empty())
	{
		return std::string();
	}

	std::string Output;

	for (Parameter Param : Parameters)
	{
		Output += Param.GetGeneratedBody();
	}

	Output.resize(Output.size() - 2);

	return Output;
}

std::string Types::Function::GetDeclaration()
{
	 return DeclarationH;
}
void Types::Function::AddBody(std::string Body)
{
	this->Body = std::format("{{\n{}\n}}", Body);
}

void Types::Function::SetParamStruct(Types::Struct&& Params)
{
	ParamStruct = Params;
}

Types::Struct& Types::Function::GetParamStruct()
{
	return ParamStruct;
}

void Types::Function::AddComment(std::string& Comment)
{
	Comments += std::format("// {}\n", Comment);
}

void Types::Function::AddComment(std::string&& Comment)
{
	Comments += std::format("// {}\n", Comment);
}

std::string Types::Function::GetGeneratedBody()
{
	return std::format("\n{}\n{}\n{}\n", Comments, DeclarationCPP, Body);
}

Types::Parameter::Parameter(std::string Type, std::string Name, bool bIsOutPtr)
{
	this->bIsOutPtr = bIsOutPtr;
	this->Type = Type;
	this->Name = Name;
}

std::string Types::Parameter::GetName()
{
	return Name;
}

std::string Types::Parameter::GetGeneratedBody()
{
	return std::format("{} {}, ", Type, Name);
}

bool Types::Parameter::IsParamOutPtr()
{
	return bIsOutPtr;
}

Types::Enum::Enum(std::string Name)
{
	Declaration = std::format("{}\n", Name);
}

Types::Enum::Enum(std::string Name, std::string Type)
{
	Declaration = std::format("{} : {}\n", Name, Type);
}

void Types::Enum::AddComment(std::string&& Comment)
{
	Declaration = std::format("// {}\n{}", std::move(Comment), Declaration);
}

void Types::Enum::AddMember(std::string&& Name, int64 Value)
{
	EnumMembers.emplace_back(std::move(Name), Value);
}

void Types::Enum::FixWindowsConstant(std::string&& ConstantName)
{
	for (auto& EnumMember : EnumMembers)
	{
		if (EnumMember.first == ConstantName)
		{
			EnumMember.first += "_";
		}
	}
}

std::string Types::Enum::GetGeneratedBody()
{
	std::string Body = Declaration + "{\n";

	for (const auto& EnumMember : EnumMembers)
	{
		Body += std::format("\t{:{}} = {},\n", EnumMember.first, 30, EnumMember.second);
	}

	return Body + "};\n";
}