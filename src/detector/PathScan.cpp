#include "PathScan.hpp"

Node::Node(RulePtr rule, NetworkSpace domain, uint16_t multiplier) :
    rule(rule), domain(domain), multiplier(multiplier), counter(0),
    descriptor_(NodeDescriptor()), is_interceptor_(false)
{

}

Node::Node(Node&& other) noexcept :
    rule(other.rule), domain(std::move(other.domain)),
    multiplier(other.multiplier), counter(other.counter),
    descriptor_(other.descriptor_), is_interceptor_(other.is_interceptor_)
{

}

Node& Node::operator=(Node&& other) noexcept
{
    rule = other.rule;
    domain = std::move(other.domain);
    multiplier = other.multiplier;
    counter = other.counter;
    descriptor_ = other.descriptor_;
    is_interceptor_ = other.is_interceptor_;
    return *this;
}

std::vector<NodeDescriptor> PathScan::getNodes(RuleId id)
{
    auto it = node_map_.find(id);
    if (it != node_map_.end()) {
        return it->second;
    }
    else {
        return std::vector<NodeDescriptor>();
    }
}

std::vector<NodeDescriptor> PathScan::getBaseNodes(RuleId id)
{
    auto it = base_node_map_.find(id);
    if (it != base_node_map_.end()) {
        return it->second;
    }
    else {
        return std::vector<NodeDescriptor>();
    }
}

NodeDescriptor PathScan::addRootNode(RulePtr rule)
{
    return add_node(rule, NetworkSpace(), 1);
}

NodeDescriptor PathScan::addChildNode(NodeDescriptor parent, RulePtr rule,
                                      NetworkSpace domain, uint16_t multiplier)
{
    auto new_node = add_node(rule, domain, multiplier);
    addEdge(new_node, parent);
    return new_node;
}

NodeDescriptor PathScan::addBaseNode(RulePtr rule, NetworkSpace domain)
{
    auto new_base_node = add_node(rule, domain, 1);
    node_graph_[new_base_node].is_interceptor_ = true;
    return new_base_node;
}

NodeDescriptor PathScan::add_node(RulePtr rule, NetworkSpace domain,
                                  uint16_t multiplier)
{
    // Create node
    auto descriptor = node_graph_.addVertex(Node(rule, domain, multiplier));
    node_graph_[descriptor].descriptor_ = descriptor;

    // Add node to internal structures
    node_map_[rule->id()].push_back(descriptor);
    return descriptor;
}

void PathScan::deleteNode(NodeDescriptor node)
{
    delete_from_node_map(node);
    node_graph_.deleteVertex(node);
}

Node& PathScan::operator[](NodeDescriptor desc)
{
    return node_graph_[desc];
}

NodeRange PathScan::parentNodes(NodeDescriptor node)
{
    return node_graph_.outVertices(node);
}

NodeRange PathScan::childNodes(NodeDescriptor node)
{
    return node_graph_.inVertices(node);
}

void PathScan::addEdge(NodeDescriptor src_node_desc,
                       NodeDescriptor dst_node_desc)
{
    node_graph_.addEdge(src_node_desc, dst_node_desc);
}

void PathScan::deleteEdge(NodeDescriptor src_node_desc,
                          NodeDescriptor dst_node_desc)
{
    node_graph_.deleteEdge(src_node_desc, dst_node_desc);
}

void PathScan::delete_from_node_map(NodeDescriptor old_node)
{
    auto rule_id = node_graph_[old_node].rule->id();
    auto node_map_it = node_map_.find(rule_id);
    if (node_map_it != node_map_.end()) {
        auto node_list = node_map_it->second;
        auto if_node = [old_node](NodeDescriptor node) {
            return node == old_node;
        };
        node_list.erase(std::remove_if(node_list.begin(),
                                       node_list.end(),
                                       if_node),
                        node_list.end());
    }
}
