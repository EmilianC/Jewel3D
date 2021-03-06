// Copyright (c) 2021 Emilian Cioca
namespace gem
{
	template<typename Return, typename... Args>
	DelegateHandle Delegate<Return(Args...)>::Bind(std::function<Return(Args...)> functor)
	{
		ASSERT(functor, "'functor' is null.");

		lifetimePtr.reset();
		func = std::move(functor);
		id = ++idGenerator;

		return {controlBlock, id};
	}

	template<typename Return, typename... Args>
	void Delegate<Return(Args...)>::Bind(std::weak_ptr<void> lifetimeObject, std::function<Return(Args...)> functor)
	{
		ASSERT(functor, "'functor' is null.");
		ASSERT(!lifetimeObject.expired(), "'lifetimeObject' is already expired.");

		lifetimePtr = std::move(lifetimeObject);
		func = std::move(functor);
		id = 0;
	}

	template<typename Return, typename... Args>
	void Delegate<Return(Args...)>::Clear()
	{
		lifetimePtr.reset();
		func = nullptr;
		id = 0;
	}

	template<typename Return, typename... Args>
	Delegate<Return(Args...)>::operator bool() const
	{
		return !!func;
	}

	template<typename Return, typename... Args>
	template<typename... Params>
	auto Delegate<Return(Args...)>::operator()(Params&&... params) -> std::conditional_t<HasReturnValue, std::optional<Return>, void>
	{
		// This checks if the pointer is initialized (not nullptr).
		if (std::weak_ptr<void>{}.owner_before(lifetimePtr))
		{
			if (lifetimePtr.expired())
			{
				Clear();
			}
		}

		if constexpr (HasReturnValue)
		{
			std::optional<Return> result;
			if (func)
			{
				result = func(std::forward<Params>(params)...);
			}
			else
			{
				Clear();
			}

			return result;
		}
		else
		{
			if (func)
			{
				func(std::forward<Params>(params)...);
			}
			else
			{
				Clear();
			}
		}
	}

	template<typename Return, typename... Args>
	void Delegate<Return(Args...)>::Unbind(const DelegateHandle& handle)
	{
		if (id == handle.id)
		{
			Clear();
		}
	}

	template<typename Return, typename... Args>
	DelegateHandle Dispatcher<Return(Args...)>::Add(std::function<Return(Args...)> functor)
	{
		ASSERT(functor, "'functor' is null.");

		const unsigned id = ++idGenerator;
		bindings.emplace_back(std::weak_ptr<void>{}, std::move(functor), id);

		return {controlBlock, id};
	}

	template<typename Return, typename... Args>
	void Dispatcher<Return(Args...)>::Add(std::weak_ptr<void> lifetimeObject, std::function<Return(Args...)> functor)
	{
		ASSERT(functor, "'functor' is null.");
		ASSERT(!lifetimeObject.expired(), "'lifetimeObject' is already expired.");

		bindings.emplace_back(std::move(lifetimeObject), std::move(functor), 0);
	}

	template<typename Return, typename... Args>
	bool Dispatcher<Return(Args...)>::HasBindings() const
	{
		for (const Binding& binding : bindings)
		{
			if (binding.func)
				return true;
		}

		return false;
	}

	template<typename Return, typename... Args>
	void Dispatcher<Return(Args...)>::Clear()
	{
		bindings.clear();
	}

	template<typename Return, typename... Args>
	template<typename... Params>
	void Dispatcher<Return(Args...)>::Dispatch(Params&&... params)
	{
		const unsigned size = bindings.size();
		for (unsigned i = 0; i < size; ++i)
		{
			Binding& binding = bindings[i];

			// This checks if the pointer is initialized (not nullptr).
			if (std::weak_ptr<void>{}.owner_before(binding.lifetimePtr))
			{
				if (binding.lifetimePtr.expired())
				{
					binding.func = nullptr;
					continue;
				}
			}

			if (binding.func)
			{
				binding.func(std::forward<Params>(params)...);
			}
		}

		// Clear empty bindings now that it is safe to do so.
		for (unsigned i = size; i-- > 0;)
		{
			if (!bindings[i].func)
			{
				bindings.erase(bindings.begin() + i);
			}
		}
	}

	template<typename Return, typename... Args>
	void Dispatcher<Return(Args...)>::Unbind(const DelegateHandle& handle)
	{
		for (Binding& binding : bindings)
		{
			if (binding.id == handle.id)
			{
				// We do not erase the binding to avoid causing issues
				// if we are currently in the middle of dispatching.
				binding.lifetimePtr.reset();
				binding.func = nullptr;
				return;
			}
		}
	}
}
