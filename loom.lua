project "loom"
	language "C++"
	kind "SharedLib"

	files
	{
		"include/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"include",
		"%{mn}/include"
	}

	links
	{
		"mn"
	}

	--language configuration
	warnings "Extra"
	cppdialect "c++17"
	systemversion "latest"

	--linux configuration
	filter "system:linux"
		defines { "OS_LINUX" }
		linkoptions {"-pthread"}

	--windows configuration
	filter "system:windows"
		defines { "OS_WINDOWS" }

	--os agnostic configuration
	filter "configurations:debug"
		targetsuffix "d"
		defines
		{
			"DEBUG",
			"LOOM_DLL"
		}
		symbols "On"

	filter "configurations:release"
		defines
		{
			"NDEBUG",
			"LOOM_DLL"
		}
		optimize "On"

	filter "platforms:x86"
		architecture "x32"

	filter "platforms:x64"
		architecture "x64"