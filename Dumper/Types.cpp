#include "Types.h"
#include "Enums.h"


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
	StructMembers.push_back(NewMember);
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

	WholeBody = Comments + Declaration + InnerBody + "};\n\n";

	return WholeBody;
}


void Types::Class::AddFunction(Function& NewFunction)
{
	ClassFunctions.push_back(NewFunction);
}

void Types::Class::AddFunction(Function&& NewFunction)
{
	ClassFunctions.push_back(NewFunction);
}

std::string Types::Class::GetGeneratedBody()
{
	for (Member ClassMember : StructMembers)
	{
		InnerBody += ClassMember.GetGeneratedBody();
	}

	for (Function ClassFunction : ClassFunctions)
	{
		InnerBody += ClassFunction.GetGeneratedBody();
	}

	WholeBody = Comments + Declaration + InnerBody + "};\n\n";

	return WholeBody;
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
	this->Type = Type;
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

Types::Function::Function(std::string Type, std::string Name, std::vector<Parameter> Parameters, bool IsClassFunction)
{
	this->Type = Type;
	this->Name = Name;
	this->Parameters = Parameters;
	this->IsClassFunction = IsClassFunction;

	if (IsClassFunction)
	{
		Indent = "\t";
	}
	else
	{
		Indent = "";
	}

	Declaration = std::format("{}{} {}({})\n", Indent, Type, Name, GetParametersAsString());
	InnerBody = std::format("{}{{", Indent);
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

void Types::Function::AddBody(std::string Body)
{
	InnerBody += Body;
}
void Types::Function::SetParamStruct(Types::Struct&& Params)
{
	ParamStruct = Params;
}

Types::Struct Types::Function::GetParamStruct()
{
	return ParamStruct;
}

std::string Types::Function::GetGeneratedBody()
{
	WholeBody = std::format("\n{}{}{}}}\n", Declaration, InnerBody, Indent);

	return WholeBody;
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

Types::Enum::Enum(std::string Name)
{
	Declaration = std::format("{}\n", Name);
	InnerBody = "{\n";
}

Types::Enum::Enum(std::string Name, std::string Type)
{
	Declaration = std::format("{} : {}\n", Name, Type);
	InnerBody = "{\n";
}

void Types::Enum::AddComment(std::string Comment)
{
	Declaration = std::format("// {}\n{}", Comment, Declaration);
}

void Types::Enum::AddMember(std::string Name, int64 Value)
{
	EnumMembers.push_back(std::format("\t{:{}} = {}", Name, 30, Value));
}

std::string Types::Enum::GetGeneratedBody()
{
	for (auto EnumMember : EnumMembers)
	{
		InnerBody += (EnumMember + ",\n");
	}

	InnerBody += "};\n\n";

	WholeBody = Declaration + InnerBody;

	return WholeBody;
}

Types::Struct::Struct(std::string Name, std::string Super)
{
	Declaration = (Super == "" ? std::format("struct {}\n", Name) : std::format("struct {} : public {}\n", Name, Super));
	InnerBody = "{\n";
}