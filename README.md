
# Dumper 7

SDK Generator for all Unreal Engine games. Supported versions are all of UE4 and UE5.

## Notes

- Be aware, that the calls to ProcessEvent may crash on debug configuration
- In the SDK, initialize `UObject::GObjects` by calling the function `SDK::InitGObjects()`
- In the SDK, functions from classes as `UKismetSystemLibrary` or `UGameplayStatics` may only be called on their default-objects
## Issues

If you have any issues using the Dumper, please create an Issue on this repository\
and explain the problem **in detail**.
## TODO

- Fix mapping-file generation
- Fix cyclic dependencies of packages
