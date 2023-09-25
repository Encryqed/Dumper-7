#pragma once
#include <filesystem>
#include "DependencyManager.h"
#include "Nodes.h"

namespace fs = std::filesystem;

class CppGenerator
{
    static inline std::string MainFolderName = "CppSDK";
    static inline std::string SubfolderName = "SDK";

    static inline fs::path MainFolder;
    static inline fs::path Subfolder;

    static void GenerateStruct(const StructNode& Struct);
    static void GenerateClass(const StructNode& Class);
    static void GenerateFunction(const FunctionNode& Function);

    static void Generate(const DependencyManager& Dependencies);

    static void InitPredefinedMembers();
    static void InitPredefinedFunctions();
};