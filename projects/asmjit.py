project = solution.Project("AsmJit", "MSVC")

project.Files("thirdparty/asmjit/src/**")

project.CompilerOption("DisableSpecificWarnings", [
	"5031","4365","4464","4619","4458","4826",
	"4804","4242","4245","4456","4267","4838",
	"4456","4706","4702","5032",
])

ExecuteScript(os.path.join(solution.root, "projects/asmjit_inc.py"), {**globals()})