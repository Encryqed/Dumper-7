
# Dumper 7

SDK Generator for all Unreal Engine games. Supported versions are all of UE4 and UE5.

## How to use

- Compile the dll in x64-Release
- Inject the dll into your target game
- The SDK is generated into the path specified by `Settings::SDKGenerationPath`, by default this is `C:\\Dumper-7`
## Overriding Offsets

- ### Only override any offsets if the generator doesn't find them by itself
- All overrides are made in **Generator::Init()** inside of **Generator.cpp**

- GObjects
  ```cpp
  ObjectArray::Init(/*GObjectsOffset*/, /*ChunkSize*/, /*bIsChunked*/);
  ```
  ```cpp
  /* Make sure only to use types which exist in the sdk (eg. uint8, uint64) */
  InitObjectArrayDecryption([](void* ObjPtr) -> uint8* { return reinterpret_cast<uint8*>(uint64(ObjPtr) ^ 0x8375); });
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


## Issues

If you have any issues using the Dumper, please create an Issue on this repository\
and explain the problem **in detail**.

- Should your game be crashing, attach Visual Studios' debugger to the game and inject the Dumper-7.dll in debug-configuration.
Then include screenshots of the exception causing the crash, as well as a screenshot of the callstack.

- Should there be any compiler-errors in the SDK please send screenshots of them. Please note that **only build errors** are considered errors, as Intellisense often reports false positives.
Make sure to always send screenshots of the code causing the first error, as it's likely to cause a chain-reaction of one error causing hundreads.

- Should your own dll-project crash, verify your code thoroughly to make sure the error actually lies within the generated SDK.

## TODO
- Add support for IDA-Mappings generation to this branch
