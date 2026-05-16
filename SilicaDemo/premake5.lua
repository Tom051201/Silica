project "SilicaDemo"
	kind "WindowedApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")
	
	files {
		"SilicaDemo/src/**.h",
		"SilicaDemo/src/**.cpp",
		"%{wks.location}/Silica/Silica/backends/**.h",
		"%{wks.location}/Silica/Silica/backends/**.cpp"
	}
	
	includedirs {
		"%{wks.location}/Silica"
	}

	links {
		"Silica",
		"d3d12.lib",
		"dxgi.lib"
	}

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
