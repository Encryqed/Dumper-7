#pragma once
#include <string>


namespace Settings
{
	namespace Generator
	{
		//Auto generated if no override is provided
		inline std::string GameName = "";
		inline std::string GameVersion = "";

		inline constexpr const char* SDKGenerationPath = "C:/Dumper-7";
	}

	namespace CppGenerator
	{
		/* No prefix for files->FilePrefix = "" */
		constexpr const char* FilePrefix = "";

		/* No seperate namespace for SDK -> SDKNamespaceName = nullptr */
		constexpr const char* SDKNamespaceName = "SDK";

		/* No seperate namespace for Params -> ParamNamespaceName = nullptr */
		constexpr const char* ParamNamespaceName = "Params";

		/* Feature is currently not supported/not working. */
		/* Do not XOR strings -> XORString = nullptr. Custom XorStr implementations differing from https://github.com/JustasMasiulis/xorstr may require changes to the struct 'StringLiteral' in CppGenerator.cpp.  */
		constexpr const char* XORString = nullptr;

		/* Customizable part of Cpp code to allow for a custom 'uintptr_t InSDKUtils::GetImageBase()' function */
		constexpr const char* GetImageBaseFuncBody = 
R"(	{
		return reinterpret_cast<uintptr_t>(GetModuleHandle(0));
	}
)";
		//Customizable part of Cpp code to allow for a custom 'InSDKUtils::CallGameFunction' function */
		constexpr const char* CallGameFunction =
R"(
	template<typename FuncType, typename... ParamTypes>
	requires std::invocable<FuncType, ParamTypes...>
	inline auto CallGameFunction(FuncType Function, ParamTypes&&... Args)
	{
		return Function(std::forward<ParamTypes>(Args)...);
	}
)";
		// This will allow the user to supply manual locations of various GOffset classes at runtime. */
		constexpr bool bAddManualOverrideOptions = true;
	}

	namespace MappingGenerator
	{
		/* Whether the MappingGenerator should check if a name was written to the nametable before. Exists to reduce mapping size. */
		constexpr bool bShouldCheckForDuplicatedNames = true;
	}

	/* Partially implemented  */
	namespace Debug
	{
		inline constexpr bool bGenerateAssertionFile = false;

		/* Adds static_assert for struct-size, as well as struct-alignment */
		inline constexpr bool bGenerateInlineAssertionsForStructSize = true;

		/* Adds static_assert for member-offsets */
		inline constexpr bool bGenerateInlineAssertionsForStructMembers = true;


		/* Prints debug information during Mapping-Generation */
		inline constexpr bool bShouldPrintMappingDebugData = false;
	}

	//* * * * * * * * * * * * * * * * * * * * *// 
	// Do **NOT** change any of these settings //
	//* * * * * * * * * * * * * * * * * * * * *//
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
	}
}
