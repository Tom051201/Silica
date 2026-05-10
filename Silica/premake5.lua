project "Silica"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "SilicaPCH.h"
	pchsource "Silica/src/SilicaPCH.cpp"
	
	files {
		"Silica/src/**.h",
		"Silica/src/**.cpp"
	}
	
	includedirs {
		"Silica/include",
		"Silica/src"
	}
	
	libdirs {}
	
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
	
	filter "configurations:Release"
		defines {
			"NDEBUG",
			"SILICA_RELEASE"
		}
		runtime "Release"
		optimize "on"
	
	filter "configurations:Distribution"
		defines {
			"NDEBUG",
			"SILICA_DISTRIBUTION"
		}
		runtime "Release"
		optimize "on"
