### Settings file can be next to the game binary, next to the dll, or in C:/Dumper-7 and will be prioritized in that order so game local files always override
### The file must always be named Dumper-7.ini but your dll can be named anything you want and this will still be found
[Settings]
### Autodump after the timeout even if you have a key set, leave on 0 or don't set it at all if you don't want this
### Anything less than 1000 will be taken as seconds, anything more than that as ms
SleepTimeout=0
### Set an int value corresponding to the virtual key code you want, can be in hex or decimal, e.g. 0x77 or 119 is F8
### You can set both a dump key and a timeout so you have the option to dump early and have a failsafe
### Again, leave it on 0 or don't set it to ignore
### https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
DumpKey=0x74
### Don't set anything if you want this to go to the C:/Dumper-7 folder
### If no drive is specified then it will be taken as a relative path
### You can access parent paths with ../
SDKGenerationPath=./
### Self explanatory
SDKNamespaceName=SDK