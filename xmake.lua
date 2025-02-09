set_project("Dumper-7")
add_rules("mode.debug", "mode.release")
set_languages("c++latest", "clatest")

target("Dumper-7")
    set_kind("shared")
    
    add_files("Dumper/**.cpp")
    add_files("Dumper/**.c")

    add_cxflags("/wd4244", "/wd4267")

    add_links(
        "kernel32", "user32", "gdi32", "winspool", "comdlg32",
        "advapi32", "shell32", "ole32", "oleaut32", "uuid",
        "odbc32", "odbccp32", "ntdll"
    )

    if is_mode("release") then
        set_runtimes("MD")
    else
        set_runtimes("MDd")
    end

    set_targetdir("Build/" .. (is_mode("release") and "Release" or "Debug") .. "/")
