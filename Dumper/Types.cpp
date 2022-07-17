#include "Types.hpp"

Types::Class::Class(std::string Name)
{
	Declaration = std::format("class {}\n", Name);
	InnerBody = "{\npublic:\n";
}

Types::Class::Class(std::string Name, std::string InheritedName)
{
	Declaration = std::format("class {} : public {}\n", Name, InheritedName);
	InnerBody = "{\npublic:\n";
}

Types::Class::~Class()
{
}

void Types::Class::AddComment(std::string Comment)
{
	Comments += std::format("// {}\n", Comment);
}

void Types::Class::AddMember(Member& NewMember)
{
	ClassMembers.push_back(NewMember);
}

void Types::Class::AddMembers(std::vector<Member>& NewMembers)
{
	for (Member NewMember : NewMembers)
	{
		this->AddMember(NewMember);
	}
}

void Types::Class::AddFunction(Function& NewFunction)
{
	ClassFunctions.push_back(NewFunction);
}

std::string Types::Class::GetGeneratedBody()
{
	for (Member ClassMember : ClassMembers)
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

Types::Includes::~Includes()
{
}

std::string Types::Includes::GetGeneratedBody()
{
	return Declaration + "\n";
}

Types::Member::Member(std::string Type, std::string Name)
{
	this->Type = Type;
	this->Name = Name;
	this->Comment = "";
}

Types::Member::~Member()
{
}

void Types::Member::AddComment(std::string Comment)
{
	this->Comment = "// " + Comment;
}

std::string Types::Member::GetGeneratedBody()
{
	Declaration = std::format("\t{:{}}{:{}} {}\n", Type, 40, Name + ";", 55, Comment);

	return Declaration;
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

Types::Function::~Function()
{
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

std::string Types::Function::GetGeneratedBody()
{
	WholeBody = std::format("\n{}{}{}}}\n", Declaration, InnerBody, Indent);

	return WholeBody;
}

Types::Parameter::Parameter(std::string Type, std::string Name)
{
	this->Type = Type;
	this->Name = Name;
}

Types::Parameter::~Parameter()
{
}

std::string Types::Parameter::GetGeneratedBody()
{
	Declaration = std::format("{} {}, ", Type, Name);

	return Declaration;
}

Types::Enum::Enum(std::string Name)
{
	Declaration = std::format("{}\n", Name);
	InnerBody = "{\n";
}

Types::Enum::Enum(std::string Name, std::string Type)
{
	Declaration = std::format("enum class {} : {}\n", Name, Type);
	InnerBody = "{\n";
}

Types::Enum::~Enum()
{
}

void Types::Enum::AddComment(std::string Comment)
{
	Declaration = std::format("// {}\n{}", Comment, Declaration);
}

void Types::Enum::AddMember(std::string Name, std::string Value)
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
