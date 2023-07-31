
# Dumper 7

SDK Generator for all Unreal Engine games. Supported versions are all of UE4 and UE5.

## How to use

- Compile the dll in x64-Release
- Inject the dll into your target game
- The SDK is generated into the path specified by `Settings::SDKGenerationPath`
## Overriding Offsets

- ### Only override any offsets if the generator doesn't find them by itself
- All overrides are made in **Generator::Init()** inside of **Generator.cpp**

- GObjects
  ```cpp
  ObjectArray::Init(/*GObjectsOffset*/, /*ChunkSize*/, /*bIsChunked*/);
  ```
  ```cpp
  ObjectArray::DecryptPtr = [](void* Objptr) -> uint8_t* { return (uint8_t*)(uint64_t(Objptr) ^ 0x8375); };
  ```
- FName::AppendString
  ```cpp
  FName::Init(/*FName::AppendStringOffset*/);
  ```
- ProcessEvent
  ```cpp
  Off::InSDK::InitPE(/*PEIndex*/);
  ```
## Notes

- Be aware, that the calls to ProcessEvent in `Main.cpp` may crash on debug configuration
- In the SDK, initialize `UObject::GObjects` by calling the function `SDK::InitGObjects()`
- In the SDK, functions from classes as `UKismetSystemLibrary` or `UGameplayStatics` may only be called on their default-objects
## Issues

If you have any issues using the Dumper, please create an Issue on this repository\
and explain the problem **in detail**.
## TODO

- Fix mapping-file generation
- Fix cyclic dependencies of packages
