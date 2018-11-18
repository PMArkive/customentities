#ifndef _MEMUTILS_H_
#define _MEMUTILS_H_

#pragma once

#include <tuple>

namespace MemUtils
{
	template <typename T>
	struct FunctionTraits;

	template <typename R, typename ...Args>
	struct FunctionTraits<R (__cdecl *)(Args...) noexcept> final
	{
		public:
			using Func = R (__cdecl *)(Args...);
			using Type = void;
			using Ret = R;
			using ArgTuple = std::tuple<Args...>;
			static inline constexpr const size_t ArgNum{sizeof...(Args)};

			static inline constexpr const bool isthiscall{false};
			static inline constexpr const bool isstdcall{false};
			static inline constexpr const bool iscdecl{true};
			static inline constexpr const bool ismember{false};
			static inline constexpr const bool isconst{true};
			static inline constexpr const bool isnoexcept{true};
	};

	template <typename R, typename ...Args>
	struct FunctionTraits<R (__stdcall *)(Args...) noexcept> final
	{
		public:
			using Func = R (__stdcall *)(Args...);
			using Type = void;
			using Ret = R;
			using ArgTuple = std::tuple<Args...>;
			static inline constexpr const size_t ArgNum{sizeof...(Args)};

			static inline constexpr const bool isthiscall{false};
			static inline constexpr const bool isstdcall{true};
			static inline constexpr const bool iscdecl{false};
			static inline constexpr const bool ismember{false};
			static inline constexpr const bool isconst{true};
			static inline constexpr const bool isnoexcept{true};
	};

	template <typename R, typename T, typename ...Args>
	struct FunctionTraits<R (__thiscall T::*)(Args...) noexcept> final
	{
		public:
			using Func = R (__thiscall T::*)(Args...);
			using Type = T;
			using Ret = R;
			using ArgTuple = std::tuple<Args...>;
			static inline constexpr const size_t ArgNum{sizeof...(Args)};

			static inline constexpr const bool isthiscall{true};
			static inline constexpr const bool isstdcall{false};
			static inline constexpr const bool iscdecl{false};
			static inline constexpr const bool ismember{true};
			static inline constexpr const bool isconst{false};
			static inline constexpr const bool isnoexcept{true};
	};

	template <typename R, typename T, typename ...Args>
	struct FunctionTraits<R (__thiscall T::*)(Args...) const noexcept> final
	{
		public:
			using Func = R (__thiscall T::*)(Args...) const;
			using Type = T; using Ret = R;
			using Ret = R;
			using ArgTuple = std::tuple<Args...>;
			static inline constexpr const size_t ArgNum{sizeof...(Args)};

			static inline constexpr const bool isthiscall{true};
			static inline constexpr const bool isstdcall{false};
			static inline constexpr const bool iscdecl{false};
			static inline constexpr const bool ismember{true};
			static inline constexpr const bool isconst{true};
			static inline constexpr const bool isnoexcept{true};
	};

	extern const void *const GetFuncAtOffset(const void *const ptr, const int offset) noexcept;
	extern const void *const *const GetVTable(const void *const ptr) noexcept;
	extern void SetVTable(void *const ptr, const void *const *const vtable) noexcept;
	extern const void *const GetVFunc(const void *const ptr, const int index) noexcept;
	extern const int GetVFuncIndex(const void *const func) noexcept;

	#pragma warning(push)
	#pragma warning(disable: 4172)
	template <typename D, typename S>
	inline constexpr D any_cast(S src) noexcept
	{
		union {
			S _src;
			D _dst;
		}; _src = src;
		return _dst;
	}
	#pragma warning(pop)

	template <typename T>
	inline void SetPtrVarAtOffset(void *const ptr, const int offset, const T &value) noexcept
	{
		*reinterpret_cast<T *const>((reinterpret_cast<unsigned char *const>(ptr) + offset)) = value;
	}

	template <typename T>
	inline T &GetPtrVarAtOffset(void *const ptr, const int offset) noexcept
	{
		return *reinterpret_cast<T *const>((reinterpret_cast<unsigned char *const>(ptr) + offset));
	}

	template <typename T>
	inline T &GetVarAtOffset(void *const ptr, const int offset) noexcept
	{
		return **reinterpret_cast<T *const *const>(reinterpret_cast<unsigned char *const>(ptr) + offset + 2);
	}

	template <typename T>
	inline void SetVarAtOffset(void *const ptr, const int offset, const T &value) noexcept
	{
		**reinterpret_cast<T *const *const>(reinterpret_cast<unsigned char *const>(ptr) + offset + 2) = value;
	}
};

#endif