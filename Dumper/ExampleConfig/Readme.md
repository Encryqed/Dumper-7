# Using the optional configuration settings

Your configuration file must always be named Dumper-7.ini and can be placed in one or more of three locations 
1. Next to the game executable
2. Next to the Dumper-7 dll (which does not need to be named dumper-7)
3. In the global path `C:/Dumper-7/Dumper-7.ini`
These locations are checked in order with only the first config found being considered so you can have game specific overrides

## Writing the config
- You must include the section header for options to be read
- You can exclude any setting to use the default option or comment anything out with # 
```ini
[Settings]
# SleepTimeout
# DumpKey
# SDKGenerationPath
# SDKNameSpaceName
```

## SleepTimeout
- The dump will occur automatically after the specified timeout duration, useful if loading at startup
- Values above 1000 will be treated as milliseconds, values below 1000 will be treated as seconds
- If you need a delay less than one second you could set a dump key
- Disable this by excluding it or setting to 0. If multiple values exist in one config only the highest is taken
  
```ini
# Sets a timeout of 5.5 seconds
SleepTimeout=5500
# Sets a timeout of 30 seconds
SleepTimeout=30
```
## DumpKey
- You can also specify a key to trigger the dump. If this is set in addition to a timeout then its a matter of which occurs first
- If you have set a dump key then the same key will unload the DLL after the dump finishes (otherwise it will be F6 to unload)
- Use the int value for the [virtual key code](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes) you want, This can be in hex or decimal, e.g. 0x77 or 119 is F8
- The Dumper-7 console will tell you the actual Key you've set
- You can also activate this with the console focused, e.g. if the game window is frozen
- Disable this by excluding it or setting to 0, invalid values like strings are ignored
```ini
# Sets the dump key to F8
DumpKey=0x74
```
## SDKGenerationPath
- Normally dumps will be stored in C:/Dumper-7 but you can specify your own path
- You can enter a full path including the drive, otherwise it will be considered relative to the game directory
- You can access parent paths with ../
```ini
# Saves the SDK next to the Binaries, Content, Saved folders in the game path
SDKGenerationPath=../../SDK
```
## SDKNameSpaceName
- Changes the namespace used in the generated classes
```ini
# Generate code under the sdk namespace instead of SDK, e.g. sdk::APlayerController 
SDKNamespaceName=sdk
```
