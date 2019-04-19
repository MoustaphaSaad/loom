project "unittest"
	language "C++"
	kind "ConsoleApp"

	files
	{
		"unittest_main.cpp",
		"unittest_loom.cpp"
	}

	includedirs
	{
		"doctest",
		"%{mn}/include",
		"../include"
	}

	links
	{
		"mn",
		"loom"
	}

	cppdialect "c++17"
	systemversion "latest"

	filter "system:linux"
		defines { "OS_LINUX" }
		linkoptions {"-pthread"}

	filter { "system:linux", "configurations:debug" }
		linkoptions {"-rdynamic"}

	filter "system:windows"
		defines { "OS_WINDOWS" }
		buildoptions {"/utf-8"}

	filter { "system:windows", "configurations:debug" }
		links {"dbghelp"}