project.Includes("thirdparty/Detours/src")
project.Depends(solution.Project("Detours"))

project.CompilerOption("PreprocessorDefinitions", "DETOURS_DONT_REMOVE_SAL_20")
project.CompilerOption("PreprocessorDefinitions", "DETOUR_DEBUG", "Debug|*")

project.LinkerOption("AdditionalDependencies", "Detours.lib")
project.LinkerOption("AdditionalLibraryDirectories", "$(SolutionDir)build\\Detours\\$(PlatformTarget)\\$(Configuration)\\")