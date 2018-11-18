project.CompilerOption("PreprocessorDefinitions", ["TF_DLL","GLOWS_ENABLE"])

basedir = os.path.normpath("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Team Fortress 2")
gamedir = os.path.join(basedir, "tf")

project.User().Property("LocalDebuggerWorkingDirectory", basedir)
project.User().Property("LocalDebuggerCommand", os.path.join(basedir, "srcds.exe"))
project.User().Property("LocalDebuggerCommandArguments", "-game tf -console +map mvm_decoy +maxplayers 32")

project.PostBuildEvent().Property("Command", "copy /Y \"$(TargetPath)\" \"" + os.path.join(gamedir, "addons/sourcemod/extensions/$(TargetFileName)") + "\"")