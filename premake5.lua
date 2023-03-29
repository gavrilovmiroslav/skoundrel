workspace "skoundrel"
	configurations { "Debug", "Release" }

	project "Skoundrel"
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++17"
		targetdir "bin/%{cfg.buildcfg}"

		externalincludedirs {
			"$(SolutionDir)entt/single_include/",
			"$(SolutionDir)glfw/include/",
		}

		includedirs {
			"src/"
		}
		
		files {
			"src/**.h", "src/**.cpp"
		}

		links { "glfw3" }

		filter "configurations:Debug"
			defines { "DEBUG" }
			symbols "On"
			libdirs { "./glfw/build/src/Debug" }

		filter "configurations:Release"
			defines { "NDEBUG" }
			optimize "On"
			libdirs { "./glfw/build/src/Release" } -- TODO: change later
