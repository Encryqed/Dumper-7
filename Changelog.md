## Dumper-7 Changelog
**Additions:**
- Added TUObjectArrayWrapper which automatically initializes GObjects the first time it's accessed
- Added predefined member **ULevel::Actors** to the SDK
- Added FieldPathProperty support (TFieldPath)
- Added OptionalProperty support (TOptional)
- Added DelegateProperty support (TDelegate)
- Added FName::ToString as a second fallback to FName::AppendString if GNames couldn't be found

**Modifications:**
- Static functions are now using the `static` keyword in the SDK and are called on their default-object by default
- Const functions are now using the `const` keyword to further indicate what the function does

**Fixes:**
- Fixed name-collisions between classes/structs, between enums, between members/functions, and between packages
- Fixed cyclic dependencies
- Fixed incorrect size/alignment on classes
- Fixed incorrect member offsets
- Fixed TWeakObjectPtr (test for TagAtLastTest)
- Fixed invalid folder names on some games
- Fixed Mapping generation
- Fixed StaticClass for BlueprintGeneratedClass (untested)
