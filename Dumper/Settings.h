#pragma once


//#define WITH_CASE_PRESERVING_NAME

namespace Settings
{
	//Auto generated if no override is provided
	inline std::string GameName;
	inline std::string GameVersion;

	inline constexpr const char* SDKGenerationPath = "C:/Dumper-7";

	//use = nullptr if you don't want your files to be prefixed
	inline constexpr const char* FilePrefix = nullptr;


	inline constexpr const bool bUseNamespaceForParams = true;

	inline constexpr const bool bUseNamespaceForSDK = true;

	inline constexpr const bool bShouldXorStrings = false;


	inline constexpr const char* SDKNamespaceName = "SDK";

	inline constexpr const char* ParamNamespaceName = "Params";

	inline constexpr const char* XORString = "XORSTR";

	namespace Debug
	{
		inline constexpr bool bGenerateAssertionFile = false;
	}

	// Dont touch this
	namespace Internal
	{
		// UEEnum::Names
		// -1 -> TPair<FName, int64>
		//  0 -> FName
		// +1 -> TPair<FName, uint8>
		inline int8 EnumNameArrayType = -1;
	}
}
