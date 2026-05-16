workspace "SilicaWorkspace"
	architecture "x64"
	startproject "SilicaDemo"
	configurations {
		"Debug",
		"Release",
		"Distribution"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Silica"
include "SilicaDemo"
