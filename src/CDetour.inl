#ifndef _CDETOUR_INL_
#define _CDETOUR_INL_

#pragma once

#include "CDetour.h"
#include "MemUtils.h"
#include <tuple>

template <const size_t i, const size_t s, typename Tuple>
inline void CArgsBuilder::BuildParamsHelper() noexcept
{
	if constexpr(i < s) {
		AddParam<std::tuple_element_t<i, Tuple>>();
		if constexpr(i+1 < s)
			BuildParamsHelper<i+1, s, Tuple>();
	}
}

template <typename ...Args>
inline void CArgsBuilder::BuildParams() noexcept
{
	constexpr const size_t size{sizeof...(Args)};
	if constexpr(size > 0)
		BuildParamsHelper<0, size, std::tuple<Args...>>();
}

template <typename T>
inline void CArgsBuilder::Build() noexcept
{
	using Traits = MemUtils::FunctionTraits<T>;
	using Tuple = typename Traits::ArgTuple;

	SetReturn<Traits::Ret>();
	SetCallConv(GetCallConv<T>());

	constexpr const size_t size{Traits::ArgNum};
	if constexpr(size > 0)
		BuildParamsHelper<0, size, Tuple>();
}

template <typename T>
inline constexpr const CArgsBuilder::CallConvention CArgsBuilder::GetCallConv() noexcept
{
	using Traits = MemUtils::FunctionTraits<T>;

	if constexpr(Traits::isthiscall)
		return CallConvention::CallConv_ThisCall;
	else if constexpr(Traits::isstdcall)
		return CallConvention::CallConv_StdCall;
	else if constexpr(Traits::iscdecl)
		return CallConvention::CallConv_Cdecl;
	else
		return CallConvention::CallConv_Unknown;
}

template <typename T>
inline constexpr const CArgsBuilder::PassType CArgsBuilder::GetPassType() noexcept
{
	using Type = std::remove_reference_t<std::remove_const_t<T>>;

	if constexpr(std::is_scalar_v<Type>)
		return PassType::PassType_Basic;
	else if constexpr(std::is_floating_point_v<Type>)
		return PassType::PassType_Float;
	else if constexpr(std::is_class_v<Type>)
		return PassType::PassType_Object;
	else
		return PassType::PassType_Unknown;
}

template <typename T>
inline constexpr const CArgsBuilder::PassFlags CArgsBuilder::GetPassFlags() noexcept
{
	constexpr const PassType type{GetPassType<T>()};
	int flags{0};

	if constexpr(std::is_reference_v<T>)
		flags |= PassFlags::PassFlag_ByRef;
	else
		flags |= PassFlags::PassFlag_ByVal;

	using Type = std::remove_reference_t<std::remove_const_t<T>>;

	if constexpr(std::is_destructible_v<Type>)
		flags |= PassFlags::PassFlag_ODtor;
	if constexpr(std::is_assignable_v<Type, Type>)
		flags |= PassFlags::PassFlag_AssignOp;
	if constexpr(std::is_trivially_constructible_v<Type>)
		flags |= PassFlags::PassFlag_OCtor;
	if constexpr(std::is_trivially_copy_constructible_v<Type>)
		flags |= PassFlags::PassFlag_CCtor;

	return static_cast<const PassFlags>(flags);
}

template <typename T>
inline void CArgsBuilder::SetReturn() noexcept
{
	constexpr const PassType type{GetPassType<T>()};
	constexpr const PassFlags flags{GetPassFlags<T>()};

	const void *ctor{nullptr};
	const void *cctor{nullptr};
	const void *ator{nullptr};
	const void *dtor{nullptr};

	using Type = std::remove_reference_t<std::remove_const_t<T>>;

	if constexpr(std::is_trivially_constructible_v<Type>)
		ctor = MemUtils::any_cast<const void *>(&CtorThunkHelper<Type>::NormalCtor);
	if constexpr(std::is_trivially_copy_constructible_v<Type>)
		cctor = MemUtils::any_cast<const void *>(&CtorThunkHelper<Type>::CopyCtor);
	if constexpr(std::is_assignable_v<Type>)
		ator = MemUtils::any_cast<const void *>(&CtorThunkHelper<Type>::AssignOp);
	if constexpr(std::is_destructible_v<Type, Type>)
		dtor = MemUtils::any_cast<const void *>(&CtorThunkHelper<Type>::Dtor);

	size_t size{0};
	if constexpr(!std::is_void_v<T>)
		size = sizeof(T);

	__super::SetReturnType(static_cast<const int>(size), type, static_cast<const int>(flags), const_cast<void *const>(ctor), const_cast<void *const>(cctor), const_cast<void *const>(dtor), const_cast<void *const>(ator));
}

template <typename T, typename>
inline void CArgsBuilder::AddParam() noexcept
{
	constexpr const PassType type{GetPassType<T>()};
	constexpr const PassFlags flags{GetPassFlags<T>()};

	const void *ctor{nullptr};
	const void *cctor{nullptr};
	const void *ator{nullptr};
	const void *dtor{nullptr};

	using Type = std::remove_reference_t<std::remove_const_t<T>>;

	if constexpr(std::is_trivially_constructible_v<Type>)
		ctor = MemUtils::any_cast<const void *>(&CtorThunkHelper<Type>::NormalCtor);
	if constexpr(std::is_trivially_copy_constructible_v<Type>)
		cctor = MemUtils::any_cast<const void *>(&CtorThunkHelper<Type>::CopyCtor);
	if constexpr(std::is_assignable_v<Type>)
		ator = MemUtils::any_cast<const void *>(&CtorThunkHelper<Type>::AssignOp);
	if constexpr(std::is_destructible_v<Type, Type>)
		dtor = MemUtils::any_cast<const void *>(&CtorThunkHelper<Type>::Dtor);

	size_t size{0};
	if constexpr(!std::is_void_v<T>)
		size = sizeof(T);

	__super::AddParam(static_cast<const int>(size), type, static_cast<const int>(flags), const_cast<void *const>(ctor), const_cast<void *const>(cctor), const_cast<void *const>(dtor), const_cast<void *const>(ator));
}

#endif