project.Includes([
	hl2sdk,
	os.path.join(hl2sdk, "public"),
	os.path.join(hl2sdk, "public/tier0"),
	os.path.join(hl2sdk, "public/tier1"),
	os.path.join(hl2sdk, "game"),
	os.path.join(hl2sdk, "game/server"),
	os.path.join(hl2sdk, "game/shared"),
])

project.LinkerOption("AdditionalLibraryDirectories", os.path.join(hl2sdk, "lib/public"))