workspace "skoundrel"
	configurations { "Debug", "Release" }

	project "Skoundrel"
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++17"
		targetdir "bin/%{cfg.buildcfg}"

		externalincludedirs {
			"$(SolutionDir)SDL/include/",
			"$(SolutionDir)entt/single_include/",
			"$(SolutionDir)libtcod/src/",			
		}

		includedirs {
			"src/"
		}
		
		files {
			"src/**.h", "src/**.cpp"
		}

		links { "SDL2d" }

		filter "configurations:Debug"
			defines { "DEBUG" }
			symbols "On"
			libdirs { "./SDL/build/Debug", "./wren/lib" }

		filter "configurations:Release"
			defines { "NDEBUG" }
			optimize "On"
			libdirs { "./SDL/build/Debug", "./wren/lib" } -- TODO: change later
