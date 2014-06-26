#pragma once
#ifndef WAYWARD_SUPPORT_MONAD_HPP_INCLUDED
#define WAYWARD_SUPPORT_MONAD_HPP_INCLUDED

namespace wayward {
  namespace monad {
    template <typename T> struct Join;
    template <typename T> struct Bind;

    template <typename T> struct Join {
      using Type = T;
    };

    template <template<class...> class MonadA, class A, template<class...> class MonadB, class B, class F, class... Rest>
    auto lift(MonadA<A, Rest...> a, MonadB<B, Rest...> b, F f) -> MonadB<decltype(f(std::declval<A>(), std::declval<B>())), Rest...> {
      return Bind<MonadA<A>>::bind(std::forward<MonadA<A>>(a), [&](A a_) {
        return Bind<MonadB<B>>::bind(std::forward<MonadB<B>>(b), [&](B b_) {
          return f(a_, b_);
        });
      });
    }

    template <template<class...> class Monad, class A, class F, class... Rest>
    auto fmap(Monad<A, Rest...>&& a, F&& f) -> typename Join<Monad<decltype(f(std::declval<A>())), Rest...>>::Type {
      return Bind<Monad<A>>::bind(std::forward<Monad<A>>(a), std::forward<F>(f));
    }

    template <template<class...> class Monad, class A, class F, class... Rest>
    auto fmap(Monad<A, Rest...>& a, F&& f) -> typename Join<Monad<decltype(f(std::declval<A>())), Rest...>>::Type {
      return Bind<Monad<A>>::bind(std::forward<Monad<A>>(a), std::forward<F>(f));
    }

    template <template<class...> class Monad, class A, class F, class... Rest>
    auto fmap(const Monad<A, Rest...>& a, F&& f) -> typename Join<Monad<decltype(f(std::declval<A>())), Rest...>>::Type {
      return Bind<Monad<A>>::bind(std::forward<Monad<A>>(a), std::forward<F>(f));
    }

    template <class Value, class F>
    auto fmap(Value&& value, F&& f) -> decltype(Bind<Value>::bind(std::forward<Value>(value), std::forward<F>(f))) {
      return Bind<Value>::bind(std::forward<Value>(value), std::forward<F>(f));
    }
  }
}

#endif // WAYWARD_SUPPORT_MONAD_HPP_INCLUDED
