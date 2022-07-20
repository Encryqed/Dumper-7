#pragma once

#include <filesystem>
#include "FileWriter.h"
#include "Settings.h"
#include "Package.h"

class Generator
{
private:
	friend class Package;

	struct PredefinedFunction
	{
		std::string DeclarationH;
		std::string DeclarationCPP;
		std::string Body;
	};

public:
	Generator();

	void Start();
};