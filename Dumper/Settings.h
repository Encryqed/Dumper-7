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

	/* Do **NOT** change any of these settings */
	namespace Internal
	{
		// UEEnum::Names
		inline bool bIsEnumNameOnly = false;

		/* Whether TWeakObjectPtr contains 'TagAtLastTest' */
		inline bool bIsWeakObjectPtrWithoutTag = false;

		/* Whether this games' engine version uses FProperty rather than UProperty */
		inline bool bUseFProperty = false;

		/* Whether this games' engine version uses FNamePool rather than TNameEntryArray */
		inline bool bUseNamePool = false;


		/* Whether this games uses case-sensitive FNames, adding int32 DisplayIndex to FName */
		inline bool bUseCasePreservingName = false;

		/* Whether this games uses FNameOutlineNumber, moving the 'Number' component from FName into FNameEntry inside of FNamePool */
		inline bool bUseUoutlineNumberName = false;


		/* Whether this games' engine version uses a contexpr flag to determine whether a FFieldVariant holds a UObject* or FField* */
		inline bool bUseMaskForFieldOwner = false;

		inline std::string MainGamePackageName;
	}
}
