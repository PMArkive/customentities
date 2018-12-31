project = solution.Project("AsmJit", "MSVC")

project.Files("thirdparty/asmjit/src/**")

project.CompilerOption("PreprocessorDefinitions", ["concept=_concept","module=_module"])

project.LinkerAndLibrarianOption("AdditionalOptions", "/IGNORE:4221")

project.CompilerOption("DisableSpecificWarnings", [
	"5031","4365","4464","4619","4458","4826",
	"4804","4242","4245","4456","4267","4838",
	"4456","4706","4702","5032","4121","4820",
	"4514","4710","4625","4623","5027","5045",
	"4626","4100","5026","4774",
])

ExecuteScript(os.path.join(solution.root, "projects/asmjit_inc.py"), {**globals()})