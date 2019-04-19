require "export-compile-commands"

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

	include "mn/mn"
	include "loom"
	include "unittest"