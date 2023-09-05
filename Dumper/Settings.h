#pragma once


namespace Settings
{
	//Auto generated if no override is provided
	inline std::string GameName = "";
	inline std::string GameVersion = "";


	inline constexpr const char* SDKGenerationPath = "C:/Dumper-7";


	//includes only packages required for the games main package (the package with the most structs/classes/enums)
	inline constexpr const bool bIncludeOnlyRelevantPackages = false;


	//No prefix for files -> FilePrefix = nullptr
	inline constexpr const char* FilePrefix = nullptr;

	//No seperate namespace for SDK -> SDKNamespaceName = nullptr
	inline constexpr const char* SDKNamespaceName = "SDK";

	//No seperate namespace for Params -> ParamNamespaceName = nullptr
	inline constexpr const char* ParamNamespaceName = "Params";

	//Do not XOR strings -> XORString = nullptr
	inline constexpr const char* XORString = nullptr;


	namespace Debug
	{
		inline constexpr bool bGenerateAssertionFile = false;
		inline constexpr bool bLimitAssertionsToEngienPackage = true;

		// Recommended
		inline constexpr bool bGenerateAssertionsForPredefinedMembers = true;
	}

	// Dont touch this
	namespace Internal
	{
		// UEEnum::Names
		inline bool bIsEnumNameOnly = false;

		inline bool bUseFProperty = false;

		inline bool bUseNamePool = false;

		inline bool bUseCasePreservingName = false;
		inline bool bUseUoutlineNumberName = false;

		inline bool bUseMaskForFieldOwner = false;

		inline std::string MainGamePackageName;
	}
}
