#pragma once

#include "PredefinedMembers.h"

/*
* Initializes predefined members for core engine types shared between CppGenerator and IDAMappingV2Generator
* Registers members for UObject, UField, UEnum, UStruct, UFunction, UClass, ULevel, UDataTable
*/
void InitCorePredefinedMembers(PredefinedMemberLookupMapType& OutMembers);
