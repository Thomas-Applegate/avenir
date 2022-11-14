workspace "avenir"
	configurations {"debug", "release"}

project "avenir"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	targetdir "bin/%{cfg.buildcfg}"

	includedirs {"include/"}
	files {"include/**.h", "source/**.cpp"}

	filter "configurations:debug"
		defines {"AVENIR_DEBUG"}
		symbols "On"
		optimize "Debug"

	filter "configurations:release"
		defines {"AVENIR_NDEBUG"}
		optimize "On"
