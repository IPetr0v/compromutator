#include "DependencyUpdater.hpp"

DependencyPtr DependencyUpdater::add_table_dependency(RulePtr src_rule,
                                                      RulePtr dst_rule,
                                                      NetworkSpace domain)
{
    // TODO: keep rule_id map and do not make dependency multiset!
    DependencyPtr dependency = std::make_shared<Dependency>(src_rule,
                                                            dst_rule,
                                                            domain);
    src_rule->out_table_dependency_list_.push_back(dependency);
    dst_rule->in_table_dependency_list_.push_back(dependency);
    return dependency;
}

DependencyPtr DependencyUpdater::add_dependency(RulePtr src_rule,
                                                RulePtr dst_rule,
                                                NetworkSpace domain)
{
    // TODO: keep rule_id map and do not make dependency multiset!
    DependencyPtr dependency = std::make_shared<Dependency>(src_rule,
                                                            dst_rule,
                                                            domain);
    src_rule->out_dependency_list_.push_back(dependency);
    dst_rule->in_dependency_list_.push_back(dependency);
    return dependency;
}

void DependencyUpdater::delete_dependency(DependencyPtr dependency)
{
    RulePtr src_rule = dependency->src_rule;
    RulePtr dst_rule = dependency->dst_rule;
    std::vector<DependencyPtr>& out_list = src_rule->out_dependency_list_;
    std::vector<DependencyPtr>& in_list = dst_rule->in_dependency_list_;
    
    auto is_from_src_rule = [src_rule] (const DependencyPtr& dep)
    {
        return dep->src_rule->id() == src_rule->id();
    };
    auto is_to_dst_rule = [dst_rule] (const DependencyPtr& dep)
    {
        return dep->dst_rule->id() == dst_rule->id();
    };
    
    out_list.erase(std::remove_if(out_list.begin(),
                                  out_list.end(),
                                  is_to_dst_rule),
                   out_list.end());
    in_list.erase(std::remove_if(in_list.begin(),
                                 in_list.end(),
                                 is_from_src_rule),
                  in_list.end());
}

void DependencyUpdater::add_dependencies(RulePtr src_rule,
                                         SwitchId dst_switch_id,
                                         PortId dst_port_id)
{
    // TODO: do smth with TopoId
    TopoId next_port = topology_.adjacentPort(dst_switch_id, dst_port_id);
    if (next_port.port_id) {
        // TODO: return RuleRange from inRules()
        auto in_rules = topology_.inRules(next_port.switch_id,
                                          next_port.port_id);
        add_dependencies(src_rule, in_rules);
    }
    else {
        // Port doesn't have connected link
    }
}

void DependencyUpdater::add_dependencies(RulePtr src_rule, TableId dst_table_id)
{
    TablePtr dst_table = network_.getTable(src_rule->switchId(), dst_table_id);
    auto table_rules = dst_table->rules();
    add_dependencies(src_rule, table_rules);
}

void DependencyUpdater::add_dependencies(RulePtr src_rule,
                                         std::vector<RulePtr>& dst_rules)
{
    // Get changed network state
    NetworkSpace output_domain = src_rule->outDomain();
    
    // Create dependencies
    for (auto& dst_rule : dst_rules) {
        NetworkSpace dependency_domain = dst_rule->domain() & output_domain;
        output_domain -= dependency_domain;
        
        add_dependency(src_rule, dst_rule, dependency_domain);
    }
}
// TODO: merge this two functions!
void DependencyUpdater::add_dependencies(RulePtr src_rule, RuleRange& dst_rules)
{
    // Get changed network state
    NetworkSpace output_domain = src_rule->outDomain();
    
    // Create dependencies
    for (auto& dst_rule : dst_rules) {
        NetworkSpace dependency_domain = dst_rule->domain() & output_domain;
        output_domain -= dependency_domain;
        
        add_dependency(src_rule, dst_rule, dependency_domain);
    }
}

void DependencyUpdater::add_table_dependencies(RulePtr new_rule)
{
    if (new_rule->inPort() == SpecialPort::NONE) return;
    
    TablePtr table = network_.getTable(new_rule->switchId(),
                                       new_rule->tableId());
    for (auto& lower_rule : table->lowerRules(new_rule)) {
        NetworkSpace lower_domain = new_rule->domain();

        // Update table dependencies
        auto& in_table_list = lower_rule->in_table_dependency_list_;
        for (auto& lower_table_dependency : in_table_list) {
            RulePtr upper_rule = lower_table_dependency->src_rule;
            if (upper_rule->priority() < new_rule->priority()) continue;
            
            NetworkSpace inter_domain = lower_domain & 
                                        lower_table_dependency->domain;
            lower_domain -= lower_table_dependency->domain;
            
            lower_table_dependency->domain -= inter_domain;
            if (lower_table_dependency->domain.empty())
                delete_dependency(lower_table_dependency);
            
            if (!inter_domain.empty()) {
                add_table_dependency(upper_rule, new_rule, inter_domain);
                add_table_dependency(new_rule, lower_rule, inter_domain);
            }
        }
        
        // Update rule dependencies
        auto& in_list = lower_rule->in_dependency_list_;
        for (auto& lower_dependency : in_list) {
            RulePtr prev_rule = lower_dependency->src_rule;
            
            NetworkSpace inter_domain = lower_domain & 
                                        lower_dependency->domain;
            
            lower_dependency->domain -= inter_domain;
            if (lower_dependency->domain.empty())
                delete_dependency(lower_dependency);
            
            if (!inter_domain.empty()) {
                add_dependency(prev_rule, new_rule, inter_domain);
                add_table_dependency(new_rule, lower_rule, inter_domain);
            }
        }
    }
}

void DependencyUpdater::delete_table_dependencies(RulePtr old_rule)
{
    // TODO: delete table dependencies properly
    old_rule->in_table_dependency_list_.clear();
    old_rule->out_table_dependency_list_.clear();
    old_rule->in_dependency_list_.clear();
}

void DependencyUpdater::add_out_dependencies(RulePtr new_rule)
{
    SwitchPtr sw = network_.getSwitch(new_rule->switchId());

    for (auto& action : new_rule->actions()) {
        switch (action.type) {
        case ActionType::PORT:
            switch (action.port_id) {
            case PortAction::DROP:
                add_dependency(new_rule, drop_, new_rule->outDomain());
                break;
            case PortAction::CONTROLLER:
                add_dependency(new_rule, controller_, new_rule->outDomain());
                break;
            case PortAction::IN_PORT:
            case PortAction::ALL:
                // In this case every port may be an output port
                // TODO: do it later with multiple actions and transfers
                // TODO: if new port is added, should we add new dependencies?
                for (auto& port_id : sw->ports()) {
                    add_dependencies(new_rule, new_rule->switchId(), port_id);
                }
                break;
            default: // Normal port
                // Check port existence
                // TODO: different switches may have same PortId!
                add_dependencies(new_rule,
                                 new_rule->switchId(),
                                 action.port_id);
                break;
            }
            break;
        case ActionType::TABLE:
            add_dependencies(new_rule, action.table_id);
            break;
        case ActionType::GROUP:
            break;
        default:
            // TODO: error, undefined action,
            // TODO: or may be enum class prevents this
            break;
        }
    }
}

void DependencyUpdater::delete_out_dependencies(RulePtr old_rule)
{
    // TODO: do this in a more optimal way!
    auto& out_list = old_rule->out_dependency_list_;
    for (auto& dependency : out_list) {
        auto is_from_old_rule = [old_rule] (const DependencyPtr& dep)
        {
            return dep->src_rule->id() == old_rule->id();
        };

        out_list.erase(std::remove_if(out_list.begin(),
                                      out_list.end(),
                                      is_from_old_rule),
                       out_list.end());
    }
    out_list.clear();
}

RulePtr DependencyUpdater::addRule(RulePtr new_rule)
{
    // Create dependencies from the new rule to output tables
    add_out_dependencies(new_rule);
    
    // Update input dependencies and internal table dependencies
    add_table_dependencies(new_rule);
    
    return new_rule;
}

void DependencyUpdater::deleteRule(RulePtr old_rule)
{
    delete_table_dependencies(old_rule);
    delete_out_dependencies(old_rule);
}

/*void DependencyGraph::addLink(SwitchId src_switch_id,
                              PortId src_port_id,
                              SwitchId dst_switch_id,
                              PortId dst_port_id)
{
    for (auto& src_rule : topology_.outRules(src_switch_id, src_port_id)) {
        add_dependencies(src_rule, dst_switch_id, dst_port_id);
    }
}

void DependencyGraph::deleteLink(SwitchId src_switch_id,
                                 PortId src_port_id,
                                 SwitchId dst_switch_id,
                                 PortId dst_port_id)
{
    
}*/

/*Transfer DependencyGraph::transfer(RulePtr rule,
                                   NetworkSpace in_domain = NetworkSpace())
{
    // Domain changes on SET actions iteratively
    Transfer transfer(rule->inPort());
    NetworkSpace current_domain = in_domain;
    
    for (auto& action : rule->actionList()) {
        switch (action.type) {
        case ActionType::SET:
            HeaderChanger header_changer = action.header_changer;
            current_domain = header_changer.apply(current_domain);
            transfer.add(header_changer);
            break;
        case ActionType::OUTPUT:
            switch (action.port) {
            case PortAction::DROP:
                break;
            case PortAction::CONTROLLER:
                break;
            case PortAction::IN_PORT:
            case PortAction::ALL:
                // In this case every port may be an output port
                transfer.add(dst_port = SpecialPort::ALL);
                break;
            default: // Normal port
                // Check port existence
                // TODO: Different switches may have same PortId!
                PortId next_port = topology_.adjacentPort(action.port);
                if (next_port) {
                    transfer.add(dst_port = next_port);
                    addDependencies(rule, topology_.inRules(next_port),
                                    transfer);
                }
                else {
                    // Port doesn't have connected link
                    disconnected_out_port_map_.emplace({action.port,
                                                        transfer});
                }
                break;
            }
            break;
        case ActionType::TABLE:
            addDependencies(rule, action.table_id, transfer);
            break;
        case ActionType::GROUP:
            break;
        default:
            // TODO: Error, undefined action,
            // or may be enum class prevents this
            break;
        }
    }
}

RulePtr getRule(vertex_descriptor vertex)
{
    return 
}

DependencyRange inTableDependencies(RulePtr rule)
{
    Graph& graph = table_dependency_graph_;
    vertex_descriptor& vertex = rule->table_dependency_vertex_;
    return DependencyRange(graph, boost::in_edges(vertex, graph));
}

DependencyRange outTableDependencies(RulePtr rule)
{
    Graph& graph = table_dependency_graph_;
    vertex_descriptor& vertex = rule->table_dependency_vertex_;
    return DependencyRange(graph, boost::out_edges(vertex, graph));
}

DependencyRange inDependencies(RulePtr rule)
{
    Graph& graph = rule_dependency_graph_;
    vertex_descriptor& vertex = rule->rule_dependency_vertex_;
    return DependencyRange(graph, boost::in_edges(vertex, graph));
}

DependencyRange outDependencies(RulePtr rule)
{
    Graph& graph = rule_dependency_graph_;
    vertex_descriptor& vertex = rule->rule_dependency_vertex_;
    return DependencyRange(graph, boost::out_edges(vertex, graph));
}
*/
