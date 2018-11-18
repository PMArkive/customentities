project.CompilerOption("PreprocessorDefinitions", "CSTRIKE_DLL")

basedir = os.path.normpath("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Source")
gamedir = os.path.join(basedir, "cstrike")

project.User().Property("LocalDebuggerWorkingDirectory", basedir)
project.User().Property("LocalDebuggerCommand", os.path.join(basedir, "srcds.exe"))
project.User().Property("LocalDebuggerCommandArguments", "-game cstrike -console +map de_dust2 +maxplayers 32")

project.PostBuildEvent().Property("Command", "copy /Y \"$(TargetPath)\" \"" + os.path.join(gamedir, "addons/sourcemod/extensions/$(TargetFileName)") + "\"")

project.Startup()