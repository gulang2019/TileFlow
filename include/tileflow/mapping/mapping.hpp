#pragma once 

#include "model/engine.hpp"
#include "compound-config/compound-config.hpp"
#include "mapping/mapping.hpp"


#include "tileflow/problem/problem.hpp"
#include "tileflow/mapping/loop.hpp"
#include "tileflow/common.hpp"

namespace mapping {

namespace TileFlow {

class Node;
class ScopeNode;
class TileNode;
class OpNode;
class CollectNode;

class Visitor {
protected:
    virtual void visitScope(const ScopeNode*);
    virtual void visitTile(const TileNode*);
    virtual void visitOp(const OpNode*);
    virtual void visitCollect(const CollectNode*);
    friend class TileNode;
    friend class ScopeNode;
    friend class OpNode;
    friend class CollectNode;
public:
    virtual void run (const Node*);
};

class Node {
public: 
    enum type_t{
        Tile,
        Op,
        Scope,
        Collect 
    };
protected:
    Node::type_t type_;
    Node* parent_ = nullptr;
    std::vector<Node*> children_;
    std::string storage_level_name_;
    unsigned storage_level_;

    void ParseStorageLevel(config::CompoundConfigNode config);
    std::unordered_map<std::string, std::pair<int, int> > ParseFactors(const std::string & factors);
    std::vector<std::string> ParsePermutations(const std::string& buffer);

public: 
    Node(type_t t_): type_(t_) {}
    
    unsigned get_storage_level() const {return storage_level_;}
    std::string get_storage_name() const {return storage_level_name_;}
    type_t get_type() const {return type_;}
    void add_child(Node* child) {assert(child != nullptr); children_.push_back(child); child->set_parent(this);}
    const std::vector<const Node*> get_children() const{
        std::vector<const Node*> retval;
        for (auto child: children_) retval.push_back(child);
        return retval;
    }
    void set_parent(Node* parent) {parent_ = parent;}
    inline const Node* get_parent() const {return parent_;}

    virtual void display(std::string prefix, bool = true) const {std::cout << prefix << std::endl;}
    virtual void accept(Visitor* visitor) const = 0;
    virtual ~Node() {for (auto node: children_) delete node;}
    friend class Visitor;
};

class ScopeNode: public Node {
public:
    enum type_t {
        Sequential,
        Sharing,
        Parallel,
        Pipeline
    };
    ScopeNode(config::CompoundConfigNode config);
    void display(std::string prefix, bool recursive) const override;
    void accept(Visitor* visitor) const {visitor->visitScope(this);}
    ScopeNode::type_t get_scope_type() const {return type;}

private: 
    ScopeNode::type_t type;
    
};

class TileNode: public Node {
public:
    enum type_t {
        Temporal,
        Spatial
    };
private:

    // std::pair<int, int> represent the <end, residual end>
    std::vector<loop::TileFlow::Descriptor> loopnests_;
    TileNode::type_t type_;

public:
    TileNode(config::CompoundConfigNode config);
    void display(std::string prefix, bool recursive) const override;
    void accept(Visitor* visitor) const {visitor->visitTile(this);}
    bool is_spatial() const {return type_ == Spatial;}
    TileNode::type_t get_tile_type() const {return type_;}
    
    loop::Nest constructLoopNest(
        const std::map<std::string, problem::Shape::FactorizedDimensionID>&) const;
    size_t n_level() const {return loopnests_.size();}
    const std::vector<loop::TileFlow::Descriptor>& get_loops() const {return loopnests_;}
};

class OpNode: public Node {
    std::string name_;
    int op_index_;
    std::shared_ptr<problem::TileFlow::Workload> p_workload;
    // op dimension --> runtime iteration
    std::unordered_map<std::string, std::string> binding_; 
public:
    OpNode(config::CompoundConfigNode config);
    void display(std::string prefix, bool recursive) const override;
    const std::string & get_name() const {return name_;}
    void accept(Visitor* visitor) const {visitor->visitOp(this);}
    const std::shared_ptr<problem::TileFlow::Workload>& get_workload() const {return p_workload;}
};

class CollectNode: public Node {
    unsigned storage_level_;
    std::string storage_level_name_;
public: 
    void accept(Visitor* visitor) const {visitor->visitCollect(this);}
}; // a placeholder node to provide 

struct Mapping {
    std::map<unsigned, std::uint64_t> fanoutX_map;
    std::map<unsigned, std::uint64_t> fanoutY_map;  
    Node * root = nullptr;
    void Print();
};

Mapping ParseAndConstruct(config::CompoundConfigNode config,
                          model::Engine::Specs& arch_specs,
                          const problem::TileFlow::Workloads& workload);

}  // namespace TileFlow  
} // namespace mapping
