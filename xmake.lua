set_project("Dumper-7")
add_rules("mode.debug", "mode.release")
set_languages("c++latest", "clatest")

target("Dumper-7")
    set_kind("shared")

    add_files("Dumper/**.cpp")
    add_files("Dumper/**.c")

    add_includedirs("Dumper/Utils", {public = true})
    add_includedirs("Dumper/Engine/Public", {public = true})
    add_includedirs("Dumper/Generator/Public", {public = true})
    add_includedirs("Dumper/Platform/Public", {public = true})

    add_cxflags("/wd4244", "/wd4267", "/wd4369", "/wd4715")

    add_links(
        "kernel32", "user32", "gdi32", "winspool", "comdlg32",
        "advapi32", "shell32", "ole32", "oleaut32", "uuid",
        "odbc32", "odbccp32", "ntdll"
    )

    if is_mode("release") then
        set_runtimes("MD")
        set_targetdir("Bin/Release/")
        set_objectdir("Bin/Intermediates/Release/.objs")
        set_dependir("Bin/Intermediates/Release/.deps")
    else
        set_runtimes("MDd")
        set_targetdir("Bin/Debug/")
        set_objectdir("Bin/Intermediates/Debug/.objs")
        set_dependir("Bin/Intermediates/Debug/.deps")
    end