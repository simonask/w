#include "wayward/support/data_franca/object.hpp"

namespace wayward {
  namespace data_franca {
    const Object Object::g_null_object;

    struct Object::ListEnumerator : Cloneable<Object::ListEnumerator, IReaderEnumerator> {
      using Iterator = Object::List::const_iterator;
      Iterator it_;
      Iterator end_;
      ListEnumerator(Iterator it, Iterator end) : it_(it), end_(end) {}
      ReaderPtr current_value() const final { return make_reader(**it_); }
      Maybe<String> current_key() const final { return Nothing; }
      bool at_end() const final { return it_ == end_; }
      void move_next() final { ++it_; }
    };

    struct Object::DictEnumerator : Cloneable<Object::DictEnumerator, IReaderEnumerator> {
      using Iterator = Object::Dictionary::const_iterator;
      Iterator it_;
      Iterator end_;
      DictEnumerator(Iterator it, Iterator end) : it_(it), end_(end) {}
      ReaderPtr current_value() const final { return make_reader(*it_->second); }
      Maybe<String> current_key() const final { return it_->first; }
      bool at_end() const final { return it_ == end_; }
      void move_next() final { ++it_; }
    };

    DataType Object::type() const {
      // XXX: Slightly fragile, but never change the order of the Either.
      switch (data_.which()) {
        case 0: return DataType::Nothing;
        case 1: return DataType::Boolean;
        case 2: return DataType::Integer;
        case 3: return DataType::Real;
        case 4: return DataType::String;
        case 5: return DataType::List;
        case 6: return DataType::Dictionary;
        default: return DataType::Nothing;
      }
    }

    ReaderEnumeratorPtr Object::enumerator() const {
      ReaderEnumeratorPtr ptr;
      data_.template when<List>([&](const List& list) {
        ptr = ReaderEnumeratorPtr{new ListEnumerator{list.begin(), list.end()}};
      });
      data_.template when<Dictionary>([&](const Dictionary& dict) {
        ptr = ReaderEnumeratorPtr{new DictEnumerator{dict.begin(), dict.end()}};
      });
      return std::move(ptr);
    }

    Object& Object::reference_at_index(size_t idx) {
      if (type() != DataType::List) {
        throw ObjectIndexOutOfBounds{"Object is not a list."};
      }
      Object* ptr = nullptr;
      data_.template when<List>([&](List& list) {
        if (idx < list.size()) {
          ptr = list[idx].get();
        }
      });
      if (ptr == nullptr) {
        throw ObjectIndexOutOfBounds{"Index out of bounds."};
      }
      return *ptr;
    }

    const Object& Object::at(size_t idx) const {
      if (type() != DataType::List) {
        throw ObjectIndexOutOfBounds{"Object is not a list."};
      }
      const Object* ptr = nullptr;
      data_.template when<List>([&](const List& list) {
        if (idx < list.size()) {
          ptr = list[idx].get();
        }
      });
      if (ptr == nullptr) {
        throw ObjectIndexOutOfBounds{"Index out of bounds."};
      }
      return *ptr;
    }

    Object& Object::reference_at_key(const String& key) {
      if (type() != DataType::Dictionary) {
        data_ = Dictionary{};
      }
      Object* ptr = nullptr;
      data_.template when<Dictionary>([&](Dictionary& dict) {
        auto it = dict.find(key);
        if (it == dict.end()) {
          it = dict.insert(std::make_pair(key, CloningPtr<Object>{new Object})).first;
        }
        ptr = it->second.get();
      });
      assert(ptr != nullptr);
      return *ptr;
    }

    bool Object::push_back(Object other) {
      if (type() != DataType::List) {
        data_ = List{};
      }
      data_.template when<List>([&](List& list) {
        list.emplace_back(new Object(std::move(other)));
      });
      return true;
    }

    size_t Object::length() const {
      size_t len = 0;
      data_.template when<List>([&](const List& list) {
        len = list.size();
      });
      data_.template when<Dictionary>([&](const Dictionary& dict) {
        len = dict.size();
      });
      return len;
    }

    bool Object::erase(const String& key) {
      bool b = false;
      data_.template when<Dictionary>([&](Dictionary& dict) {
        dict.erase(key);
        b = true;
      });
      return b;
    }

    const Object& Object::get(const String& key) const {
      const Object* ptr = &g_null_object;
      data_.template when<Dictionary>([&](const Dictionary& dict) {
        auto it = dict.find(key);
        if (it != dict.end()) {
          ptr = it->second.get();
        }
      });
      return *ptr;
    }

    bool Object::has_key(const String& key) const {
      bool has = false;
      data_.template when<Dictionary>([&](const Dictionary& dict) {
        auto it = dict.find(key);
        has = it != dict.end();
      });
      return has;
    }
  }
}
