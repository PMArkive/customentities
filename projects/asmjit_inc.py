project.Includes("thirdparty/asmjit/src")
project.Depends(solution.Project("AsmJit"))

project.CompilerOption("PreprocessorDefinitions", ["ASMJIT_STATIC","ASMJIT_BUILD_X86"])
project.CompilerOption("PreprocessorDefinitions", "ASMJIT_DEBUG", "Debug|*")
project.CompilerOption("PreprocessorDefinitions", ["ASMJIT_RELEASE","ASMJIT_DISABLE_LOGGING","ASMJIT_DISABLE_TEXT","ASMJIT_DISABLE_VALIDATION"], "Release|*")

project.LinkerOption("AdditionalDependencies", "AsmJit.lib")
project.LinkerOption("AdditionalLibraryDirectories", "$(SolutionDir)build\\AsmJit\\$(PlatformTarget)\\$(Configuration)\\")