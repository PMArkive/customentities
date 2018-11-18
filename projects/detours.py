project = solution.Project("Detours", "MSVC")

project.Files("thirdparty/Detours/src/**")
for fil in project.Files(["thirdparty/Detours/src/Makefile","thirdparty/Detours/src/uimports.cpp"]):
	fil.Remove()

project.CompilerOption("PreprocessorDefinitions").Remove("DBGHELP_TRANSLATE_TCHAR")

project.CompilerOption("PreprocessorDefinitions", ["DETOUR_DEBUG=1"], "Debug|*")
project.CompilerOption("PreprocessorDefinitions", ["DETOUR_DEBUG=0"], "Release|*")

project.CompilerOption("DisableSpecificWarnings", [
	"4668","4365","4777","4826","4191","4477",
	"4474",
])

ExecuteScript(os.path.join(solution.root, "projects/detours_inc.py"), {**globals()})