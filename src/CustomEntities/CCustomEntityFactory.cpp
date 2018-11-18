#include "CCustomEntityFactory.h"
#include "CCustomBaseEntity.h"
#include <filesystem>
#include <activitylist.h>
#include <eventlist.h>

ICustomEntityFactory::ICustomEntityFactory(const char *const realclassname, const char *fakeclassname, const char *const classname) noexcept
	: IEntityFactory(), _realclassname{realclassname}, _fakeclassname{fakeclassname}, _classname{classname}
{ FactoryList().push_front(this); }

ICustomEntityFactory::FactoryList_t &ICustomEntityFactory::FactoryList() noexcept {
	static ICustomEntityFactory::FactoryList_t _list{};
	return _list;
}

void ICustomEntityFactory::Install() noexcept
{
	_realfactory = entityfactory->FindFactory(_realclassname);
	if(!_realfactory) {
		smutils->LogError(myself, "%s [%s] (%s) not found", _realclassname, _fakeclassname, _classname);
		return;
	}

	if(entityfactory->FindFactory(_fakeclassname)) {
		__EntityFactoryDictionary()->UninstallFactory(_realclassname);
		_replaced = true;
	}

	entityfactory->InstallFactory(this, _fakeclassname);

	_gameconf = CGameConfsManager::Instance().Load(_classname);

	if(_replaced)
		smutils->LogMessage(myself, "%s [%s] (%s) Replaced", _realclassname, _fakeclassname, _realclassname);
	else
		smutils->LogMessage(myself, "%s [%s] (%s) Installed", _fakeclassname, _classname, _realclassname);
}

void ICustomEntityFactory::Uninstall() noexcept
{
	__EntityFactoryDictionary()->UninstallFactory(_fakeclassname);

	if(_replaced)
		entityfactory->InstallFactory(_realfactory, _realclassname);

	CGameConfsManager::Instance().Unload(_classname);
	_gameconf = nullptr;
}

DETOUR_DECL_STATIC(void, __cdecl, RemoveEntity, CBaseEntity *const entity)
{
	CCustomBaseEntity *const custom{CCustomBaseEntity::Instance(entity)};
	if(custom) {
		custom->_delentity = false;
		custom->Delete();
	}

	DETOUR_STATIC_CALL(RemoveEntity, entity);
}

static class NextBotCombatCharacterFactory final : public IEntityFactory {
	public:
		void Initialize() noexcept
		{
			IEntityFactory *factory{entityfactory->FindFactory("simple_bot")};
			int offset{MemUtils::GetVFuncIndex(MemUtils::any_cast<const void *>(&IEntityFactory::Create))};
			const void *addr{MemUtils::GetVFunc(factory, offset)};
			mainconfig->GetOffset("CSimpleBot::CSimpleBot", &offset);
			addr = MemUtils::GetFuncAtOffset(addr, offset);
			mainconfig->GetOffset("NextBotCombatCharacter::NextBotCombatCharacter", &offset);
			addr = MemUtils::GetFuncAtOffset(addr, offset);
			_ctor = addr;

			mainconfig->GetOffset("sizeof(NextBotCombatCharacter)", &offset);
			_size = offset;

			mainconfig->GetOffset("CBaseEntity::PostConstructor", &offset);
			_postidx = offset;

			entityfactory->InstallFactory(this, "nextbotcombatcharacter");
		}

		void Shutdown() noexcept
		{
			__EntityFactoryDictionary()->UninstallFactory("nextbotcombatcharacter");
		}

	private:
		virtual IServerNetworkable *Create(const char *const pClassName) noexcept override final
		{
			CBaseEntity *const entity{static_cast<CBaseEntity *const>(engine->PvAllocEntPrivateData(GetEntitySize()))};

			using Ctor_t = void (CBaseEntity::*)();
			(entity->*MemUtils::any_cast<Ctor_t>(_ctor))();

			const void *const postctor{MemUtils::GetVFunc(entity, _postidx)};
			using PostCtor_t = void (CBaseEntity::*)(const char *const);
			(entity->*MemUtils::any_cast<PostCtor_t>(postctor))(pClassName);

			return reinterpret_cast<IServerEntity *const>(entity)->GetNetworkable();
		}
		virtual void Destroy(IServerNetworkable *const pNetworkable) noexcept override final { pNetworkable->Release(); }
		virtual size_t GetEntitySize() noexcept override final { return _size; }

	private:
		const void *_ctor{nullptr};
		int _size{0};
		int _postidx{0};
} _nbcfactory{};


static class CombatCharacterFactory final : public IEntityFactory {
	public:
		void Initialize() noexcept
		{
			IEntityFactory *factory{entityfactory->FindFactory("physics_cannister")};
			int offset{MemUtils::GetVFuncIndex(MemUtils::any_cast<const void *>(&IEntityFactory::Create))};
			const void *addr{MemUtils::GetVFunc(factory, offset)};
			mainconfig->GetOffset("CPhysicsCannister::CPhysicsCannister", &offset);
			addr = MemUtils::GetFuncAtOffset(addr, offset);
			mainconfig->GetOffset("CBaseCombatCharacter::CBaseCombatCharacter", &offset);
			addr = MemUtils::GetFuncAtOffset(addr, offset);
			_ctor = addr;

			mainconfig->GetOffset("sizeof(CBaseCombatCharacter)", &offset);
			_size = offset;

			mainconfig->GetOffset("CBaseEntity::PostConstructor", &offset);
			_postidx = offset;

			entityfactory->InstallFactory(this, "combatcharacter");
		}

		void Shutdown() noexcept
		{
			__EntityFactoryDictionary()->UninstallFactory("combatcharacter");
		}

	private:
		virtual IServerNetworkable *Create(const char *const pClassName) noexcept override final
		{
			CBaseEntity *const entity{static_cast<CBaseEntity *const>(engine->PvAllocEntPrivateData(GetEntitySize()))};

			using Ctor_t = void (CBaseEntity::*)();
			(entity->*MemUtils::any_cast<Ctor_t>(_ctor))();

			const void *const postctor{MemUtils::GetVFunc(entity, _postidx)};
			using PostCtor_t = void (CBaseEntity::*)(const char *const);
			(entity->*MemUtils::any_cast<PostCtor_t>(postctor))(pClassName);

			return reinterpret_cast<IServerEntity *const>(entity)->GetNetworkable();
		}
		virtual void Destroy(IServerNetworkable *const pNetworkable) noexcept override final { pNetworkable->Release(); }
		virtual size_t GetEntitySize() noexcept override final { return _size; }

	private:
		const void *_ctor{nullptr};
		int _size{0};
		int _postidx{0};
} _ccfactory{};

static CDetour detRemoveEntity{};

void ICustomEntityFactory::Initialize() noexcept
{
	int offset{MemUtils::GetVFuncIndex(MemUtils::any_cast<const void *>(&IServerTools::RemoveEntity))};
	const void *addr{MemUtils::GetVFunc(servertools, offset)};
	mainconfig->GetOffset("UTIL_RemoveEntity", &offset);
	addr = MemUtils::GetFuncAtOffset(addr, offset);

	detRemoveEntity.DETOUR_STATIC(RemoveEntity, addr);

	_nbcfactory.Initialize();
	_ccfactory.Initialize();
}

void ICustomEntityFactory::Shutdown() noexcept
{
	_nbcfactory.Shutdown();
	_ccfactory.Shutdown();

	detRemoveEntity.Remove();
}

void ICustomEntityFactory::InstallAll() noexcept {
	const FactoryList_t &list{FactoryList()};
	for(ICustomEntityFactory *const it : list)
		it->Install();

	//!!TODO!! delete this
	ActivityList_Free();
	ActivityList_Init();
	ActivityList_RegisterSharedActivities();

	EventList_Free();
	EventList_Init();
	EventList_RegisterSharedEvents();
}

void ICustomEntityFactory::UninstallAll() noexcept {
	CCustomBaseEntity::RemoveAll();
	CGameConfsManager::Instance().UnloadAll();
	const FactoryList_t &list{FactoryList()};
	for(ICustomEntityFactory *const it : list)
		it->Uninstall();
}

CGameConfsManager &CGameConfsManager::Instance() noexcept {
	static CGameConfsManager _instance{};
	return _instance;
}

IGameConfig *const CGameConfsManager::Load(const char *const classname) noexcept
{
	IGameConfig *gameconf{nullptr};

	ConfMap_t::iterator it{_confmap.find(classname)};
	if(it == _confmap.end()) {

		char path[MAX_PATH]{'\0'};
		smutils->BuildPath(Path_SM, path, _countof(path), "gamedata/CustomEntities/%s.txt", classname);

		if(!std::filesystem::exists(path))
			return nullptr;

		smutils->Format(path, _countof(path), "CustomEntities/%s", classname);

		char error[260]{'\0'};
		if(!gameconfs->LoadGameConfigFile(path, &gameconf, error, _countof(error))) {
			smutils->LogError(myself, "Failed to load %s [%s]", path, error);
			return nullptr;
		}

		gameconf = _confmap.insert_or_assign(classname, gameconf).first->second;
	} else
		gameconf = it->second;

	return gameconf;
}

void CGameConfsManager::Unload(const char *const classname) noexcept
{
	ConfMap_t::iterator it{_confmap.find(classname)};
	if(it == _confmap.end())
		return;

	gameconfs->CloseGameConfigFile(it->second);

	_confmap.erase(it);
}

void CGameConfsManager::UnloadAll() noexcept {
	for(const ConfMap_t::value_type &it : _confmap)
		gameconfs->CloseGameConfigFile(it.second);
}

IServerNetworkable *ICustomEntityFactory::Create(const char *const pClassName) noexcept {
	IServerNetworkable *const network{_realfactory->Create(pClassName)};
	CCustomBaseEntity *const custom{CreateCustom(network->GetBaseEntity())};
	custom->_factory = this;
	return network;
}