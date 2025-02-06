set_project("Dumper-7")
add_rules("mode.debug", "mode.release")
set_languages("c++latest")

target("Dumper-7")
    set_kind("shared")
    
    add_files("Dumper/**.cpp")
    add_files("Dumper/**.c")

    add_cxflags("/wd4244", "/wd4267")

    add_syslinks("User32", "Ntdll")

    set_targetdir("Build/$(mode)/")
