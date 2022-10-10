# Dumper 7

SDK Generator for every Fortnite version from [1.2] (UE 4.16) -> [11.50] (UE 4.24) \
and also for some other games.


## Notes

* Make sure when you use the sdk to initialize it first by calling `SDK::InitGObjects()`
* Call functions from classes like `UKismetSystemLibrary` or `UGameplayStatics` only on their DefaultObject
* For some odd reason, you can't call ProcessEvent in Debug Mode, so if you compile in debug just comment out the ProcessEvent call.
## Issues

If you have any issues using the Dumper, please create a Issue on this repository\
and explain the problem in detail.
## TODO

- Adding UE 4.25 and higher Support (FProperty)
