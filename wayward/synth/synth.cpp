#include <wayward/support/format.hpp>
#include <wayward/support/http.hpp>
#include <wayward/support/node.hpp>
#include <wayward/template_engine.hpp>
#include <wayward/w.hpp>
#include <ajg/synth.hpp>
#include <iostream>

namespace ajg {
  namespace synth {
    struct AdaptedWaywardNode {
      explicit AdaptedWaywardNode(const wayward::Node& node) : node(node.ptr_) {}
      wayward::StructuredDataConstPtr node;

      bool operator==(const AdaptedWaywardNode& other) const {
        if (!node || !other.node) {
          return node == other.node;
        } else if (node->type() == other.node->type()) {
          auto t = node->type();
          switch (t) {
            case wayward::NodeType::Nil: {
              return true;
              break;
            }
            case wayward::NodeType::Boolean: {
              return *node->get_boolean() == *other.node->get_boolean();
              break;
            }
            case wayward::NodeType::Integer: {
              return *node->get_integer() == *other.node->get_integer();
              break;
            }
            case wayward::NodeType::Float: {
              return *node->get_float() == *other.node->get_float();
              break;
            }
            case wayward::NodeType::String: {
              return *node->get_string() == *other.node->get_string();
              break;
            }
            case wayward::NodeType::List: {
              return false;
              break;
            }
            case wayward::NodeType::Dictionary: {
              return false;
              break;
            }
          }
        }
        return false;
      }

      bool operator<(const AdaptedWaywardNode& other) const {
        if (node && other.node) {
          return wayward::Node{node}.to_string() < wayward::Node{other.node}.to_string();
        } else {
          return node < other.node;
        }
      }

      struct iterator {
        iterator(const AdaptedWaywardNode& node, size_t idx) : node(node), idx(idx) {}
        iterator(const iterator& other) = default;
        bool operator==(const iterator& other) const { return &node == &other.node && idx == other.idx; }
        bool operator!=(const iterator& other) const { return !(*this == other); }

        AdaptedWaywardNode operator*() const {
          return AdaptedWaywardNode{wayward::Node{node.node}[idx]};
        }

        iterator& operator++() { ++idx; return *this; }
        iterator operator++(int) { auto copy = *this; ++idx; return copy; }

        const AdaptedWaywardNode& node;
        size_t idx;
      };

      iterator begin() const { return iterator{*this, 0}; }
      iterator end() const { return iterator{*this, wayward::Node{node}.length()}; }
    };

    template <typename OS>
    OS& operator<<(OS& os, const AdaptedWaywardNode& node) {
      return os << wayward::Node(node.node).to_string();
    }

    template <class Behavior>
    struct adapter<Behavior, AdaptedWaywardNode> : concrete_adapter<Behavior, AdaptedWaywardNode> {
      adapter(const AdaptedWaywardNode& adapted) : concrete_adapter<Behavior, AdaptedWaywardNode>(AdaptedWaywardNode(adapted)) {}

      AJG_SYNTH_ADAPTER_TYPEDEFS(Behavior);

      wayward::Node node() const { return wayward::Node(this->adapted().node); }

      boolean_type to_boolean() const { return (bool)node(); } // Conversion with operator bool()
      //datetime_type to_datetime() const { return boost::local_sec_clock::local_time(); /* TODO */ }
      void output(ostream_type& out) const { out << get_string(); }
      const_iterator begin() const { return this->adapted().begin(); }
      const_iterator end()   const { return this->adapted().end(); }

      boolean_type is_boolean() const { return node().type() == wayward::NodeType::Boolean; }
      boolean_type is_string()  const { return node().type() == wayward::NodeType::String; }
      boolean_type is_numeric() const { return node().type() == wayward::NodeType::Integer || node().type() == wayward::NodeType::Float; }

      optional<value_type> index(const value_type& what) const {
        const wayward::Node& o = node();

        // Dictionary lookup:
        if (o.type() == wayward::NodeType::Dictionary) {
          std::string key = what.to_string();
          return optional<value_type>(AdaptedWaywardNode(o[key]));
        }

        if (o.type() == wayward::NodeType::List) {
          size_t idx = static_cast<size_t>(what.to_floating());
          return optional<value_type>(AdaptedWaywardNode(o[idx]));
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

    void initialize(Dict options) final {
      options["template_path"] >> template_path;
    }

    std::string render(const std::string& template_name, Dict params) final {
      namespace synth = ajg::synth;
      using Traits = synth::default_traits<char>;
      using Engine = synth::engines::django::engine<Traits>;
      using Template = synth::templates::path_template<Engine>;
      using Context = typename Template::context_type;

      auto path = wayward::format("{0}/{1}", template_path, template_name);
      Template templ { path, { template_path } };

      Context ctx;
      for (auto& pair: params) {
        ctx[pair.first] = synth::AdaptedWaywardNode{pair.second};
      }

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
