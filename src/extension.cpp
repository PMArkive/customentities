#include "extension.h"
#include <CCustomEntityFactory.h>
#include <CDetour.h>
#include <filesystem.h>

Extension _extension{};
SMEXT_LINK(&_extension);

class CNavMesh;

IServerTools *servertools{nullptr};
IEntityFactoryDictionary *entityfactory{nullptr};
SourceHook::IHookManagerAutoGen *g_HMAGPtr{nullptr};
IGameConfig *mainconfig{nullptr};
CGlobalVars *gpGlobals{nullptr};
CBaseEntityList *g_pEntityList{nullptr};
IStaticPropMgrServer *staticpropmgr{nullptr};
IEngineTrace *enginetrace{nullptr};
IMDLCache *mdlcache{nullptr};
IVModelInfo *modelinfo{nullptr};
IUniformRandomStream *random{nullptr};
CNavMesh *TheNavMesh{nullptr};
ISpatialPartition *partition{nullptr};
IFileSystem *filesystem{nullptr};

class CCvar final
{
	public:
		static const FnCommandCallback_t GetCMDCallback(const ConCommand *const cmd) noexcept
		{
			return cmd->m_fnCommandCallback;
		}
};

bool Extension::SDK_OnLoad(char *const error, const size_t maxlen, const bool late) noexcept
{
	if(!gameconfs->LoadGameConfigFile("CustomEntities/CustomEntities", &mainconfig, error, maxlen))
		return false;

	const ConCommand *const nav_test_stairs{cvar->FindCommand("nav_test_stairs")};
	const FnCommandCallback_t callback{CCvar::GetCMDCallback(nav_test_stairs)};

	unsigned long old{0};
	VirtualProtect(callback, sizeof(callback), PAGE_EXECUTE_READWRITE, &old);

	int offset{0};
	mainconfig->GetOffset("TheNavMesh", &offset);
	TheNavMesh = MemUtils::GetVarAtOffset<CNavMesh *>(callback, offset);

	g_pEntityList = reinterpret_cast<CBaseEntityList *const>(gamehelpers->GetGlobalEntityList());

	ICustomEntityFactory::Initialize();
	ICustomEntityFactory::InstallAll();

	return true;
}

void Extension::SDK_OnUnload() noexcept
{
	ICustomEntityFactory::UninstallAll();
	ICustomEntityFactory::Shutdown();

	CDetour::RemoveAll();

	gameconfs->CloseGameConfigFile(mainconfig);
}

bool Extension::SDK_OnMetamodLoad(ISmmAPI *const ismm, char *const error, const size_t maxlen, const bool late) noexcept
{
	GET_V_IFACE_CURRENT(GetServerFactory, servertools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, staticpropmgr, IStaticPropMgrServer, INTERFACEVERSION_STATICPROPMGR_SERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, mdlcache, IMDLCache, MDLCACHE_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, modelinfo, IVModelInfo, VMODELINFO_SERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, partition, ISpatialPartition, INTERFACEVERSION_SPATIALPARTITION);
	GET_V_IFACE_CURRENT(GetEngineFactory, cvar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

	gpGlobals = ismm->GetCGlobals();
	entityfactory = servertools->GetEntityFactoryDictionary();
	g_HMAGPtr = static_cast<SourceHook::IHookManagerAutoGen *const>(ismm->MetaFactory(MMIFACE_SH_HOOKMANAUTOGEN, nullptr, nullptr));
	g_pSharedChangeInfo = engine->GetSharedEdictChangeInfo();

	ConVar_Register(FCVAR_GAMEDLL, this);

	return true;
}

bool Extension::RegisterConCommandBase(ConCommandBase *const pVar)
{
	cvar->RegisterConCommand(pVar);
	return true;
}

bool Extension::SDK_OnMetamodUnload(char *const error, const size_t maxlen) noexcept
{
	ConVar_Unregister();

	return true;
}