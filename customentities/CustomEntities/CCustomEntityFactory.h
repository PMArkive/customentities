#ifndef _CUSTOMENTITYFACTORY_H_
#define _CUSTOMENTITYFACTORY_H_

#pragma once

#include <mathlib/mathlib.h>
#include <tier1/utlvector.h>
#include <mathlib/vmatrix.h>
#include <shareddefs.h>
#include <util.h>
#include <tier1/utldict.h>
#include <forward_list>
#include <toolframework/itoolentity.h>

#include <extension.h>
#include <MemUtils.h>
#include <CDetour.h>
#include <unordered_map>

class CCustomBaseEntity;

class __single_inheritance __declspec(novtable) ICustomEntityFactory : public IEntityFactory
{
	private:
		friend class CCustomBaseEntity;
		friend class CHookInfo;

	public:
		static void InstallAll() noexcept;
		static void UninstallAll() noexcept;

		static void Initialize() noexcept;
		static void Shutdown() noexcept;

	public:
		ICustomEntityFactory(const char *const realclassname, const char *fakeclassname, const char *const classname) noexcept;

	protected:
		virtual ~ICustomEntityFactory() noexcept = default;

		void Install() noexcept;
		void Uninstall() noexcept;

		virtual CCustomBaseEntity *const CreateCustom(CBaseEntity *const entity) noexcept = 0;

	public:
		virtual IServerNetworkable *Create(const char *const pClassName) noexcept override final;
		virtual void Destroy(IServerNetworkable *const pNetworkable) noexcept override final { _realfactory->Destroy(pNetworkable); }
		virtual size_t GetEntitySize() noexcept override final { return _realfactory->GetEntitySize(); }

	private:
		IEntityFactory *_realfactory{nullptr};
		const char *_realclassname{nullptr};
		const char *_fakeclassname{nullptr};
		const char *_classname{nullptr};
		bool _replaced{false};
		IGameConfig *_gameconf{nullptr};

		using FactoryList_t = std::forward_list<ICustomEntityFactory *>;
		static FactoryList_t &FactoryList() noexcept;
};

template <class T>
class CCustomEntityFactory final : private ICustomEntityFactory
{
	public:
		using ICustomEntityFactory::ICustomEntityFactory;

	private:
		virtual CCustomBaseEntity *const CreateCustom(CBaseEntity *const entity) noexcept override final {
			T *const custom{new T{}};
			custom->Attach(entity);
			return custom;
		}
};

class CEntityFactoryDictionary final : public IEntityFactoryDictionary
{
	public:
		void UninstallFactory(const char *const classname) noexcept
			{ m_Factories.Remove(classname); }

	private:
		CUtlDict<IEntityFactory *, unsigned short> m_Factories;
};

inline constexpr CEntityFactoryDictionary *const __EntityFactoryDictionary() noexcept
{ return reinterpret_cast<CEntityFactoryDictionary *const>(entityfactory); }

#define LINK_CUSTOM_ENTITY_TO_CLASS(real, fake, name) \
	static const CCustomEntityFactory<name> _##fake##_factory{#real, #fake, #name};

class CGameConfsManager final
{
	public:
		IGameConfig *const Load(const char *const classname) noexcept;
		void Unload(const char *const classname) noexcept;
		void UnloadAll() noexcept;

		static CGameConfsManager &Instance() noexcept;

	private:
		using ConfMap_t = std::unordered_map<std::string, IGameConfig *>;
		ConfMap_t _confmap{};
};

#endif