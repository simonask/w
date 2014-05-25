#pragma once
#ifndef WAYWARD_SUPPORT_INTRUSIVE_LIST_HPP_INCLUDED
#define WAYWARD_SUPPORT_INTRUSIVE_LIST_HPP_INCLUDED

#include <wayward/support/error.hpp>

namespace wayward {
  template <typename T>
  struct IntrusiveListLink {
    IntrusiveListLink<T>* next = nullptr;
    IntrusiveListLink<T>** prev_next_ptr = nullptr;

    ~IntrusiveListLink() {
      unlink();
    }

    bool is_linked() const {
      return next != nullptr;
    }

    void unlink() {
      if (is_linked()) {
        next->prev_next_ptr = prev_next_ptr;
        *prev_next_ptr = next;
        next = nullptr;
      }
    }
  };

  template <typename T, typename M>
  /*constexpr*/ std::ptrdiff_t
  offset_of_member(M T::*member) {
    // Technically undefined behavior.
    const volatile T* ptr = nullptr;
    return reinterpret_cast<std::ptrdiff_t>(&(ptr->*member));
  }

  struct IntrusiveListError : Error {
    IntrusiveListError(const std::string& msg) : Error(msg) {}
  };

  template <typename T, IntrusiveListLink<T> T::*LinkMember>
  struct IntrusiveList {
    using value_type = T;

    IntrusiveList() {
      sentinel_.prev_next_ptr = &sentinel_.next;
      sentinel_.next = &sentinel_;
    }
    IntrusiveList(const IntrusiveList<T, LinkMember>&) = delete;
    ~IntrusiveList() {
      if (!empty()) {
        throw IntrusiveListError("A non-empty IntrusiveList was destroyed. This means dangling pointers and is probably a logic error.");
      }
    }

    T* head() const {
      if (!empty()) {
        return object_from_link(sentinel_.next);
      }
      return nullptr;
    }

    bool empty() const {
      return sentinel_.next == &sentinel_;
    }

    void link_before(IntrusiveListLink<T>* link, IntrusiveListLink<T>* before) {
      link->next = before;
      link->prev_next_ptr = before->prev_next_ptr;
      *before->prev_next_ptr = link;
      before->prev_next_ptr = &link->next;
    }

    void link_after(IntrusiveListLink<T>* link, IntrusiveListLink<T>* before) {
      link_before(link, before->next);
    }

    void link_head(IntrusiveListLink<T>* link) {
      link_after(link, &sentinel_);
    }

    void link_tail(IntrusiveListLink<T>* link) {
      link_before(link, &sentinel_);
    }

    void link_before(T* element, T* before) {
      link_before(&(element->*LinkMember), &(before->*LinkMember));
    }

    void link_after(T* element, T* after) {
      link_after(&(element->*LinkMember), &(after->*LinkMember));
    }

    void link_head(T* element) {
      link_head(&(element->*LinkMember));
    }

    void link_tail(T* element) {
      link_tail(&(element->*LinkMember));
    }

    // STL container-like interface:
    struct iterator;
    iterator begin() const;
    iterator end() const;
    size_t size() const;
    T& front() { return *begin(); }

    static T* object_from_link(IntrusiveListLink<T>* link) {
      // Super undefined and non-portable, but we don't want links to
      // hold a pointer to the object they're part of.
      assert(link != nullptr);
      auto offset = offset_of_member(LinkMember);
      char* po = reinterpret_cast<char*>(link) - offset;
      return reinterpret_cast<T*>(po);
    }

  private:
    IntrusiveListLink<T> sentinel_;
  };

  template <typename T, IntrusiveListLink<T> T::*LinkMember>
  struct IntrusiveList<T, LinkMember>::iterator {
    explicit iterator(IntrusiveListLink<T>* link = nullptr) : link(link) {}
    iterator(const iterator&) = default;
    iterator& operator=(const iterator&) = default;
    bool operator==(const iterator& other) const { return link == other.link; }
    bool operator!=(const iterator& other) const { return link != other.link; }

    iterator& operator++() { link = link->next; return *this; }
    iterator  operator++(int) { auto copy = *this; ++(*this); return copy; }

    T* get() const { return IntrusiveList<T, LinkMember>::object_from_link(link); }
    T& operator*() const { return *get(); }
    T* operator->() const { return get(); }

    IntrusiveListLink<T>* link;
  };

  template <typename T, IntrusiveListLink<T> T::*LinkMember>
  size_t IntrusiveList<T, LinkMember>::size() const {
    // We have no other option but to count. If we kept a record of the number of elements,
    // the list object would have to be updated every time an object removes itself from the
    // list, requiring objects to know which list they're in, which defeats the purpose of the
    // intrusive list.
    size_t count = 0;
    for (auto& it: *this) {
      ++count;
    }
    return count;
  }

  template <typename T, IntrusiveListLink<T> T::*LinkMember>
  typename IntrusiveList<T, LinkMember>::iterator
  IntrusiveList<T, LinkMember>::begin() const {
    return iterator{const_cast<IntrusiveListLink<T>*>(sentinel_.next)};
  }

  template <typename T, IntrusiveListLink<T> T::*LinkMember>
  typename IntrusiveList<T, LinkMember>::iterator
  IntrusiveList<T, LinkMember>::end() const {
    return iterator{const_cast<IntrusiveListLink<T>*>(&sentinel_)};
  }
}

#endif // WAYWARD_SUPPORT_INTRUSIVE_LIST_HPP_INCLUDED
