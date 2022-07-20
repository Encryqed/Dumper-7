#pragma once
#include <iostream>
#include <format>
#include <fstream>
#include <vector>
#include <sstream>
#include "Enums.h"

namespace Types
{
	class Includes
	{
	private:
		std::string Declaration;
		std::vector<std::pair<std::string, bool>> HeaderFiles;

	public:
		Includes(std::vector<std::pair<std::string, bool>> HeaderFiles);

		std::string GetGeneratedBody();
	};

	class Enum
	{
	private:
		std::string Declaration;
		std::string InnerBody;
		std::string WholeBody;
		std::vector<std::string> EnumMembers;

	public:
		Enum(std::string Name);
		Enum(std::string Name, std::string Type);

		void AddComment(std::string Comment);
		void AddMember(std::string Name, int64 Value);

		std::string GetGeneratedBody();
	};

	class Member
	{
	private:
		std::string Type;
		std::string Name;
		std::string Comment = "";

	public:
		Member(std::string Type, std::string Name, std::string Comment = "");

		void AddComment(std::string Comment);

		std::string GetGeneratedBody();
	};

	class Parameter
	{
	private:
		bool bIsOutPtr;
		std::string Type;
		std::string Name;

	public:
		Parameter(std::string Type, std::string Name, bool bIsOutPtr = false);

		std::string GetName();
		std::string GetGeneratedBody();
	};


	class Struct
	{
	protected:
		std::string Declaration;
		std::string InnerBody;
		std::string WholeBody;
		std::string Comments;
		std::vector<Member> StructMembers;

	public:
		Struct() = default;
		Struct(std::string Name, std::string Super = "");

		void AddComment(std::string Comment);
		void AddMember(Member& NewMember);
		void AddMember(Member&& NewMember);
		void AddMembers(std::vector<Member>& NewMembers);

		std::string GetGeneratedBody();
	};


	class Function
	{
	private:
		Struct ParamStruct;

		bool IsClassFunction;
		std::string Indent;
		std::string WholeBody;
		std::string InnerBody;
		std::string Declaration;
		std::string Type;
		std::string Name;
		std::vector<Parameter> Parameters;

	public:
		Function(std::string Type, std::string Name, std::vector<Parameter> Parameters = {}, bool IsClassFunction = true);

		std::vector<Parameter>& GetParameters();
		std::string GetParametersAsString();

		void AddBody(std::string Body);
		void SetParamStruct(Struct&& Params);

		std::string GetGeneratedBody();
		Struct GetParamStruct();
	};

	class Class : public Struct
	{
	private:
		std::vector<Function> ClassFunctions;

	public:

		using Struct::Struct;

		void AddFunction(Function& NewFunction);
		void AddFunction(Function&& NewFunction);

		std::string GetGeneratedBody();
	};
}
