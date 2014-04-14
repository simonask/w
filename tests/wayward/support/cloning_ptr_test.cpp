#include <gtest/gtest.h>
#include <wayward/support/cloning_ptr.hpp>

namespace {
  using wayward::CloningPtr;
  using wayward::Cloneable;

  struct CopyCountingObject : Cloneable<CopyCountingObject> {
    CopyCountingObject() {}
    CopyCountingObject(const CopyCountingObject& other) : count(other.count + 1) {}
    CopyCountingObject(CopyCountingObject&& other) : count(other.count) {}
    CopyCountingObject& operator=(const CopyCountingObject& other) { count++; return *this; }
    CopyCountingObject& operator=(CopyCountingObject&& other) { return *this; }

    int count = 0;
  };

  struct CloningPtrWithCounter : ::testing::Test {
    virtual void SetUp() override {
      ptr = CloningPtr<CopyCountingObject> { new CopyCountingObject };
    }

    CloningPtr<CopyCountingObject> ptr;
  };

  TEST_F(CloningPtrWithCounter, does_not_copy_on_move_construct) {
    auto b = std::move(ptr);
    EXPECT_EQ(b->count, 0);
  }

  TEST_F(CloningPtrWithCounter, clones_on_copy_construct) {
    auto b = ptr;
    EXPECT_EQ(b->count, 1);
  }

  TEST_F(CloningPtrWithCounter, does_not_copy_on_move_assign) {
    CloningPtr<CopyCountingObject> b;
    b = std::move(ptr);
    EXPECT_EQ(b->count, 0);
  }

  TEST_F(CloningPtrWithCounter, clones_on_copy_assign) {
    CloningPtr<CopyCountingObject> b;
    b = ptr;
    EXPECT_EQ(b->count, 1);
  }

  TEST_F(CloningPtrWithCounter, swaps_without_copying) {
    CloningPtr<CopyCountingObject> b { new CopyCountingObject };
    std::swap(ptr, b);
    EXPECT_EQ(ptr->count, 0);
    EXPECT_EQ(b->count, 0);
  }
}

