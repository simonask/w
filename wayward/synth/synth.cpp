#include <wayward/support/format.hpp>
#include <wayward/support/http.hpp>
#include <wayward/support/data_franca/spelunker.hpp>
#include <wayward/template_engine.hpp>
#include <wayward/w.hpp>
#include <ajg/synth.hpp>
#include <iostream>

namespace ajg {
  namespace synth {
    using wayward::data_franca::Spelunker;
    using wayward::data_franca::ReaderPtr;
    using wayward::data_franca::ReaderEnumeratorPtr;
    using wayward::data_franca::DataType;
    using wayward::data_franca::make_reader;
    using wayward::data_franca::Boolean;
    using wayward::data_franca::Integer;
    using wayward::data_franca::Real;
    using wayward::data_franca::String;

    struct AdaptedWaywardNode {

      explicit AdaptedWaywardNode(Spelunker node) : node(std::move(node)) {}
      Spelunker node;

      bool operator==(const AdaptedWaywardNode& other) const {
        if (!node || !other.node) {
          return node.is_nothing() == other.node.is_nothing();
        } else if (node.type() == other.node.type()) {
          auto t = node.type();
          switch (t) {
            case DataType::Nothing: {
              return true;
              break;
            }
            case DataType::Boolean: {
              Boolean a, b;
              return (node >> a) && (other.node >> b) && a == b;
            }
            case DataType::Integer: {
              Integer a, b;
              return (node >> a) && (other.node >> b) && a == b;
              break;
            }
            case DataType::Real: {
              Real a, b;
              return (node >> a) && (other.node >> b) && a == b;
              break;
            }
            case DataType::String: {
              String a, b;
              return (node >> a) && (other.node >> b) && a == b;
              break;
            }
            case DataType::List: {
              return false;
              break;
            }
            case DataType::Dictionary: {
              return false;
              break;
            }
          }
        }
        return false;
      }

      bool operator<(const AdaptedWaywardNode& other) const {
        if (node && other.node) {
          auto t = node.type();
          if (t == other.node.type()) {
            switch (t) {
              case DataType::Nothing: {
                return false;
              }
              case DataType::Boolean: {
                Boolean a, b;
                return (node >> a) && (other.node >> b) && a < b;
              }
              case DataType::Integer: {
                Integer a, b;
                return (node >> a) && (other.node >> b) && a < b;
                break;
              }
              case DataType::Real: {
                Real a, b;
                return (node >> a) && (other.node >> b) && a < b;
                break;
              }
              case DataType::String: {
                String a, b;
                return (node >> a) && (other.node >> b) && a < b;
                break;
              }
              case DataType::List: {
                // TODO:
                return false;
              }
              case DataType::Dictionary: {
                // TODO:
                return false;
              }
            }
          } else {
            return node.type() < other.node.type();
          }
        } else {
          return false;
        }
      }

      struct iterator {
        Spelunker::iterator it;

        iterator(Spelunker::iterator it) : it(std::move(it)) {}
        iterator(const iterator&) = default;
        iterator(iterator&&) = default;


        bool operator==(const iterator& other) const { return it == other.it; }
        bool operator!=(const iterator& other) const { return !(*this == other); }

        AdaptedWaywardNode operator*() const {
          return AdaptedWaywardNode{*it};
        }

        iterator& operator++() { ++it; return *this; }
        iterator operator++(int) { auto copy = *this; ++it; return copy; }
      };

      iterator begin() const { return iterator{node.begin()}; }
      iterator end() const { return iterator{node.end()}; }

      std::string to_string() const {
        switch (node.type()) {
          case DataType::Nothing: {
            return "";
          }
          case DataType::Boolean: {
            Boolean b;
            node >> b;
            return b ? "true" : "false";
          }
          case DataType::Integer: {
            Integer n;
            node >> n;
            std::stringstream ss;
            ss << n;
            return ss.str();
          }
          case DataType::Real: {
            Real r;
            node >> r;
            std::stringstream ss;
            ss << r;
            return ss.str();
          }
          case DataType::String: {
            String s;
            node >> s;
            return std::move(s);
          }
          case DataType::List: {
            std::stringstream ss;
            ss << '[';
            for (auto it = node.begin(); it != node.end();) {
              if (it->type() == DataType::String)
                ss << '"';
              ss << AdaptedWaywardNode{*it}.to_string();
              if (it->type() == DataType::String)
                ss << '"';
              ++it;
              if (it != node.end()) {
                ss << ", ";
              }
            }
            ss << ']';
            return ss.str();
          }
          case DataType::Dictionary: {
            std::stringstream ss;
            ss << '{';
            for (auto it = node.begin(); it != node.end();) {
              ss << *it.key();
              ss << ": ";
              if (it->type() == DataType::String)
                ss << '"';
              ss << AdaptedWaywardNode{*it}.to_string();
              if (it->type() == DataType::String)
                ss << '"';
              ++it;
              if (it != node.end()) {
                ss << ", ";
              }
            }
            ss << '}';
            return ss.str();
          }
        }
      }
    };

    template <typename OS>
    OS& operator<<(OS& os, const AdaptedWaywardNode& node) {
      return os << node.to_string();
    }

    template <class Behavior>
    struct adapter<Behavior, AdaptedWaywardNode> : concrete_adapter<Behavior, AdaptedWaywardNode> {
      adapter(const AdaptedWaywardNode& adapted) : concrete_adapter<Behavior, AdaptedWaywardNode>(AdaptedWaywardNode(adapted)) {}

      AJG_SYNTH_ADAPTER_TYPEDEFS(Behavior);

      const Spelunker& node() const { return this->adapted().node; }

      boolean_type to_boolean() const { return (bool)node(); } // Conversion with operator bool()
      //datetime_type to_datetime() const { return boost::local_sec_clock::local_time(); /* TODO */ }
      void output(ostream_type& out) const { out << get_string(); }
      const_iterator begin() const { return this->adapted().begin(); }
      const_iterator end()   const { return this->adapted().end(); }

      boolean_type is_boolean() const { return node().type() == DataType::Boolean; }
      boolean_type is_string()  const { return node().type() == DataType::String; }
      boolean_type is_numeric() const { return node().type() == DataType::Integer || node().type() == DataType::Real; }

      optional<value_type> index(const value_type& what) const {
        auto& o = node();

        // Dictionary lookup:
        if (o.type() == DataType::Dictionary) {
          std::string key = what.to_string();
          return optional<value_type>(AdaptedWaywardNode{o[key]});
        }

        if (o.type() == DataType::List) {
          size_t idx = static_cast<size_t>(what.to_floating());
          return optional<value_type>(AdaptedWaywardNode{o[idx]});
        }

        return boost::none;
      }

    private:
      std::string get_string() const {
        std::string str;
        node() >> str;
        return std::move(str);
      }
    };
  }
}

namespace wayward {
  struct SynthTemplateEngine : ITemplateEngine {
    std::string template_path;

    void initialize(Options options) final {
      options["template_path"] >> template_path;
    }

    std::string render(const std::string& template_name, Options params) final {
      namespace synth = ajg::synth;
      using Traits = synth::default_traits<char>;
      using Engine = synth::engines::django::engine<Traits>;
      using Template = synth::templates::path_template<Engine>;
      using Context = typename Template::context_type;

      auto path = wayward::format("{0}/{1}", template_path, template_name);
      Template templ { path, { template_path } };

      Context ctx;
      for (auto it = params.begin(); it != params.end(); ++it) {
        ctx[it->first] = synth::AdaptedWaywardNode{it->second};
      }

      wayward::log::debug("synth", wayward::format("Rendering template: {0}", path));

      return templ.render_to_string(ctx);
    }

    static std::shared_ptr<ITemplateEngine> instantiate() {
      return std::make_shared<SynthTemplateEngine>();
    }
  };
}

extern "C" void wayward_init_plugin() {
  wayward::register_template_engine("synth", wayward::SynthTemplateEngine::instantiate);
}
