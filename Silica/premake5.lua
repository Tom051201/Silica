project "Silica"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "silicapch.h"
	pchsource "Silica/src/silicapch.cpp"

	files {
		"Silica/src/**.h",
		"Silica/src/**.cpp",
		"Silica/include/**.h"
	}
	
	includedirs {
		"Silica/include",
		"Silica/src"
	}

	filter { "files:Silica/src/vendor/**.cpp or Silica/backends/**.cpp" }
		enablepch "Off"
	
	filter "system:windows"
		systemversion "latest"
		defines {
			"SILICA_PLATFORM_WINDOWS"
		}

	filter "configurations:Debug"
		defines {
			"_DEBUG",
			"SILICA_DEBUG",
			"SILICA_ENABLE_ASSERTS"
		}
		runtime "Debug"
		symbols "on"
		targetsuffix "-Debug"

	filter "configurations:Release"
		defines {
			"NDEBUG",
			"SILICA_RELEASE"
		}
		runtime "Release"
		optimize "on"
		targetsuffix "-Release"
	
	filter "configurations:Distribution"
		defines {
			"NDEBUG",
			"SILICA_DISTRIBUTION"
		}
		runtime "Release"
		optimize "on"
		targetsuffix "-Distribution"
