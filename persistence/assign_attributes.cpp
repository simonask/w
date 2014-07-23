#include "persistence/assign_attributes.hpp"
#include <wayward/support/data_visitor.hpp>

namespace persistence {
  namespace {
    using namespace wayward::data_franca;

    struct BestFaithDeserializerDataVisitor : wayward::DataVisitor {
      const Spectator& spec;
      BestFaithDeserializerDataVisitor(const Spectator& spec) : spec(spec) {}

      void visit_nil() final {
        return;
      }

      void visit_boolean(bool& b) final {
        if (spec >> b) return;
        Integer n;
        if (spec >> n) { b = n != 0; return; }
        Real r;
        if (spec >> r) { b = r != 0; return; }
        String str;
        if (spec >> str) { b = str != "false"; return; }
      }

      void visit_int8(std::int8_t& o) final {
        Integer i;
        if (spec >> i) { o = static_cast<std::int8_t>(i); }
        Real r;
        if (spec >> r) { o = static_cast<std::int8_t>(r); }
        String str;
        if (spec >> str) {
          std::stringstream ss { std::move(str) };
          ss >> o;
        }
      }

      void visit_int16(std::int16_t& o) final {
        Integer i;
        if (spec >> i) { o = static_cast<std::int16_t>(i); }
        Real r;
        if (spec >> r) { o = static_cast<std::int16_t>(r); }
        String str;
        if (spec >> str) {
          std::stringstream ss { std::move(str) };
          ss >> o;
        }
      }

      void visit_int32(std::int32_t& o) final {
        Integer i;
        if (spec >> i) { o = static_cast<std::int32_t>(i); }
        Real r;
        if (spec >> r) { o = static_cast<std::int32_t>(r); }
        String str;
        if (spec >> str) {
          std::stringstream ss { std::move(str) };
          ss >> o;
        }
      }

      void visit_int64(std::int64_t& o) final {
        Integer i;
        if (spec >> i) { o = static_cast<std::int64_t>(i); }
        Real r;
        if (spec >> r) { o = static_cast<std::int64_t>(r); }
        String str;
        if (spec >> str) {
          std::stringstream ss { std::move(str) };
          ss >> o;
        }
      }

      void visit_uint8(std::uint8_t& o) final {
        Integer i;
        if (spec >> i) { o = static_cast<std::uint8_t>(i); }
        Real r;
        if (spec >> r) { o = static_cast<std::uint8_t>(r); }
        String str;
        if (spec >> str) {
          std::stringstream ss { std::move(str) };
          ss >> o;
        }
      }

      void visit_uint16(std::uint16_t& o) final {
        Integer i;
        if (spec >> i) { o = static_cast<std::uint16_t>(i); }
        Real r;
        if (spec >> r) { o = static_cast<std::uint16_t>(r); }
        String str;
        if (spec >> str) {
          std::stringstream ss { std::move(str) };
          ss >> o;
        }
      }

      void visit_uint32(std::uint32_t& o) final {
        Integer i;
        if (spec >> i) { o = static_cast<std::uint32_t>(i); }
        Real r;
        if (spec >> r) { o = static_cast<std::uint32_t>(r); }
        String str;
        if (spec >> str) {
          std::stringstream ss { std::move(str) };
          ss >> o;
        }
      }

      void visit_uint64(std::uint64_t& o) final {
        Integer i;
        if (spec >> i) { o = static_cast<std::uint64_t>(i); }
        Real r;
        if (spec >> r) { o = static_cast<std::uint64_t>(r); }
        String str;
        if (spec >> str) {
          std::stringstream ss { std::move(str) };
          ss >> o;
        }
      }

      void visit_float(float& o) final {
        Real r;
        if (spec >> r) { o = static_cast<float>(r); return; }
        Integer i;
        if (spec >> i) { o = static_cast<float>(i); return; }
        String str;
        if (spec >> str) {
          std::stringstream ss { std::move(str) };
          ss >> o;
        }
      }

      void visit_double(double& o) final {
        Real r;
        if (spec >> r) { o = static_cast<double>(r); return; }
        Integer i;
        if (spec >> i) { o = static_cast<double>(i); return; }
        String str;
        if (spec >> str) {
          std::stringstream ss { std::move(str) };
          ss >> o;
        }
      }

      void visit_string(std::string& o) final {
        Integer i;
        if (spec >> i) {
          std::stringstream ss;
          ss << i;
          o = ss.str();
          return;
        }
        Real r;
        if (spec >> r) {
          std::stringstream ss;
          ss << r;
          o = ss.str();
          return;
        }

        spec >> o;
      }

      void visit_key_value(const std::string& key, AnyRef data, const IType* type) final {
        auto sub_spectator = spec[key];
        BestFaithDeserializerDataVisitor sub_visitor { sub_spectator };
        type->visit_data(data, sub_visitor);
      }

      void visit_element(std::int64_t idx, AnyRef data, const IType* type) final {
        auto sub_spectator = spec[idx];
        BestFaithDeserializerDataVisitor sub_visitor { sub_spectator };
        type->visit_data(data, sub_visitor);
      }

      void visit_special(AnyRef data, const IType* type) final {
        // XXX: TODO
      }

      bool can_modify() const {
        return true;
      }

      bool is_nil_at_current() const {
        return spec.type() == DataType::Nothing;
      }

    };
  }

  namespace detail {
    void assign_attributes(AnyRef record, const IRecordType* record_type, const wayward::data_franca::Spectator& data) {
      BestFaithDeserializerDataVisitor visitor { data };
      record_type->visit_data(record, visitor);
    }
  }
}
