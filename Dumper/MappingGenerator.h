#pragma once
#include "ObjectArray.h"
#include "MemberWrappers.h"
#include "EnumWrapper.h"

class MappingGenerator
{
private:
    using StreamType = std::ofstream;

public:
    static inline PredefinedMemberLookupMapType PredefinedMembers;

    static inline std::string MainFolderName = "Mappings";
    static inline std::string SubfolderName = "";

    static inline fs::path MainFolder;
    static inline fs::path Subfolder;

private:
    static EMappingsTypeFlags GetMappingType(UEProperty Property) { } // Emtpy for now

public:
    static void Generate() {} // Empty for now

    /* Always empty, there are no predefined members for mappings */
    static void InitPredefinedMembers() { }
    static void InitPredefinedFunctions() { }
};
