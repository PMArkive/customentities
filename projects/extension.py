import os, argparse

parser = argparse.ArgumentParser()
parser.add_argument("--sm-path", type=str, default="", dest="smpath")
parser.add_argument("--mm-path", type=str, default="", dest="mmpath")
parser.add_argument("--hl2sdk-root", type=str, default="", dest="hl2sdkroot")
args, unknown = parser.parse_known_args()

args.smpath = os.path.normpath(args.smpath)
args.mmpath = os.path.normpath(args.mmpath)
args.hl2sdkroot = os.path.normpath(args.hl2sdkroot)

solution = builder.Solution("extension", os.path.join(os.path.dirname(__file__), ".."))
solution.Configuration(["Debug","Release"])
solution.Platform(["x64",("x86","Win32")])

ExecuteScript([
	os.path.join(solution.root, "projects/detours.py"),
	os.path.join(solution.root, "projects/asmjit.py"),
], {**globals()})

for sdk in ["tf2","css"]:
	project = solution.Project("customentities.ext.2."+sdk, "MSVC")

	#hl2sdk = os.path.join(args.hl2sdkroot, "hl2sdk-"+sdk)
	hl2sdk = os.path.join(args.hl2sdkroot, "hl2sdk-sdk2013")

	project.CompilerOption("PreprocessorDefinitions", "SOURCE_ENGINE=SE_"+sdk.upper())

	ExecuteScript(os.path.join(solution.root, "projects/extension_base.py"), {**globals()})
	ExecuteScript(os.path.join(solution.root, "projects/extension_sdk.py"), {**globals()})
	ExecuteScript(os.path.join(solution.root, "projects/extension_{}.py".format(sdk)), {**globals()})