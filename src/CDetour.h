#ifndef _CDETOUR_H_
#define _CDETOUR_H_

#pragma once

#include "MemUtils.h"
#include <forward_list>

using HANDLE = void *;

#define DISABLE_SOURCEHOOK_WARNINGS() \
	__pragma(warning(disable: 4265 4458))

#pragma warning(push)
DISABLE_SOURCEHOOK_WARNINGS();
#include <sourcehook.h>
#include <sourcehook_pibuilder.h>
#pragma warning(pop)

#define DISABLE_ASMJIT_WARNINGS() \
	__pragma(warning(disable: 4464 5031 4804)) \
	__pragma(warning(disable: 4242 4245 4826))

#pragma push_macro("new")
#undef new
#pragma warning(push)
DISABLE_ASMJIT_WARNINGS();
#undef clamp
#undef min
#undef max
#include <asmjit/src/asmjit/asmjit.h>
#pragma warning(pop)
#pragma pop_macro("new")

#define DETOUR_MEMBER_CALL(name, ...) (reinterpret_cast<name *const>(this)->*name::Actual)(__VA_ARGS__)
#define DETOUR_STATIC_CALL(name, ...) (name##Actual)(__VA_ARGS__)

#define DETOUR_DECL_MEMBER(ret, name, ...) \
	class name final { \
		public: \
			ret __thiscall Detour(__VA_ARGS__) noexcept; \
			static inline ret (__thiscall name::*Actual)(__VA_ARGS__) noexcept {nullptr}; \
	}; \
	ret __thiscall name::Detour(__VA_ARGS__) noexcept

#define DETOUR_DECL_STATIC(ret, conv, name, ...) \
	ret (conv *name##Actual)(__VA_ARGS__) noexcept {nullptr}; \
	ret conv name(__VA_ARGS__) noexcept

#define DETOUR_STATIC(name, addr) \
	Detour(MemUtils::any_cast<const void **>(&name##Actual), nullptr, MemUtils::any_cast<const void *>(name), MemUtils::any_cast<const void *>(addr));

#define DETOUR_MEMBER(name, addr) \
	Detour(MemUtils::any_cast<const void **>(&name::Actual), nullptr, MemUtils::any_cast<const void *>(&name::Detour), MemUtils::any_cast<const void *>(addr));

#define DETOUR_CREATE_STATIC(name, addr) \
	static const CDetour _##name##Detour{MemUtils::any_cast<const void **>(&name##Actual), nullptr, MemUtils::any_cast<const void *>(name), MemUtils::any_cast<const void *>(addr)};

#define DETOUR_CREATE_MEMBER(name, addr) \
	static const CDetour _##name##Detour{MemUtils::any_cast<const void **>(&name::Actual), nullptr, MemUtils::any_cast<const void *>(&name::Detour), MemUtils::any_cast<const void *>(addr)};

extern SourceHook::ISourceHook *g_SHPtr;
extern int g_PLID;

#define DETOUR_GETTHIS(type) META_IFACEPTR(type)
#define DETOUR_RETURN(res) RETURN_META(MRES_##res)
#define DETOUR_RETURN_VALUE(res, val) RETURN_META_VALUE(MRES_##res, val)

class CArgsBuilder final : private SourceHook::CProtoInfoBuilder
{
	public:
		using CallConvention = SourceHook::ProtoInfo::CallConvention;
		using PassType = SourceHook::PassInfo::PassType;
		using PassFlags = SourceHook::PassInfo::PassFlags;

		CArgsBuilder() noexcept : SourceHook::CProtoInfoBuilder(CallConvention::CallConv_Unknown) {}

		template <typename T>
		CArgsBuilder() noexcept : SourceHook::CProtoInfoBuilder(CallConvention::CallConv_Unknown) { Build<T>(); }

		template <typename T>
		CArgsBuilder(T f) noexcept : SourceHook::CProtoInfoBuilder(CallConvention::CallConv_Unknown) { Build<T>(); }

		template <typename T>
		void Build() noexcept;

		template <typename T>
		static constexpr const CallConvention GetCallConv() noexcept;

		template <typename T>
		static constexpr const PassFlags GetPassFlags() noexcept;

		template <typename T>
		static constexpr const PassType GetPassType() noexcept;

		void SetCallConv(const CallConvention conv) noexcept { const_cast<SourceHook::ProtoInfo *const>(GetInfo())->convention = static_cast<const CallConvention>(conv); }

		template <typename T>
		void SetReturn() noexcept;

		template <typename T, typename = std::enable_if_t<!std::is_void_v<T>>>
		void AddParam() noexcept;

		template <typename ...Args>
		void BuildParams() noexcept;

		const SourceHook::ProtoInfo *const GetInfo() const noexcept { return const_cast<CArgsBuilder *const>(this)->operator SourceHook::ProtoInfo *(); }

	private:
		template <class T, typename = std::enable_if_t<!std::is_void_v<T>>>
		class CtorThunkHelper final
		{
			public:
				using Type = std::remove_reference_t<std::remove_const_t<T>>;

				#pragma push_macro("new")
				#undef new
				__forceinline constexpr Type *const NormalCtor() noexcept { return new(reinterpret_cast<Type *const>(this)) Type{}; }
				__forceinline constexpr Type *const CopyCtor(const Type &other) noexcept { return new(reinterpret_cast<Type *const>(this)) Type{other}; }
				#pragma pop_macro("new")
				__forceinline constexpr void Dtor() noexcept { reinterpret_cast<Type *const>(this)->~Type(); }
				__forceinline constexpr const Type &AssignOp(const Type &other) noexcept { return (*reinterpret_cast<Type *const>(this) = other); }
		};

		template <const size_t i, const size_t s, typename Tuple>
		void BuildParamsHelper() noexcept;
};

struct PubFuncInfo_t;
struct DelegateInfo_t;

class CDetour final
{
	public:
		CDetour() noexcept {}
		~CDetour() noexcept { Remove(); }

		CDetour(const void **const src, const void *const pthis, const void *const dst, const void *const addr) noexcept { Detour(src, pthis, dst, addr); }
		CDetour(const void *const ptr, const int index, const void *const pthis, const void *const dst, const CArgsBuilder &args, const bool post) noexcept { Hook(ptr, index, pthis, dst, args, post); }

		const bool Detour(const void **const src, const void *const pthis, const void *const dst, const void *const addr) noexcept;
		const bool Hook(const void *const ptr, const int index, const void *const pthis, const void *const dst, const CArgsBuilder &args, const bool post) noexcept;

		void UnDetour() noexcept;
		void UnHook() noexcept;
		void Remove() noexcept;

		void Pause() noexcept;
		void Resume() noexcept;

	private:
		const void *_src{nullptr};
		const void *_dst{nullptr};
		HANDLE _thread{nullptr};
		bool _thispatched{false};
		bool _paused{false};

		int _hookid{0};
		PubFuncInfo_t *_pubfuncinfo{nullptr};
		DelegateInfo_t *_delegateinfo{nullptr};

	public:
		static void RemoveAll() noexcept;

		static asmjit::JitRuntime &JitRuntime() noexcept;

	private:
		using DetourList_t = std::forward_list<CDetour *>;
		static DetourList_t &GetDetourList() noexcept;
};

#include "CDetour.inl"

#endif