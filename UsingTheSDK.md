# Using the SDK to create a simple DLL project

## Visual Studio project setup
1. Create a new empty C++ project
2. Go to `Project>Properties`
3. Set `Configuration Type` from `Application (.exe)` to `Dynamic Library (.dll)`
4. In the same settings set the `C++ Language Standard` from `Default (ISO C++14 Standard)` to `Preview - Latest...`
5. Hit Apply and close the settings \
  ![image](https://github.com/Encryqed/Dumper-7/assets/64608145/e0170247-631d-466d-91c7-94a1c55b34a1)
7. Switch your build configuration from `x86` to `x64` \
   ![image](https://github.com/Encryqed/Dumper-7/assets/64608145/5f8963c1-4e55-4f3a-b080-26445d585c86)
8. Add a file `Main.cpp` to your project
9. Add a `DllMain` function and a `MainThread` function (See code [here](#code))

## Including the SDK into the project
1. Take the contents of your CppSDK folder (by default `C:\\Dumper-7\\GameName-GameVersion\\CppSDK`) \
   ![image](https://github.com/Encryqed/Dumper-7/assets/64608145/5a9404a7-1b49-4fd2-a3fa-a7467f18f39a)
2. Drop the contents into your VS projects' directory \
  ![image](https://github.com/Encryqed/Dumper-7/assets/64608145/14d4bb1b-8a23-43f8-8994-8bdae25af005)
3. If you do not care about your projects' compilation time, add `#include "SDK.hpp"` at the top of your `Main.cpp` file
4. If you **do** care, and you want faster compilation-times, directly include only the files you require. \
    Adding `#include "SDK/Engine_classes.hpp"` is a good start in this case.
5. Add `Basic.cpp` and `CoreUObject_functions.cpp` to your VS project
6. If you call a function from the SDK you need to add the .cpp file, that contains the function-body, to your project. \
   Example: \
   Calling `GetViewportSize()` from `APlayerController` requires you to add `Engine_functions.cpp` to your project. \
   ![image](https://github.com/Encryqed/Dumper-7/assets/64608145/c9ecf0c7-ec73-4e6a-8c6d-d7c86c26b5c8)
7. After attempting to build the SDK go to the "Error List" and make sure to select **`Build Only`**
  ![image](https://github.com/user-attachments/assets/cd72d55e-64de-4134-a115-6a9a0af80baa)
8. If there are any static_asserts failing, or other errors occuring, during building, read the [Issue](README.md#issues) part of the [ReadMe](README.md)

## Using the SDK
### 1. Retrieving instances of classes/structs to manipulate them
   - FindObject, used to find an object by its' name
     ```c++
     SDK::UObject* Obj1 = SDK::UObject::FindObject("ClassName PackageName.Outer1.Outer2.ObjectName");
     SDK::UObject* Obj2 = SDK::UObject::FindObjectFast("ObjectName");

     SDK::UObject* Obj3 = SDK::UObject::FindObjectFast("StructName", EClassCastFlags::Struct); // Finds a UStruct
     ```
   - StaticFunctions / GlobalVariables, used to retreive class-instances from static variables
     ```c++
     /* UWorld::GetWorld() replaces GWorld, no offset required */
     SDK::UWorld* World = SDK::UWorld::GetWorld();
     SDK::APlayerController* MyController = World->OwningGameInstance->LocalPlayers[0]->PlayerController;
     ```
### 2. Calling functions
  - Non-Static functions
    ```c++
    SDK::APlayerController* MyController = MagicFuncToGetPlayerController();
    
    float OutX, OutY;
    MyController->GetMousePosition(&OutX, &OutY);
    ```
  - Static functions
    ```c++
    /* static functions do not require an instance, they are automatically called using their DefaultObject */
    SDK::FName MyNewName = SDK::UKismetStringLibrary::Conv_StringToName(L"DemoNetDriver");
    ```
### 3. Checking a UObject's type
  - With EClassCastFlags
    ```c++
    /* Limited to base types, but is the fastest option */
    const bool bIsActor = Obj->IsA(EClassCastFlags::Actor);
    ```
  - With `UClass*`
    ```c++
    /* Ideal for native classes. Use `StaticName()` for Blueprint classes instead */
    const bool bIsSpecificActor = Obj->IsA(ASomeSpecificActor::StaticClass());
    ```
  - With `FName` (class name)
    ```c++
    /* Works for every class */
    const bool bIsSpecificActor = Obj->IsA(ASomeSpecificActor_C::StaticName());
    ```
### 4. Casting
  UnrealEngine heavily relies on inheritance and often uses pointers to a base class, which are later assigned addresses to \
  instances of child classes.
  ```c++
  if (MyController->Pawn->IsA(SDK::AGameSpecificPawn::StaticClass()))
  {
      SDK::AGameSpecificPawn* MyGamePawn = static_cast<SDK::AGameSpecificPawn*>(MyController->Pawn);
      MyGamePawn->GameSpecificVariable = 30; 
  }
  ```

## Code
### DllMain and MainThread
```c++
#include <Windows.h>
#include <iostream>

DWORD MainThread(HMODULE Module)
{
        /* Code to open a console window */
        AllocConsole();
        FILE* Dummy;
        freopen_s(&Dummy, "CONOUT$", "w", stdout);
        freopen_s(&Dummy, "CONIN$", "r", stdin);

        // Your code here

        return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
        switch (reason)
        {
                case DLL_PROCESS_ATTACH:
                CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
                break;
        }

        return TRUE;
}
```
### Example program that enables the UnrealEngine console
```c++
#include <Windows.h>
#include <iostream>

#include "SDK/Engine_classes.hpp"

// Basic.cpp was added to the VS project
// Engine_functions.cpp was added to the VS project

DWORD MainThread(HMODULE Module)
{
    /* Code to open a console window */
    AllocConsole();
    FILE* Dummy;
    freopen_s(&Dummy, "CONOUT$", "w", stdout);
    freopen_s(&Dummy, "CONIN$", "r", stdin);

    /* Functions returning "static" instances */
    SDK::UEngine* Engine = SDK::UEngine::GetEngine();
    SDK::UWorld* World = SDK::UWorld::GetWorld();

    /* Getting the PlayerController, World, OwningGameInstance, ... should all be checked not to be nullptr! */
    SDK::APlayerController* MyController = World->OwningGameInstance->LocalPlayers[0]->PlayerController;

    /* Print the full-name of an object ("ClassName PackageName.OptionalOuter.ObjectName") */
    std::cout << Engine->ConsoleClass->GetFullName() << std::endl;

    /* Manually iterating GObjects and printing the FullName of every UObject that is a Pawn (not recommended) */
    for (int i = 0; i < SDK::UObject::GObjects->Num(); i++)
    {
        SDK::UObject* Obj = SDK::UObject::GObjects->GetByIndex(i);

        if (!Obj)
            continue;

        if (Obj->IsDefaultObject())
            continue;

        /* Only the 'IsA' check using the cast flags is required, the other 'IsA' is redundant */
        if (Obj->IsA(SDK::APawn::StaticClass()) || Obj->HasTypeFlag(SDK::EClassCastFlags::Pawn))
        {
            std::cout << Obj->GetFullName() << "\n";
        }
    }

    /* You might need to loop all levels in UWorld::Levels */
    SDK::ULevel* Level = World->PersistentLevel;
    SDK::TArray<SDK::AActor*>& Actors = Level->Actors;

    for (SDK::AActor* Actor : Actors)
    {
        /* The 2nd and 3rd checks are equal, prefer using EClassCastFlags if available for your class. */
        if (!Actor || !Actor->IsA(SDK::EClassCastFlags::Pawn) || !Actor->IsA(SDK::APawn::StaticClass()))
            continue;

        SDK::APawn* Pawn = static_cast<SDK::APawn*>(Actor);
        // Use Pawn here
    }

    /* 
    * Changes the keyboard-key that's used to open the UE console
    * 
    * This is a rare case of a DefaultObjects' member-variables being changed.
    * By default you do not want to use the DefaultObject, this is a rare exception.
    */
    SDK::UInputSettings::GetDefaultObj()->ConsoleKeys[0].KeyName = SDK::UKismetStringLibrary::Conv_StringToName(L"F2");

    /* Creates a new UObject of class-type specified by Engine->ConsoleClass */
    SDK::UObject* NewObject = SDK::UGameplayStatics::SpawnObject(Engine->ConsoleClass, Engine->GameViewport);

    /* The Object we created is a subclass of UConsole, so this cast is **safe**. */
    Engine->GameViewport->ViewportConsole = static_cast<SDK::UConsole*>(NewObject);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
        break;
    }

    return TRUE;
}
```
