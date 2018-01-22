#include "PathScan.hpp"

#include <queue>

Node::Node(NodeId id, RulePtr rule, NetworkSpace&& domain,
           Transfer root_transfer, uint64_t multiplier):
    id(id), rule(rule), domain(std::move(domain)),
    root_transfer(std::move(root_transfer)), multiplier(multiplier),
    counter(0), final_time_(Timestamp::max())
{

}

DomainPath::DomainPath(PathId id, NodeDescriptor source, NodeDescriptor sink,
                       Timestamp starting_time):
    id(id), source(source), sink(sink), source_domain(source->domain),
    sink_domain(source->root_transfer.apply(source_domain)),
    starting_time(starting_time), final_time(Timestamp::max())
{

}

const std::list<NodeDescriptor>& PathScan::getNodes(RulePtr rule) const
{
    return rule->rule_mapping_->node_list;
}

void PathScan::forEachSubtreeNode(NodeDescriptor root, NodeVisitor visitor)
{
    std::queue<NodeDescriptor> node_queue;
    node_queue.push(root);
    while (not node_queue.empty()) {
        // Save child nodes to the queue
        auto& node = node_queue.front();
        auto rule = node->rule;
        if (rule->type() != RuleType::SOURCE) {
            // Save child nodes to the queue
            auto children = node->children();
            for (auto child_node : children) {
                node_queue.push(child_node);
            }
        }

        // Visit node
        visitor(node);

        node_queue.pop();
    }
}

void PathScan::forEachPathNode(NodeDescriptor source, NodeDescriptor sink,
                               NodeDeletingVisitor deleting_visitor)
{
    std::queue<NodeDescriptor> node_queue;
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
            deleteNode(node);
        }

        node_queue.pop();
    }

}

NodeDescriptor PathScan::addRootNode(RulePtr rule)
{
    assert(RuleType::SINK == rule->type());
    auto domain = NetworkSpace::wholeSpace();
    auto multiplier = (uint64_t)1;
    auto root_transfer = Transfer::identityTransfer();
    auto node = add_node(rule, std::move(domain),
                         root_transfer, multiplier);
    node->parent = NodeDescriptor(nullptr);
    return node;
}

NodeDescriptor PathScan::addChildNode(NodeDescriptor parent, RulePtr rule,
                                      NetworkSpace&& domain,
                                      const Transfer& transfer,
                                      uint64_t multiplier)
{
    assert(RuleType::SINK != rule->type());
    // TODO: CRITICAL - flows may merge into one flow, there is no intercepter!
    auto root_transfer = transfer * parent->root_transfer;
    auto node = add_node(rule, std::move(domain),
                         root_transfer, multiplier);
    node->parent = parent;
    node->parent_backward_iterator_ = parent->children_.emplace(
        parent->children_.end(), node
    );
    return node;
}

void PathScan::setNodeFinalTime(NodeDescriptor node, Timestamp time)
{
    // Node version becomes old
    assert(Timestamp::max() != time);
    node->final_time_ = time;

    // Remove node from corresponding vertex
    auto mapping = node->rule->rule_mapping_;
    auto mapping_it = node->vertex_backward_iterator_;
    assert(NodeRemovalIterator(nullptr) != mapping_it);
    mapping->node_list.erase(mapping_it);
    if (node->rule->rule_mapping_->empty()) {
        delete_rule_mapping(node->rule);
    }

    // Remove node from the parent if it's not a root
    if (RuleType::SINK != node->rule->type()) {
        auto parent = node->parent;
        auto parent_it = node->parent_backward_iterator_;
        assert(NodeRemovalIterator(nullptr) != parent_it);
        parent->children_.erase(parent_it);
    }
}

void PathScan::deleteNode(NodeDescriptor node)
{
    // We must delete only old versions
    assert(Timestamp::max() != node->final_time_);
    node_list_.erase(node);
}

const DomainPath& PathScan::domainPath(DomainPathDescriptor desc) const
{
    return *desc;
}
DomainPathDescriptor PathScan::outDomainPath(NodeDescriptor source) const
{
    assert(source->rule->type() == RuleType::SOURCE);
    assert(source->out_path_ != DomainPathDescriptor(nullptr));
    return source->out_path_;
}

DomainPathDescriptor PathScan::addDomainPath(NodeDescriptor source,
                                             NodeDescriptor sink,
                                             Timestamp starting_time)
{
    assert(source->rule->type() == RuleType::SOURCE);
    assert(sink->rule->type() == RuleType::SINK);
    auto path = domain_path_list_.emplace(domain_path_list_.end(),
        last_path_id_++, source, sink, starting_time
    );

    path->source_interceptor = new Rule(source->rule,
                                        std::move(path->source_domain));
    path->sink_interceptor = new Rule(sink->rule,
                                      std::move(path->sink_domain));
    source->out_path_ = path;
    return path;
}

void PathScan::setDomainPathFinalTime(DomainPathDescriptor path, Timestamp time)
{
    path->final_time = time;
}

void PathScan::deleteDomainPath(DomainPathDescriptor path)
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
    assert(rule->rule_mapping_->empty());
    rule_mapping_list_.erase(rule->rule_mapping_);
    rule->rule_mapping_ = RuleMappingDescriptor(nullptr);
}

NodeDescriptor PathScan::add_node(RulePtr rule, NetworkSpace&& domain,
                                  const Transfer& root_transfer,
                                  uint64_t multiplier)
{
    // Create node
    auto node = node_list_.emplace(node_list_.end(),
        last_node_id_++, rule, std::move(domain),
        root_transfer, multiplier
    );

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
