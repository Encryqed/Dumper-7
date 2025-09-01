#pragma once

#include <string>

#include "Unreal/Enums.h"

namespace Settings
{
	constexpr bool Is32Bit()
	{
#if defined(_WIN64)
		return false;
#elif defined(_WIN32)
		return true;
#endif
	}
  
	inline constexpr const char* GlobalConfigPath = "C:/Dumper-7/Dumper-7.ini";

	namespace Config
	{
		inline int SleepTimeout = 0;
		inline std::string SDKNamespaceName = "SDK";

		void Load();
	};

	namespace EngineCore
	{
		/* A special setting to fix UEnum::Names where the type is sometimes TArray<FName> and sometimes TArray<TPair<FName, Some8BitData>> */
		constexpr bool bCheckEnumNamesInUEnum = true;

		/* Enables support for TEncryptedObjectProperty */
		constexpr bool bEnableEncryptedObjectPropertySupport = false;
	}

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

		/* No seperate namespace for Params -> ParamNamespaceName = nullptr */
		constexpr const char* ParamNamespaceName = "Params";

		/* XOR function name, that will be wrapped around any generated string. e.g. "xorstr_" -> xorstr_("Pawn") etc. */
		constexpr const char* XORString = nullptr;
		/* XOR header file name. e.g. "xorstr.hpp" */
		constexpr const char* XORStringInclude = nullptr;

		/* Customizable part of Cpp code to allow for a custom 'uintptr_t InSDKUtils::GetImageBase()' function */
		constexpr const char* GetImageBaseFuncBody = 
R"({
	return reinterpret_cast<uintptr_t>(GetModuleHandle(0));
}
)";
		/* Customizable part of Cpp code to allow for a custom 'InSDKUtils::CallGameFunction' function */
		constexpr const char* CallGameFunction =
R"(
	template<typename FuncType, typename... ParamTypes>
	requires std::invocable<FuncType, ParamTypes...>
	inline auto CallGameFunction(FuncType Function, ParamTypes&&... Args)
	{
		return Function(std::forward<ParamTypes>(Args)...);
	}
)";
		/* An option to force the UWorld::GetWorld() function in the SDK to get the world through an instance of UEngine. Useful for games on which the dumper finds the wrong GWorld offset. */
		constexpr bool bForceNoGWorldInSDK = false;

		/* This will allow the user to manually initialize global variable addresses in the SDK (eg. GObjects, GNames, AppendString). */
		constexpr bool bAddManualOverrideOptions = true;

		/* Adds the 'final' specifier to classes with no loaded child class at SDK-generation time. */
		constexpr bool bAddFinalSpecifier = true;
	}

	namespace MappingGenerator
	{
		/* Whether the MappingGenerator should check if a name was written to the nametable before. Exists to reduce mapping size. */
		constexpr bool bShouldCheckForDuplicatedNames = true;

		/* Whether EditorOnly should be excluded from the mapping file. */
		constexpr bool bExcludeEditorOnlyProperties = true;

		/* Which compression method to use when generating the file. */
		constexpr EUsmapCompressionMethod CompressionMethod = EUsmapCompressionMethod::ZStandard;
	}

	/* Partially implemented  */
	namespace Debug
	{
		/* Generates a dedicated file defining macros for static asserts (Make sure InlineAssertions are off) */
		inline constexpr bool bGenerateAssertionFile = true;

		/* Prefix for assertion macros in assertion file. Example for "MyPackage_params.hpp": #define DUMPER7_ASSERTS_PARAMS_MyPackage */
		inline constexpr const char* AssertionMacroPrefix = "DUMPER7_ASSERTS_";


		/* Adds static_assert for struct-size, as well as struct-alignment */
		inline constexpr bool bGenerateInlineAssertionsForStructSize = false;

		/* Adds static_assert for member-offsets */
		inline constexpr bool bGenerateInlineAssertionsForStructMembers = false;


		/* Prints debug information during Mapping-Generation */
		inline constexpr bool bShouldPrintMappingDebugData = false;
	}

	//* * * * * * * * * * * * * * * * * * * * *// 
	// Do **NOT** change any of these settings //
	//* * * * * * * * * * * * * * * * * * * * *//
	namespace Internal
	{
		/* Whether UEnum::Names stores only the name of the enum value, or a Pair<Name, Value> */
		inline bool bIsEnumNameOnly = false; // EDemoPlayFailure

		/* Whether the 'Value' component in the Pair<Name, Value> UEnum::Names is a uint8 value, rather than the default int64 */
		inline bool bIsSmallEnumValue = false;

		/* Whether TWeakObjectPtr contains 'TagAtLastTest' */
		inline bool bIsWeakObjectPtrWithoutTag = false;

		/* Whether this games' engine version uses FProperty rather than UProperty */
		inline bool bUseFProperty = false;

		/* Whether this game's engine version uses FNamePool rather than TNameEntryArray */
		inline bool bUseNamePool = false;

		/* Whether UObject::Name or UObject::Class is first. Affects the calculation of the size of FName in fixup code. Not used after Off::Init(); */
		inline bool bIsObjectNameBeforeClass = false;

		/* Whether this games uses case-sensitive FNames, adding int32 DisplayIndex to FName */
		inline bool bUseCasePreservingName = false;

		/* Whether this games uses FNameOutlineNumber, moving the 'Number' component from FName into FNameEntry inside of FNamePool */
		inline bool bUseOutlineNumberName = false;

		/* Whether this game uses the 'FFieldPathProperty' cast flags for a custom property 'FObjectPtrProperty' */
		inline bool bIsObjPtrInsteadOfFieldPathProperty = false;

		/* Whether this games' engine version uses a contexpr flag to determine whether a FFieldVariant holds a UObject* or FField* */
		inline bool bUseMaskForFieldOwner = false;

		/* Whether this games' engine version uses double for FVector, instead of float. Aka, whether the engine version is UE5.0 or higher. */
		inline bool bUseLargeWorldCoordinates = false;

		/* Whether this game uses uint8 for UEProperty::ArrayDim, instead of int32 */
		inline bool bUseUint8ArrayDim = false;
	}

	extern void InitWeakObjectPtrSettings();
	extern void InitLargeWorldCoordinateSettings();

	extern void InitObjectPtrPropertySettings();
	extern void InitArrayDimSizeSettings();
}
