project.Configuration("ConfigurationType", "DynamicLibrary")
project.Configuration("CharacterSet", "MultiByte")
project.CompilerOption("PreprocessorDefinitions").Remove(["UNICODE","_UNICODE"])
project.CompilerOption("CallingConvention", "Cdecl")
project.CompilerOption("RuntimeTypeInfo", "true", "Release|*")
project.LinkerOption("GenerateDebugInformation", "DebugFull", "Release|*")
project.CompilerOption("DebugInformationFormat", "ProgramDatabase", "Release|*")
project.CompilerOption("OmitFramePointers", "false", "Release|*")

project.CompilerOption("PreprocessorDefinitions", ["concept=_concept","module=_module"])

ExecuteScript([
	os.path.join(solution.root, "projects/detours_inc.py"),
	os.path.join(solution.root, "projects/asmjit_inc.py"),
], {**globals()})

project.Files([
	"src/**",
	("src/sdk_hack/**", "sdk_hack"),
	("src/sdk_hack/NextBot/**", "sdk_hack/NextBot"),
	("src/CustomEntities/**", "CustomEntities"),
	("src/entities/**", "entities"),

	([	"src/smsdk_config.h",
		os.path.join(args.smpath, "public/smsdk_ext.cpp"),
		os.path.join(args.smpath, "public/smsdk_ext.h"),
	], "smsdk"),
])

project.Includes([
	"src/**",
	"thirdparty",

	os.path.join(args.smpath, "sourcepawn/include"),
	os.path.join(args.smpath, "public"),
	#os.path.join(args.smpath, "public/asm"),
	#os.path.join(args.smpath, "public/jit"),
	#os.path.join(args.smpath, "public/libudis86"),
	os.path.join(args.smpath, "public/amtl"),
	os.path.join(args.smpath, "public/amtl/amtl"),
	os.path.join(args.smpath, "public/extensions"),
	#os.path.join(args.smpath, "public/CDetour"),

	os.path.join(args.mmpath, "public"),
	os.path.join(args.mmpath, "core"),
	os.path.join(args.mmpath, "core/sourcehook"),
])

project.LinkerOption("AdditionalDependencies", [
	"legacy_stdio_definitions.lib",
	"tier0.lib","tier1.lib",
	"mathlib.lib","raytrace.lib",
	"vstdlib.lib",
])
project.LinkerOption("IgnoreSpecificDefaultLibraries", ["libcmt"])

project.CompilerOption("PreprocessorDefinitions", ["PLATFORM_WINDOWS","SERVER_DLL","GAME_DLL","NEXT_BOT","NEXTBOT"])

pch = os.path.join(solution.root, "src/pch.h")
project.CompilerOption("PrecompiledHeader", "Use")
project.CompilerOption("PrecompiledHeaderFile", pch)
project.CompilerOption("ForcedIncludeFiles", pch)
project.Files(pch[:-2]+".cpp")[0].Option("PrecompiledHeader", "Create")

for sdk in [
	("EPISODEONE","1"),
	("DARKMESSIAH","2"),
	("ORANGEBOX","3"),
	("BLOODYGOODTIME","4"),
	("EYE","5"),
	("CSS","6"),
	("HL2DM","7"),
	("DODS","8"),
	("SDK2013","9"),
	("BMS","10"),
	("TF2","11"),
	("LEFT4DEAD","12"),
	("NUCLEARDAWN","13"),
	("CONTAGION","14"),
	("LEFT4DEAD2","15"),
	("ALIENSWARM","16"),
	("PORTAL2","17"),
	("BLADE","18"),
	("INSURGENCY","19"),
	("DOI","20"),
	("CSGO","21"),
]:
	project.CompilerOption("PreprocessorDefinitions", ["SE_"+sdk[0]+"="+sdk[1]])

project.CompilerOption("DisableSpecificWarnings", [
	"4265","4619","4242","4365","4668",
	"4244","4826","4946","4189","4191",
	"4826","4458","4459","4706","4018",
	"4211","4101","4061","4471","4005",
	"4263","4264","4266","4099","5033",
	"4555","4464","5038","4062","4838",
	"4456","4457","4150","4612","4373",
	"4121","4820","4514","4710","4625",
	"4623","5027","5045","4626","4100",
	"5026","4774",
])

project.CompilerOption("PreprocessorDefinitions", ["_CRT_SECURE_NO_WARNINGS","DBGFLAG_H","RAD_TELEMETRY_DISABLED"])