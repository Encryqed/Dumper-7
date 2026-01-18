#include "Settings.h"

#include <Windows.h>
#include <filesystem>
#include <string>

#include "Unreal/UnrealObjects.h"
#include "Unreal/ObjectArray.h"

void Settings::InitWeakObjectPtrSettings()
{
	const UEStruct LoadAsset = ObjectArray::FindObjectFast<UEFunction>("LoadAsset", EClassCastFlags::Function);

	if (!LoadAsset)
	{
		std::cerr << "\nDumper-7: 'LoadAsset' wasn't found, could not determine value for 'bIsWeakObjectPtrWithoutTag'!\n" << std::endl;
		return;
	}

	const UEProperty Asset = LoadAsset.FindMember("Asset", EClassCastFlags::SoftObjectProperty);
	if (!Asset)
	{
		std::cerr << "\nDumper-7: 'Asset' wasn't found, could not determine value for 'bIsWeakObjectPtrWithoutTag'!\n" << std::endl;
		return;
	}

	const UEStruct SoftObjectPath = ObjectArray::FindStructFast("SoftObjectPath");

	constexpr int32 SizeOfFFWeakObjectPtr = 0x08;
	constexpr int32 OldUnrealAssetPtrSize = 0x10;
	const int32 SizeOfSoftObjectPath = SoftObjectPath ? SoftObjectPath.GetStructSize() : OldUnrealAssetPtrSize;

	Settings::Internal::bIsWeakObjectPtrWithoutTag = Asset.GetSize() <= (SizeOfSoftObjectPath + SizeOfFFWeakObjectPtr);

	//std::cerr << std::format("\nDumper-7: bIsWeakObjectPtrWithoutTag = {}\n", Settings::Internal::bIsWeakObjectPtrWithoutTag) << std::endl;
}

void Settings::InitLargeWorldCoordinateSettings()
{
	const UEStruct FVectorStruct = ObjectArray::FindStructFast("Vector");

	if (!FVectorStruct) [[unlikely]]
	{
		std::cerr << "\nSomething went horribly wrong, FVector wasn't even found!\n\n" << std::endl;
		return;
	}

	const UEProperty XProperty = FVectorStruct.FindMember("X");

	if (!XProperty) [[unlikely]]
	{
		std::cerr << "\nSomething went horribly wrong, FVector::X wasn't even found!\n\n" << std::endl;
		return;
	}

		/* Check the underlaying type of FVector::X. If it's double we're on UE5.0, or higher, and using large world coordinates. */
	Settings::Internal::bUseLargeWorldCoordinates = XProperty.IsA(EClassCastFlags::DoubleProperty);

	//std::cerr << std::format("\nDumper-7: bUseLargeWorldCoordinates = {}\n", Settings::Internal::bUseLargeWorldCoordinates) << std::endl;
}

void Settings::InitObjectPtrPropertySettings()
{
	const UEClass ObjectPtrPropertyClass = ObjectArray::FindClassFast("ObjectPtrProperty");

	if (!ObjectPtrPropertyClass)
	{
		// The class doesn't exist, this so FieldPathProperty couldn't have been replaced with ObjectPtrProperty
		std::cerr << std::format("\nDumper-7: bIsObjPtrInsteadOfFieldPathProperty = {}\n", Settings::Internal::bIsObjPtrInsteadOfFieldPathProperty) << std::endl;
		Settings::Internal::bIsObjPtrInsteadOfFieldPathProperty = false;
		return;
	}

	Settings::Internal::bIsObjPtrInsteadOfFieldPathProperty = ObjectPtrPropertyClass.GetDefaultObject().IsA(EClassCastFlags::FieldPathProperty);

	std::cerr << std::format("\nDumper-7: bIsObjPtrInsteadOfFieldPathProperty = {}\n", Settings::Internal::bIsObjPtrInsteadOfFieldPathProperty) << std::endl;
}

void Settings::InitArrayDimSizeSettings()
{
	/*
	 * UEProperty::GetArrayDim() is already fully functional at this point.
	 *
	 * This setting is just there to stop it from returning (int32)0xFFFFFF01 when it should be just (uint8)0x01.
	*/
	for (const UEObject Obj : ObjectArray())
	{
		if (!Obj.IsA(EClassCastFlags::Struct))
			continue;

		const UEStruct AsStruct = Obj.Cast<UEStruct>();

		for (const UEProperty Property : AsStruct.GetProperties())
		{
			// This number should just be 0x1 to indicate it's a single element, but the upper bytes aren't cleared to zero
			if (Property.GetArrayDim() >= 0x000F0001)
			{
				Settings::Internal::bUseUint8ArrayDim = true;
				std::cerr << std::format("\nDumper-7: bUseUint8ArrayDim = {}\n", Settings::Internal::bUseUint8ArrayDim) << std::endl;
				return;
			}
		}
	}

	Settings::Internal::bUseUint8ArrayDim = false;
	std::cerr << std::format("\nDumper-7: bUseUint8ArrayDim = {}\n", Settings::Internal::bUseUint8ArrayDim) << std::endl;
}

void Settings::Config::Load()
{
	namespace fs = std::filesystem;

	// Try local Dumper-7.ini
	const std::string LocalPath = (fs::current_path() / "Dumper-7.ini").string();
	const char* ConfigPath = nullptr;

	if (fs::exists(LocalPath)) 
	{
		ConfigPath = LocalPath.c_str();
	}
	else if (fs::exists(GlobalConfigPath)) // Try global path
	{
		ConfigPath = GlobalConfigPath;
	}

	// If no config found, use defaults
	if (!ConfigPath) 
	{
		return;
	}

	char SDKNamespace[256] = {};
	GetPrivateProfileStringA("Settings", "SDKNamespaceName", "SDK", SDKNamespace, sizeof(SDKNamespace), ConfigPath);

	SDKNamespaceName = SDKNamespace;

	if (strcmp(SDKNamespace, "SDK") != 0)
	{
		std::cerr << "Using custom namespace: " << SDKNamespaceName << "\n";
	}

	// Check for output path override
	char SDKPath[256] = {};
	GetPrivateProfileStringA("Settings", "SDKGenerationPath", "C:/Dumper-7", SDKPath, sizeof(SDKPath), ConfigPath);

	// Path issues are generally resolved during generation but its better to catch during config load
	if (strcmp(SDKPath, "C:/Dumper-7") != 0)
	{
		try
		{
			auto UserSDKPath = fs::path(SDKPath);

			// This can fail in some cases, e.g. a symbolic link to a non-existent directory can still exist
			if (!fs::exists(UserSDKPath))
			{
				// Try to create the outer path for the dump if it doesn't exist
				fs::create_directories(UserSDKPath);
			}

			std::error_code ec;
			// Try to canonicalize the input which actually opens the path to resolve any link or permission issues
			auto SDKCanonicalPath = fs::canonical(UserSDKPath, ec);
			if (!ec) 
			{
				// Report the absolute path to the user
				std::cerr << "SDK Generation Path: " << SDKCanonicalPath.string().c_str() << "\n";
				// Only actually set the SDKGenerationPath if it is valid
				Settings::Generator::SDKGenerationPath = SDKCanonicalPath.string();
			}
		}
		catch (const std::filesystem::filesystem_error& fe)
		{
			std::cerr << "Invalid SDK Generation Path: \n";
			std::cerr << fe.what() << std::endl;
			std::cerr << "Falling back to default path: C:/Dumper-7 \n";
		}
	}
	
	SleepTimeout = max(GetPrivateProfileIntA("Settings", "SleepTimeout", 0, ConfigPath), 0);

	if (SleepTimeout > 0) 
	{
		// Convert seconds to ms automatically if a value under 1 second is set
		if (SleepTimeout < 1000) 
		{
			SleepTimeout *= 1000;
		}

		std::cerr << "Sleep Timeout: " << std::dec << SleepTimeout << "ms" << "\n";
	}

	// DumpKey takes VK Scancodes as integers
	DumpKey = GetPrivateProfileIntA ( "Settings", "DumpKey", 0, ConfigPath);

	if (DumpKey != 0) 
	{
		// Map the virtual key and print the actual name
		char KeyName[256] = {};
		LONG lParamValue = MapVirtualKeyA(DumpKey, MAPVK_VK_TO_VSC) << 16;
		if (GetKeyNameTextA(lParamValue, KeyName, sizeof(KeyName)) != 0) 
		{
			std::cerr << "Press " << KeyName << " to begin dump." << "\n";
		}
	}
}
