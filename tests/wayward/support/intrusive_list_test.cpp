#include <gtest/gtest.h>
#include <wayward/support/intrusive_list.hpp>

namespace {
  using namespace wayward;

  struct Foo {
    int a;
    explicit Foo(int a) : a(a) {}

    IntrusiveListLink<Foo> link;
  };

  TEST(IntrusiveList, link_head) {
    IntrusiveList<Foo, &Foo::link> list;
    Foo a{1};
    Foo b{2};
    Foo c{3};
    list.link_head(&a);
    list.link_head(&b);
    list.link_head(&c);
    EXPECT_EQ(list.head(), &c);
    EXPECT_EQ(c.link.next, &b.link);
    EXPECT_EQ(b.link.next, &a.link);
  }

  TEST(IntrusiveList, link_tail) {
    IntrusiveList<Foo, &Foo::link> list;
    Foo a{1};
    Foo b{2};
    Foo c{3};
    list.link_tail(&a);
    list.link_tail(&b);
    list.link_tail(&c);
    EXPECT_EQ(list.head(), &a);
    EXPECT_EQ(a.link.next, &b.link);
    EXPECT_EQ(b.link.next, &c.link);
  }

  TEST(IntrusiveList, automatic_unlink) {
    IntrusiveList<Foo, &Foo::link> list;
    Foo a{1};
    list.link_tail(&a);
    {
      Foo b{2};
      list.link_head(&b);
      EXPECT_EQ(b.link.next, &a.link);
      EXPECT_EQ(2, list.size());
    }
    EXPECT_EQ(1, list.size());
    EXPECT_EQ(list.head(), &a);
  }

  TEST(IntrusiveList, iterators) {
    IntrusiveList<Foo, &Foo::link> list;
    Foo a{1};
    Foo b{2};
    Foo c{3};
    list.link_tail(&a);
    list.link_tail(&b);
    list.link_tail(&c);
    size_t sum = 0;
    for (auto& foo: list) {
      sum += foo.a;
    }
    EXPECT_EQ(6, sum);
  }
}
