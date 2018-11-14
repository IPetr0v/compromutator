#include "PathScan.hpp"

#include <queue>

Node::Node(NodeId id, RulePtr rule, NetworkSpace&& domain,
           Transfer root_transfer, uint64_t multiplier):
    id(id), rule(rule), domain(std::move(domain)),
    root_transfer(std::move(root_transfer)), multiplier(multiplier),
    counter({0, 0}), final_time_(Timestamp::max())
{

}

std::ostream& operator<<(std::ostream& os, const Node& node)
{
    auto type = node.rule->type() == RuleType::SOURCE ? "SOURCE" : "FLOW";
    std::string children = "(";
    for (auto child : node.children_) {
        children += std::to_string(child->id) + ",";
    }
    //if (not node.children_.empty()) {
    //    children = children.substr(0, children.size() - 2);
    //}
    children += ")";

    os << "[" << type
       << ": id=" << node.id
       << ", parent=" << node.parent->id
       << ", child=" << children
       << ", domain=" << node.domain
       << ", mult=" << node.multiplier
       << "]";
    return os;
}

DomainPath::DomainPath(PathId id, NodePtr source, NodePtr sink,
                       Timestamp starting_time):
    id(id), source(source), sink(sink), source_domain(source->domain),
    sink_domain(source->root_transfer.apply(source_domain)),
    last_counter({0, 0}), starting_time(starting_time),
    final_time(Timestamp::max())
{

}

void DomainPath::set_interceptor(RulePtr rule)
{
    interceptor_rule_ = rule;
    if (rule) {
        interceptor = rule->info();
    }
}

RuleStatsFields PathScan::addNodeCounter(NodePtr node,
                                         RuleStatsFields new_counter)
{
    //return node->addCounter(new_counter);
    auto rule = node->rule;
    rule->rule_mapping_->counter.packet_count += new_counter.packet_count;
    rule->rule_mapping_->counter.byte_count += new_counter.byte_count;
    node->counter.packet_count += new_counter.packet_count;
    node->counter.byte_count += new_counter.byte_count;
    return node->counter;
}

void PathScan::forEachSubtreeNode(NodePtr root, NodeVisitor visitor)
{
    std::queue<NodePtr> node_queue;
    node_queue.push(root);
    while (not node_queue.empty()) {
        // Save child nodes to the queue
        auto& node = node_queue.front();
        auto rule = node->rule;
        if (rule->type() != RuleType::SOURCE) {
            // Save child nodes to the queue
            const auto& children = node->children_;
            for (auto child_node : children) {
                node_queue.push(child_node);
            }
        }

        // Visit node
        visitor(node);

        node_queue.pop();
    }
}

void PathScan::forEachPathNode(NodePtr source, NodePtr sink,
                               NodeDeletingVisitor deleting_visitor)
{
    std::queue<NodePtr> node_queue;
    node_queue.push(source);
    while (not node_queue.empty()) {
        // Save the parent node to the queue
        auto& node = node_queue.front();
        auto rule = node->rule;
        if (node->id != sink->id && rule->type() != RuleType::SINK) {
            node_queue.push(node->parent);
        }

        // Visit node
        if (deleting_visitor(node)) {
            //deleteNode(node);
            deleted_nodes_.insert(node);
        }

        node_queue.pop();
    }
}

NodePtr PathScan::addRootNode(RulePtr rule)
{
    assert(RuleType::SINK == rule->type());
    auto domain = rule->domain();
    auto multiplier = (uint64_t)1;
    auto root_transfer = Transfer::portTransfer(domain.inPort());
    auto node = add_node(rule, std::move(domain),
                         root_transfer, multiplier);
    node->root = node;
    node->parent = NodePtr(nullptr);
    return node;
}

NodePtr PathScan::addChildNode(NodePtr parent, RulePtr rule,
                                      NetworkSpace&& domain,
                                      const Transfer& transfer,
                                      uint64_t multiplier)
{
    assert(RuleType::SINK != rule->type());
    // TODO: CRITICAL - flows may merge into one flow, there is no interceptor!
    auto root_transfer = transfer * parent->root_transfer;
    auto node = add_node(rule, std::move(domain),
                         root_transfer, multiplier);
    node->root = parent->root;
    node->parent = parent;
    node->parent_backward_iterator_ = parent->children_.emplace(
        parent->children_.end(), node
    );
    return node;
}

void PathScan::setNodeFinalTime(NodePtr node, Timestamp final_time)
{
    //std::cout<<"[Path] setNodeFinalTime: "<<node->id<<std::endl;
    // Node version becomes old
    assert(Timestamp::max() != final_time);
    node->final_time_ = final_time;

    // Remove node from corresponding vertex
    auto mapping = node->rule->rule_mapping_;
    auto mapping_it = node->vertex_backward_iterator_;
    assert(NodeRemovalIterator(nullptr) != mapping_it);
    mapping->node_list.erase(mapping_it);

    // Remove node from the parent if it's not a root
    if (RuleType::SINK != node->rule->type()) {
        auto parent = node->parent;
        auto parent_it = node->parent_backward_iterator_;
        assert(NodeRemovalIterator(nullptr) != parent_it);
        parent->children_.erase(parent_it);
    }
}

void PathScan::deleteNode(NodePtr node)
{
    //std::cout<<"[Path] deleteNode: "<<node->id<<std::endl;
    // We must delete only old versions
    assert(Timestamp::max() != node->final_time_);
    if (node->rule->rule_mapping_->empty()) {
        delete_rule_mapping(node->rule);
    }
    node_list_.erase(node);
}

void PathScan::clearDeletedNodes()
{
    for (auto& node : deleted_nodes_) {
        deleteNode(node);
    }
    deleted_nodes_.clear();
}

const DomainPath& PathScan::domainPath(DomainPathPtr desc) const
{
    return *desc;
}

DomainPathPtr PathScan::outDomainPath(NodePtr source) const
{
    assert(source->rule->type() == RuleType::SOURCE);
    assert(source->out_path_ != DomainPathPtr(nullptr));
    return source->out_path_;
}

DomainPathPtr PathScan::addDomainPath(NodePtr source, NodePtr sink,
                                      Timestamp starting_time)
{
    assert(source->rule->type() == RuleType::SOURCE);
    assert(sink->rule->type() == RuleType::SINK);
    auto path = domain_path_list_.emplace(domain_path_list_.end(),
        last_path_id_++, source, sink, starting_time
    );
    source->out_path_ = path;
    return path;
}

void PathScan::setDomainPathFinalTime(DomainPathPtr path,
                                      Timestamp final_time)
{
    path->final_time = final_time;
}

void PathScan::deleteDomainPath(DomainPathPtr path)
{

}

RuleMappingDescriptor PathScan::add_rule_mapping(RulePtr rule)
{
    assert(RuleMappingDescriptor(nullptr) == rule->rule_mapping_);
    return rule->rule_mapping_ = rule_mapping_list_.emplace(
        rule_mapping_list_.end()
    );
}

void PathScan::delete_rule_mapping(RulePtr rule)
{
    //std::cout<<"delete_rule_mapping "<<rule<<std::endl;
    assert(rule->rule_mapping_->empty());
    rule_mapping_list_.erase(rule->rule_mapping_);
    rule->rule_mapping_ = RuleMappingDescriptor(nullptr);
}

NodePtr PathScan::add_node(RulePtr rule, NetworkSpace&& domain,
                                  const Transfer& root_transfer,
                                  uint64_t multiplier)
{
    // Create node
    auto node = node_list_.emplace(node_list_.end(),
        last_node_id_++, rule, std::move(domain),
        root_transfer, multiplier
    );
    //std::cout<<"[Path] add_node: "<<node->id<<std::endl;

    // Create vertex
    if (RuleMappingDescriptor(nullptr) == rule->rule_mapping_) {
        add_rule_mapping(rule);
    }

    // Adjust vertex
    auto mapping = rule->rule_mapping_;
    auto mapping_it = mapping->node_list.emplace(
        mapping->node_list.end(), node
    );
    node->vertex_backward_iterator_ = mapping_it;
    return node;
}
