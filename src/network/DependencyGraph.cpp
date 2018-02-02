#include "DependencyGraph.hpp"

#include <map>
#include <memory>

DependencyGraph::DependencyGraph(std::shared_ptr<Network> network):
    network_(std::move(network))
{
    graph_ = std::make_unique<GraphData>();
}

EdgeDiff DependencyGraph::addRule(RulePtr rule)
{
    latest_diff_.clear();

    auto vertex_desc = add_vertex(rule);
    assert(VertexDescriptor(nullptr) != vertex_desc);
    add_out_edges(rule);
    add_in_edges(rule);

    return latest_diff_;
}

EdgeDiff DependencyGraph::deleteRule(RulePtr rule)
{
    latest_diff_.clear();

    assert(VertexDescriptor(nullptr) != rule->vertex_);
    delete_out_edges(rule);
    delete_in_edges(rule);
    delete_vertex(rule);
    rule->vertex_ = VertexDescriptor(nullptr);

    return latest_diff_;
}

EdgeDiff DependencyGraph::addLink(Link link)
{
    latest_diff_.clear();

    for (auto directed_link : {link, Link::inverse(link)}) {
        auto& src_port = directed_link.src_port;
        auto& dst_port = directed_link.dst_port;

        // Delete source/sink edges
        delete_in_edges(src_port->sinkRule());
        delete_out_edges(dst_port->sourceRule());

        // Add edges that go through ports
        for (auto src_rule : src_port->srcRules()) {
            // Find getPort action in the src rule
            auto& actions = src_rule->actions().port_actions;
            auto it = std::find_if(actions.begin(), actions.end(),
                [src_port](const PortAction& port_action) -> bool {
                    if (port_action.port_type == PortType::NORMAL) {
                        return port_action.port->id() == src_port->id();
                    } else
                        return false;
                }
            );
            assert(actions.end() != it);

            const auto& transfer = it->transfer;
            auto output_domain = transfer.apply(src_rule->domain());
            add_edges(src_rule, dst_port->dstRules(), transfer, output_domain);
        }
    }

    return latest_diff_;
}

EdgeDiff DependencyGraph::deleteLink(Link link)
{
    latest_diff_.clear();

    for (auto directed_link : {link, Link::inverse(link)}) {
        auto& src_port = directed_link.src_port;
        auto& dst_port = directed_link.dst_port;

        // Move edges to source/sink rules
        for (auto src_rule : src_port->srcRules()) {
            // Delete edges between ports and add edges from the source rule
            for (auto dst_rule : dst_port->dstRules()) {
                delete_edge(src_rule, dst_rule);
                auto identity_transfer = Transfer::identityTransfer();
                add_edge(dst_port->sourceRule(), dst_rule,
                         identity_transfer, dst_rule->domain());
            }

            // Find getPort action in the src rule
            auto& actions = src_rule->actions().port_actions;
            auto it = std::find_if(actions.begin(), actions.end(),
                [src_port](const PortAction& port_action) -> bool {
                    if (port_action.port_type == PortType::NORMAL) {
                       return port_action.port->id() == src_port->id();
                    } else
                       return false;
                }
            );
            assert(actions.end() != it);

            // Add edges to the sink rule
            const auto& transfer = it->transfer;
            auto output_domain = transfer.apply(src_rule->domain());
            add_edge(src_rule, src_port->sinkRule(), transfer, output_domain);
        }
    }

    return latest_diff_;
}

const Edge& DependencyGraph::edge(EdgeDescriptor desc) const
{
    return graph_->edgeData(desc);
}

VertexDescriptor DependencyGraph::add_vertex(RulePtr rule)
{
    auto vertex_desc = graph_->addVertex({rule});
    rule->vertex_ = vertex_desc;
    return vertex_desc;
}

void DependencyGraph::delete_vertex(RulePtr rule)
{
    auto vertex_desc = rule->vertex_;
    graph_->deleteVertex(vertex_desc);
}

void DependencyGraph::add_out_edges(RulePtr src_rule)
{
    for (const auto& port_action : src_rule->actions().port_actions) {
        add_edges_to_port(src_rule, port_action);
    }
    for (const auto& table_action : src_rule->actions().table_actions) {
        add_edges_to_table(src_rule, table_action);
    }
    for (const auto& group_action : src_rule->actions().group_actions) {
        add_edges_to_group(src_rule, group_action);
    }
}

void DependencyGraph::add_in_edges(RulePtr dst_rule)
{
    // This map is used to save edges domains and unite them if necessary
    struct EdgeData {Transfer transfer; NetworkSpace domain;};
    std::map<RulePtr, EdgeData, Rule::PtrComparator> new_edges;

    auto dst_rule_domain = dst_rule->domain();
    auto sw = dst_rule->sw();
    auto table = dst_rule->table();
    if (not sw || not table) {
        // Dst rule is not a table rule
        return;
    }

    bool is_table_miss = table->tableMissRule()->id() == dst_rule->id();
    bool is_in_front_table = sw->isFrontTable(table);
    if (is_table_miss && is_in_front_table) {
        // Create edges from source rules
        for (auto port : sw->ports()) {
            auto source_rule = port->sourceRule();
            auto edge_domain = dst_rule->domain() & NetworkSpace(port->id());
            new_edges.emplace(std::make_pair(
                source_rule,
                EdgeData{Transfer::identityTransfer(), edge_domain}
            ));
        }
    }
    else {
        // Move edges from lower rules to the dst rule
        for (auto low_dst_rule : table->lowerRules(dst_rule)) {
            auto fallthrough_domain = NetworkSpace::emptySpace();
            for (auto edge : graph_->inEdges(low_dst_rule->vertex_)) {
                auto src_vertex = graph_->srcVertex(edge);
                auto src_rule = graph_->vertexData(src_vertex).rule;
                const auto &edge_transfer = graph_->edgeData(edge).transfer;
                auto edge_domain = graph_->edgeData(edge).domain;

                auto domain_intersection = dst_rule_domain & edge_domain;
                if (not domain_intersection.empty()) {
                    fallthrough_domain += domain_intersection;

                    // Save new edge
                    auto it = new_edges.find(src_rule);
                    if (it != new_edges.end()) {
                        auto &new_edge_domain = it->second.domain;
                        new_edge_domain += domain_intersection;
                    } else {
                        new_edges.emplace(std::make_pair(
                            src_rule,
                            EdgeData{edge_transfer, domain_intersection}
                        ));
                    }

                    // Change edge domain
                    edge_domain -= domain_intersection;
                    if (not edge_domain.empty()) {
                        set_edge_domain(edge, edge_domain);
                    } else {
                        delete_edge(src_rule, low_dst_rule);
                    }
                }
            }

            // Adjust dst rule domain as it goes through lower rules
            dst_rule_domain -= fallthrough_domain;
            if (dst_rule_domain.empty()) break;
        }
    }

    // Add new edges
    for (auto edge_pair : new_edges) {
        auto src_rule = edge_pair.first;
        const auto& transfer = edge_pair.second.transfer;
        auto domain = edge_pair.second.domain;
        add_edge(src_rule, dst_rule, transfer, domain, true);
    }
}

void DependencyGraph::delete_out_edges(RulePtr src_rule)
{
    // TODO: CRITICAL - implement deletion through lambda in the deletion
    // function so we traverse edges only once
    for (auto out_vertex_desc : graph_->outVertices(src_rule->vertex_)) {
        auto dst_rule = graph_->vertexData(out_vertex_desc).rule;
        latest_diff_.removed_edges.emplace_back(src_rule, dst_rule);
    }
    graph_->deleteOutEdges(src_rule->vertex_);
}

void DependencyGraph::delete_in_edges(RulePtr dst_rule)
{
    auto table = dst_rule->table();
    if (not table) {
        // Dst rule is not a getTable rule
        return;
    }

    // Move edges from the dst rule to lower rules
    for (auto edge : graph_->inEdges(dst_rule->vertex_)) {
        auto src_vertex = graph_->srcVertex(edge);
        auto src_rule = graph_->vertexData(src_vertex).rule;
        const auto& edge_transfer = graph_->edgeData(edge).transfer;
        auto edge_domain = graph_->edgeData(edge).domain;

        for (auto low_dst_rule : table->lowerRules(dst_rule)) {
            auto domain_intersection = edge_domain & low_dst_rule->domain();
            if (not domain_intersection.empty()) {
                add_edge(src_rule, low_dst_rule, edge_transfer,
                         domain_intersection);

                // Decrease the edge domain as it goes through lower rules
                edge_domain -= domain_intersection;
            }
        }
        latest_diff_.removed_dependent_edges.emplace_back(src_rule, dst_rule);
    }
    graph_->deleteInEdges(dst_rule->vertex_);
}

void DependencyGraph::add_edges_to_port(RulePtr src_rule,
                                        const PortAction& action)
{
    const auto& transfer = action.transfer;
    auto output_domain = transfer.apply(src_rule->domain());
    switch (action.port_type) {
    case PortType::NORMAL:
        {
            auto src_port = action.port;
            auto dst_port = network_->adjacentPort(src_port);
            if (dst_port) {
                add_edges(src_rule, dst_port->dstRules(),
                          transfer, output_domain);
            }
            else {
                add_edge(src_rule, src_port->sinkRule(),
                         transfer, output_domain);
            }
        }
        break;
    case PortType::DROP:
        add_edge(src_rule, network_->dropRule(), transfer, output_domain);
        break;
    case PortType::CONTROLLER:
        add_edge(src_rule, network_->controllerRule(), transfer, output_domain);
        break;
    case PortType::IN_PORT:
    case PortType::ALL:
        // In this case every port may be an output getPort
        // TODO: do it later with multiple actions and transfers
        // TODO: if new getPort is added, should we add new dependencies?
        break;
    }
}

void DependencyGraph::add_edges_to_table(RulePtr src_rule,
                                         const TableAction& action)
{
    auto dst_rules = action.table->rules();
    const auto& transfer = action.transfer;
    auto output_domain = transfer.apply(src_rule->domain());
    add_edges(src_rule, dst_rules, transfer, output_domain);
}

void DependencyGraph::add_edges_to_group(RulePtr src_rule,
                                         const GroupAction& action)
{

}

void DependencyGraph::add_edges(RulePtr src_rule, RuleRange dst_rules,
                                const Transfer& transfer,
                                NetworkSpace output_domain)
{
    // Create edges
    for (const auto& dst_rule : dst_rules) {
        NetworkSpace edge_domain = output_domain & dst_rule->domain();
        if (not edge_domain.empty()) {
            output_domain -= edge_domain;
            add_edge(src_rule, dst_rule, transfer, edge_domain);
        }
    }
}

void DependencyGraph::add_edge(RulePtr src_rule, RulePtr dst_rule,
                               const Transfer& transfer,
                               const NetworkSpace& domain,
                               bool is_dependent)
{
    auto src_vertex = src_rule->vertex_;
    auto dst_vertex = dst_rule->vertex_;

    auto edge_pair = graph_->edge(src_vertex, dst_vertex);
    auto edge_exists = edge_pair.second;
    if (edge_exists) {
        auto edge = edge_pair.first;
        auto edge_domain = graph_->edgeData(edge).domain;
        set_edge_domain(edge, edge_domain + domain);
    }
    else {
        auto edge = graph_->addEdge(src_vertex, dst_vertex,
                                    Edge{src_rule, dst_rule, transfer, domain});
        if (not is_dependent) {
            latest_diff_.new_edges.emplace_back(edge);
        }
        else {
            latest_diff_.new_dependent_edges.emplace_back(edge);
        }
    }
}

void DependencyGraph::delete_edge(RulePtr src_rule, RulePtr dst_rule)
{
    graph_->deleteEdge(src_rule->vertex_, dst_rule->vertex_);
    latest_diff_.removed_edges.emplace_back(src_rule, dst_rule);
}

void DependencyGraph::set_edge_domain(EdgeDescriptor edge,
                                      const NetworkSpace& domain)
{
    graph_->edgeData(edge).domain = domain;
    latest_diff_.changed_edges.emplace_back(edge);
}
