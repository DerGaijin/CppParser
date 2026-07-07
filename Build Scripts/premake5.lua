workspace "CppParser"
    location ".."
    configurations { "Debug", "Release" }

project "CppParser"
    location "../CppParser"
    kind "ConsoleApp"
    language "C++"
    cppdialect "c++17"
    targetdir "../Bin/%{cfg.buildcfg}"
    objdir "../Bin-Int/%{cfg.buildcfg}"

    files {
        "../CppParser/**.cpp",
        "../CppParser/**.h",
        "../CppParser/**.hpp",
    }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

-- Module includes will be placed here
