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

    template <template<class> class MonadA, class A, template<class> class MonadB, class B, class F>
    auto lift(MonadA<A> a, MonadB<B> b, F f) -> MonadB<decltype(f(std::declval<A>(), std::declval<B>()))> {
      return Bind<MonadA<A>>::bind(a, [&](A a_) {
        return Bind<MonadB<B>>::bind(b, [&](B b_) {
          return f(a_, b_);
        });
      });
    }

    template <template<class> class Monad, class A, class F>
    auto fmap(Monad<A> a, F&& f) -> typename Join<Monad<decltype(f(std::declval<A>()))>>::Type {
      return Bind<Monad<A>>::bind(std::move(a), std::forward<F>(f));
    }

    template <class Value, class F>
    auto fmap(Value&& value, F&& f) -> decltype(Bind<Value>::bind(std::forward<Value>(value), std::forward<F>(f))) {
      return Bind<Value>::bind(std::forward<Value>(value), std::forward<F>(f));
    }
  }
}

#endif // WAYWARD_SUPPORT_MONAD_HPP_INCLUDED
