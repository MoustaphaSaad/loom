require "export-compile-commands"

newoption {
	trigger		= "asan",
	description = "enables address sanitizer for leaks and bounds checking"
}

newoption {
	trigger		= "tsan",
	description = "enables thread sanitizer for race conditions"
}

mn = path.getabsolute("mn")
workspace "loom"
	configurations {"debug", "release"}
	platforms {"x86", "x64"}
	targetdir "bin/%{cfg.platform}/%{cfg.buildcfg}/"
	startproject "unittest"
	defaultplatform "x64"

	if _ACTION then
		location ("projects/" .. _ACTION)
	end

	-- these two flags are useful but incompatible so turn one on when you need it
	-- address: for leaks and bounds overrun
	if _OPTIONS["asan"] then
		buildoptions {"-fsanitize=address"}
		linkoptions {"-fsanitize=address"}
	-- thread: for race conditions
	elseif _OPTIONS["tsan"] then
		buildoptions {"-fsanitize=thread"}
		linkoptions {"-fsanitize=thread"}
	end

	include "mn/mn"
	include "loom"
	include "unittest"