#include "DependencyGraph.hpp"

#include <map>
#include <memory>

std::ostream& operator<<(std::ostream& os, const EdgeDiff& diff)
{
    os<<"+"<<diff.new_edges.size()+diff.new_dependent_edges.size()
      <<"("<<diff.new_edges.size()<<")"
      <<" ~"<<diff.changed_edges.size()
      <<" -"<<diff.removed_edges.size()+diff.removed_dependent_edges.size()
      <<"("<<diff.removed_edges.size()<<")";
    return os;
}

EdgeInstaller::EdgeInstaller(RuleGraph& graph):
    rule_graph_(graph)
{

}

EdgeDiff EdgeInstaller::popEdgeDiff()
{
    clearEmptyEdges();
    auto diff = std::move(diff_);
    diff_.clear();
    return diff;
}

EdgePtr EdgeInstaller::addEdge(VertexPtr src, VertexPtr dst,
                               Transfer transfer, bool is_dependent)
{
    auto src_domain = src->rule->domain();
    auto domain = transfer.apply(src_domain) & dst->domain;
    if (not domain.empty()) {
        auto edge = rule_graph_.addEdge(
            src, dst, transfer, domain
        );
        if (not is_dependent)
            diff_.new_edges.emplace_back(edge);
        else
            diff_.new_dependent_edges.emplace_back(edge);
        return edge;
    }
    else {
        return EdgePtr(nullptr);
    }
}

void EdgeInstaller::updateEdge(EdgePtr edge)
{
    auto src_domain = edge->src->rule->domain();
    auto domain = edge->transfer.apply(src_domain) & edge->dst->domain;
    if (not domain.empty()) {
        edge->domain = domain;
        diff_.changed_edges.emplace_back(edge);
    }
    else {
        empty_edges_.emplace_back(edge);
    }
}

void EdgeInstaller::deleteEdge(VertexPtr src, VertexPtr dst, bool is_dependent)
{
    if (not is_dependent)
        diff_.removed_edges.emplace_back(src->rule, dst->rule);
    else
        diff_.removed_dependent_edges.emplace_back(src->rule, dst->rule);
    rule_graph_.deleteEdge(src, dst);
}

void EdgeInstaller::deleteOutEdges(VertexPtr src, bool is_dependent)
{
    // TODO: CRITICAL - implement deletion through lambda in the deletion
    // function so we traverse edges only once
    for (auto dst : rule_graph_.outVertices(src)) {
        if (not is_dependent)
            diff_.removed_edges.emplace_back(src->rule, dst->rule);
        else
            diff_.removed_dependent_edges.emplace_back(src->rule, dst->rule);
    }
    rule_graph_.deleteOutEdges(src);
}

void EdgeInstaller::deleteInEdges(VertexPtr dst, bool is_dependent)
{
    for (auto src : rule_graph_.inVertices(dst)) {
        if (not is_dependent)
            diff_.removed_edges.emplace_back(src->rule, dst->rule);
        else
            diff_.removed_dependent_edges.emplace_back(src->rule, dst->rule);
    }
    rule_graph_.deleteInEdges(dst);
}

void EdgeInstaller::clearEmptyEdges()
{
    for (auto empty_edge : empty_edges_) {
        diff_.removed_edges.emplace_back(std::make_pair(
            empty_edge->src->rule, empty_edge->dst->rule
        ));
        rule_graph_.deleteEdge(empty_edge);
    }
    empty_edges_.clear();
}

DependencyGraph::DependencyGraph(std::shared_ptr<Network> network):
    network_(std::move(network)), edge_installer_(rule_graph_)
{
    
}

EdgeDiff DependencyGraph::addRule(RulePtr rule)
{
    add_vertex(rule);
    add_out_edges(rule);
    add_in_edges(rule);
    return edge_installer_.popEdgeDiff();
}

EdgeDiff DependencyGraph::deleteRule(RulePtr rule)
{
    assert(VertexPtr(nullptr) != rule->vertex_);
    delete_out_edges(rule);
    delete_in_edges(rule);
    delete_vertex(rule);
    return edge_installer_.popEdgeDiff();
}

EdgeDiff DependencyGraph::addLink(Link link)
{
    // Add edges from both ends of the link
    for (auto directed_link : {link, Link::inverse(link)}) {
        auto& src_port = directed_link.src_port;
        auto& dst_port = directed_link.dst_port;

        // Delete source/sink edges
        edge_installer_.deleteInEdges(src_port->sinkRule()->vertex_);
        edge_installer_.deleteOutEdges(dst_port->sourceRule()->vertex_);

        // Add edges that go through the link
        for (auto src_rule : src_port->srcRules()) {
            // Find port action that sends packets to the src rule
            auto action = src_rule->actions().getPortAction(src_port->id());
            assert(nullptr != action);

            // Transfer to the destination port
            auto transfer = std::move(action->transfer);
            transfer.dstPort(dst_port->id());
            add_edges(src_rule, dst_port->dstRules(), std::move(transfer));
        }
    }

    return edge_installer_.popEdgeDiff();
}

EdgeDiff DependencyGraph::deleteLink(Link link)
{
    for (auto directed_link : {link, Link::inverse(link)}) {
        auto& src_port = directed_link.src_port;
        auto& dst_port = directed_link.dst_port;

        // Move edges to source/sink rules
        for (auto src_rule : src_port->srcRules()) {
            // Delete edges between ports
            for (auto dst_rule : dst_port->dstRules()) {
                edge_installer_.deleteEdge(src_rule->vertex_, dst_rule->vertex_);
            }

            // Find port action that sends packets to the src rule
            auto action = src_rule->actions().getPortAction(src_port->id());
            assert(nullptr != action);

            // Add edges to the sink rule
            edge_installer_.addEdge(src_rule->vertex_,
                                    src_port->sinkRule()->vertex_,
                                    action->transfer);
        }

        // Add edges from the source rule
        add_edges(dst_port->sourceRule(), dst_port->dstRules());
    }

    return edge_installer_.popEdgeDiff();
}

const Vertex& DependencyGraph::vertex(RulePtr rule) const
{
    return *rule->vertex_;
}

const Edge& DependencyGraph::edge(EdgePtr edge) const
{
    return *edge;
}

VertexPtr DependencyGraph::add_vertex(RulePtr rule)
{
    auto vertex_desc = rule_graph_.addVertex(
        Vertex(rule, rule->domain(), influence_graph_.addVertex())
    );
    rule->vertex_ = vertex_desc;
    return vertex_desc;
}

void DependencyGraph::delete_vertex(RulePtr rule)
{
    influence_graph_.deleteVertex(vertex(rule).influence_vertex);
    rule_graph_.deleteVertex(rule->vertex_);
    rule->vertex_ = VertexPtr(nullptr);
}

void DependencyGraph::add_in_edges(RulePtr dst_rule)
{
    // Dst rule is not a table rule
    if (RuleType::FLOW != dst_rule->type()) {
        return;
    }

    auto table = dst_rule->table();
    if (table->isFrontTable() && dst_rule->isTableMiss()) {
        // Create edges from source rules (because table miss is installed before links)
        for (auto port : dst_rule->sw()->ports()) {
            auto src_rule = port->sourceRule();
            edge_installer_.addEdge(src_rule->vertex_, dst_rule->vertex_,
                                    Transfer::identityTransfer(), true);
        }
    }
    else {
        // Create influence from upper rules
        for (auto upper_rule : table->upperRules(dst_rule)) {
            auto influence_domain = upper_rule->domain() & dst_rule->domain();
            if (not influence_domain.empty()) {
                add_influence(upper_rule, dst_rule, influence_domain);
            }
        }
        update_domain(dst_rule);

        // Move edges from lower rules to the dst rule
        for (auto lower_rule : table->lowerRules(dst_rule)) {
            auto influence_domain = dst_rule->domain() & lower_rule->domain();
            if (not influence_domain.empty()) {
                add_influence(dst_rule, lower_rule, influence_domain);
                update_domain(lower_rule);

                for (auto in_edge : rule_graph_.inEdges(lower_rule->vertex_)) {
                    auto common_edge_domain = in_edge->domain & dst_rule->vertex_->domain;
                    if (not common_edge_domain.empty()) {
                        edge_installer_.addEdge(in_edge->src, dst_rule->vertex_,
                                                in_edge->transfer, true);
                        edge_installer_.updateEdge(in_edge);
                    }
                }
            }
        }
    }
}

void DependencyGraph::delete_in_edges(RulePtr dst_rule)
{
    auto table = dst_rule->table();
    if (table) {
        auto influences = influence_graph_.outEdges(
            dst_rule->vertex_->influence_vertex
        );
        for (auto influence_it = influences.begin();
             influence_it != influences.end();)
        {
            auto influence = *influence_it++;
            auto lower_rule = influence->dst_rule;

            delete_influence(dst_rule, lower_rule);
            update_domain(lower_rule);
            for (auto in_edge : rule_graph_.inEdges(dst_rule->vertex_)) {
                auto result = rule_graph_.edge(in_edge->src,
                                               lower_rule->vertex_);
                if (result.second) {
                    edge_installer_.updateEdge(result.first);
                }
                else {
                    edge_installer_.addEdge(in_edge->src,
                                            lower_rule->vertex_,
                                            in_edge->transfer);
                }
            }
        }
        edge_installer_.deleteInEdges(dst_rule->vertex_, true);
    }
    else {
        edge_installer_.deleteInEdges(dst_rule->vertex_);
    }
}

void DependencyGraph::add_out_edges(RulePtr src_rule)
{
    if (src_rule->type() == RuleType::FLOW) {
        for (const auto &port_action : src_rule->actions().port_actions) {
            add_edges_to_port(src_rule, port_action);
        }
        for (const auto &table_action : src_rule->actions().table_actions) {
            add_edges_to_table(src_rule, table_action);
        }
        for (const auto &group_action : src_rule->actions().group_actions) {
            add_edges_to_group(src_rule, group_action);
        }
    }
}

void DependencyGraph::add_edges_to_port(RulePtr src_rule,
                                        const PortAction& action)
{
    auto transfer = action.transfer;
    switch (action.port_type) {
    case PortType::NORMAL: {
        auto src_port = action.port;
        auto dst_port = network_->adjacentPort(src_port);
        if (dst_port) {
            // Transfer to the destination port
            transfer.dstPort(dst_port->id());
            add_edges(src_rule, dst_port->dstRules(), std::move(transfer));
        }
        else {
            edge_installer_.addEdge(src_rule->vertex_,
                                    src_port->sinkRule()->vertex_,
                                    std::move(transfer));
        }
        break;
    }
    case PortType::DROP:
        edge_installer_.addEdge(src_rule->vertex_,
                                network_->dropRule()->vertex_,
                                std::move(transfer));
        break;
    case PortType::CONTROLLER:
        edge_installer_.addEdge(src_rule->vertex_,
                                network_->controllerRule()->vertex_,
                                std::move(transfer));
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
    auto transfer = action.transfer;
    add_edges(src_rule, dst_rules, std::move(transfer));
}

void DependencyGraph::add_edges_to_group(RulePtr src_rule,
                                         const GroupAction& action)
{

}

void DependencyGraph::delete_out_edges(RulePtr src_rule)
{
    edge_installer_.deleteOutEdges(src_rule->vertex_);
}

void DependencyGraph::add_edges(RulePtr src_rule, RuleRange dst_rules,
                                Transfer transfer)
{
    for (const auto& dst_rule : dst_rules) {
        edge_installer_.addEdge(src_rule->vertex_, dst_rule->vertex_, transfer);
    }
}

void DependencyGraph::update_domain(RulePtr rule)
{
    rule->vertex_->domain = rule->domain();
    for (auto influence : influence_graph_.inEdges(rule->vertex_->influence_vertex)) {
        rule->vertex_->domain -= influence->domain;
    }
}

void DependencyGraph::add_influence(RulePtr src_rule, RulePtr dst_rule,
                                    NetworkSpace domain)
{
    influence_graph_.addEdge(
        src_rule->vertex_->influence_vertex,
        dst_rule->vertex_->influence_vertex,
        src_rule, dst_rule, std::move(domain)
    );
}

void DependencyGraph::delete_influence(RulePtr src_rule, RulePtr dst_rule)
{
    influence_graph_.deleteEdge(
        src_rule->vertex_->influence_vertex,
        dst_rule->vertex_->influence_vertex
    );
}
