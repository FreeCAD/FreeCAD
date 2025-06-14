#pragma once

namespace is::signals
{
namespace detail
{
template <class ReturnType, class ClassType, bool AddConst, class... Args>
struct weak_binder
{
	using ConstMethodType = ReturnType (ClassType::*)(Args... args) const;
	using NonConstMethodType = ReturnType (ClassType::*)(Args... args);
	using MethodType = std::conditional_t<AddConst, ConstMethodType, NonConstMethodType>;
	using WeakPtrType = std::weak_ptr<ClassType>;

	weak_binder(MethodType pMethod, WeakPtrType&& pObject)
		: m_pMethod(pMethod)
		, m_pObject(pObject)
	{
	}

	ReturnType operator()(Args... args) const
	{
		if (auto pThis = m_pObject.lock())
		{
			return (pThis.get()->*m_pMethod)(std::forward<Args>(args)...);
		}
		return ReturnType();
	}

	MethodType m_pMethod;
	WeakPtrType m_pObject;
};
} // namespace detail

/// Weak this binding of non-const methods.
template <typename ReturnType, typename ClassType, typename... Params, typename... Args>
decltype(auto) bind_weak(ReturnType (ClassType::*memberFn)(Params... args), std::shared_ptr<ClassType> const& pThis, Args... args)
{
	using weak_binder_alias = detail::weak_binder<ReturnType, ClassType, false, Params...>;

	weak_binder_alias invoker(memberFn, std::weak_ptr<ClassType>(pThis));
	return std::bind(invoker, args...);
}

/// Weak this binding of const methods.
template <typename ReturnType, typename ClassType, typename... Params, typename... Args>
decltype(auto) bind_weak(ReturnType (ClassType::*memberFn)(Params... args) const, std::shared_ptr<ClassType> const& pThis, Args... args)
{
	using weak_binder_alias = detail::weak_binder<ReturnType, ClassType, true, Params...>;

	weak_binder_alias invoker(memberFn, std::weak_ptr<ClassType>(pThis));
	return std::bind(invoker, args...);
}

/// Weak this binding of non-const methods.
template <typename ReturnType, typename ClassType, typename... Params, typename... Args>
decltype(auto) bind_weak(ReturnType (ClassType::*memberFn)(Params... args), std::weak_ptr<ClassType> pThis, Args... args)
{
	using weak_binder_alias = detail::weak_binder<ReturnType, ClassType, false, Params...>;

	weak_binder_alias invoker(memberFn, std::move(pThis));
	return std::bind(invoker, args...);
}

/// Weak this binding of const methods.
template <typename ReturnType, typename ClassType, typename... Params, typename... Args>
decltype(auto) bind_weak(ReturnType (ClassType::*memberFn)(Params... args) const, std::weak_ptr<ClassType> pThis, Args... args)
{
	using weak_binder_alias = detail::weak_binder<ReturnType, ClassType, true, Params...>;

	weak_binder_alias invoker(memberFn, std::move(pThis));
	return std::bind(invoker, args...);
}

} // namespace is::signals
