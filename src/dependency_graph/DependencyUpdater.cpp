#include "DependencyUpdater.hpp"

DependencyUpdater::DependencyUpdater(Network& network, Topology& topology):
    network_(network), topology_(topology)
{

}

DependencyPtr DependencyUpdater::add_table_dependency(RulePtr src_rule,
                                                      RulePtr dst_rule,
                                                      NetworkSpace domain)
{
    // TODO: keep rule_id map and do not make dependency multiset!
    DependencyPtr dependency = std::make_shared<Dependency>(src_rule,
                                                            dst_rule,
                                                            domain);
    // TODO: push_front is a hack for iteration and addition at the same time!
    src_rule->out_table_dependency_list_.push_front(dependency);
    dst_rule->in_table_dependency_list_.push_front(dependency);
    return dependency;
}

DependencyPtr DependencyUpdater::add_dependency(RulePtr src_rule,
                                                RulePtr dst_rule)
{
    add_dependency(src_rule, dst_rule, src_rule->outDomain());
}

DependencyPtr DependencyUpdater::add_dependency(RulePtr src_rule,
                                                RulePtr dst_rule,
                                                NetworkSpace domain)
{
    // TODO: keep rule_id map and do not make dependency multiset!
    DependencyPtr dependency = std::make_shared<Dependency>(src_rule,
                                                            dst_rule,
                                                            domain);
    // TODO: push_front is a hack for iteration and addition at the same time!
    src_rule->out_dependency_list_.push_front(dependency);
    dst_rule->in_dependency_list_.push_front(dependency);
    return dependency;
}

void DependencyUpdater::delete_dependency(DependencyPtr dependency)
{
    RulePtr src_rule = dependency->src_rule;
    RulePtr dst_rule = dependency->dst_rule;
    std::list<DependencyPtr>& out_list = src_rule->out_dependency_list_;
    std::list<DependencyPtr>& in_list = dst_rule->in_dependency_list_;

    // DEBUG LOG
    std::cout<<"Delete dependency "<<src_rule->id()<<"->"<<dst_rule->id()
             <<" ("<<dependency->domain.header()<<")"<<std::endl;
    
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
                                         SwitchId src_switch_id,
                                         PortId src_port_id)
{
    // DEBUG LOG
    std::cout<<"    From "<<src_switch_id<<":"<<src_port_id<<" to ";

    // TODO: do something with TopoId
    TopoId next_port = topology_.adjacentPort(src_switch_id, src_port_id);
    if (next_port.port_id) {
        // DEBUG LOG
        std::cout<<next_port.switch_id<<":"<<next_port.port_id<<std::endl;

        // TODO: return RuleRange from inRules()
        auto dst_rules = topology_.inRules(next_port.switch_id,
                                           next_port.port_id);
        add_dependencies(src_rule, dst_rules);
    }
    else {
        // DEBUG LOG
        std::cout<<"nowhere"<<std::endl;

        // Port doesn't have connected link, connect to the sink rule
        ///RulePtr sink_rule = network_.getSinkRule(src_switch_id, src_port_id);
        ///add_dependency(src_rule, sink_rule);
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

        // DEBUG LOG
        std::cout<<"        "<<src_rule->id()<<"->"<<dst_rule->id()
                 <<" ("<<dependency_domain.header()<<")"<<std::endl;
        
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

        // DEBUG LOG
        std::cout<<"        "<<src_rule->id()<<"->"<<dst_rule->id()
                 <<" ("<<dependency_domain.header()<<")"<<std::endl;
        
        add_dependency(src_rule, dst_rule, dependency_domain);
    }
}

void DependencyUpdater::add_table_dependencies(RulePtr new_rule)
{
    if (new_rule->inPort() == SpecialPort::NONE) return;

    // DEBUG LOG
    std::cout<<"Add table dependencies "<<new_rule->id()
             <<" ("<<new_rule->domain().header()<<")"<<std::endl;
    
    TablePtr table = network_.getTable(new_rule->switchId(),
                                       new_rule->tableId());
    for (auto& lower_rule : table->lowerRules(new_rule)) {
        NetworkSpace lower_domain = new_rule->domain();
        // TODO: delete empty dependencies in cycle (do not save and delete)
        std::vector<DependencyPtr> empty_dependencies;

        // DEBUG LOG
        std::cout<<"    Update table dependencies "
                 <<lower_rule->id()
                 <<" ("<<lower_rule->domain().header()<<")"<<std::endl;

        // Update old table dependencies
        auto& in_table_list = lower_rule->in_table_dependency_list_;
        //for (auto& lower_table_dependency : in_table_list) {
        for (auto dependency_it = in_table_list.begin();
             dependency_it != in_table_list.end();) {
            auto& lower_table_dependency = *dependency_it++;
            RulePtr upper_rule = lower_table_dependency->src_rule;
            if (upper_rule->priority() < new_rule->priority()) continue;
            
            NetworkSpace traverse_domain = lower_domain &
                                           lower_table_dependency->domain;
            ///lower_domain -= lower_table_dependency->domain;

            // DEBUG LOG
            std::cout<<"        "<<upper_rule->id()<<"->"
                     <<new_rule->id()<<"->"<<lower_rule->id()
                     <<" ("<<traverse_domain.header()<<")"
                     <<") # "<<upper_rule->id()<<"->"
                     <<lower_table_dependency->dst_rule->id()
                     <<" ("<<lower_table_dependency->domain.header()<<")"
                     <<std::endl;
            
            lower_table_dependency->domain -= traverse_domain;

            // DEBUG LOG
            std::cout<<"        / ("<<lower_domain.header()
                     <<")"<<std::endl;

            if (lower_table_dependency->domain.empty())
                empty_dependencies.push_back(lower_table_dependency);
            
            if (!traverse_domain.empty()) {
                add_table_dependency(upper_rule, new_rule, traverse_domain);
                ///add_table_dependency(new_rule, lower_rule, inter_domain);
            }
        }

        // DEBUG LOG
        std::cout<<"    ----/ ("<<lower_domain.header()
                 <<")"<<std::endl;

        // DEBUG LOG
        std::cout<<"    Update rule dependencies "
                 <<lower_rule->id()
                 <<" ("<<lower_rule->domain().header()<<")"<<std::endl;
        
        // Update rule dependencies
        auto& in_list = lower_rule->in_dependency_list_;
        //for (auto& lower_dependency : in_list) {
        for (auto dependency_it = in_list.begin();
             dependency_it != in_list.end();) {
            auto& lower_dependency = *dependency_it++;
            RulePtr prev_rule = lower_dependency->src_rule;

            NetworkSpace incoming_domain = lower_domain &
                                           lower_dependency->domain;

            // DEBUG LOG
            std::cout<<"        "<<new_rule->id()<<"->"<<lower_rule->id()
                     <<" ("<<incoming_domain.header()
                     <<") # "<<prev_rule->id()<<"->"
                     <<lower_dependency->dst_rule->id()
                     <<" ("<<lower_dependency->domain.header()<<")"
                     <<std::endl;
            
            lower_dependency->domain -= incoming_domain;

            // DEBUG LOG
            std::cout<<"        / ("<<lower_domain.header()
                     <<")"<<std::endl;

            if (lower_dependency->domain.empty())
                empty_dependencies.push_back(lower_dependency);
            
            if (!incoming_domain.empty()) {
                add_dependency(prev_rule, new_rule, incoming_domain);
                ///add_table_dependency(new_rule, lower_rule, inter_domain);
            }
        }

        // Create new table dependency
        NetworkSpace inter_domain = lower_domain & lower_rule->domain();
        add_table_dependency(new_rule, lower_rule, inter_domain);
        lower_domain -= lower_rule->domain();

        // Delete empty dependencies
        for (auto& empty_dependency : empty_dependencies) {
            // DEBUG LOG
            auto in_port = empty_dependency->domain.inPort();
            auto header = empty_dependency->domain.header();
            std::cout<<"Port empty = "<<(in_port == SpecialPort::NONE)
                     <<" | Header empty = "<<header.empty()<<std::endl;

            delete_dependency(empty_dependency);
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
    SwitchId switch_id = new_rule->switchId();

    // DEBUG LOG
    std::cout<<"Out dependencies "<<new_rule->id()
             <<" ("<<new_rule->domain().header()<<")"<<std::endl;

    for (auto& action : new_rule->actions()) {
        switch (action.type) {
        case ActionType::PORT:
            switch (action.port_id) {
            case PortAction::DROP:
                ///add_dependency(new_rule, drop_, new_rule->outDomain());
                break;
            case PortAction::CONTROLLER:
                ///add_dependency(new_rule, controller_, new_rule->outDomain());
                break;
            case PortAction::IN_PORT:
            case PortAction::ALL:
                // In this case every port may be an output port
                // TODO: do it later with multiple actions and transfers
                // TODO: if new port is added, should we add new dependencies?
                for (auto& port_id : sw->ports()) {
                    add_dependencies(new_rule, switch_id, port_id);
                }
                break;
            default: // Normal port
                // Check port existence
                add_dependencies(new_rule, switch_id, action.port_id);
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

void DependencyUpdater::addLink(SwitchId src_switch_id, PortId src_port_id,
                                SwitchId dst_switch_id, PortId dst_port_id)
{
    // TODO: check if return value is a reference
    auto src_rules = topology_.outRules(src_switch_id, src_port_id);
    for (auto& src_rule : src_rules) {
        auto dst_rules = topology_.inRules(dst_switch_id, dst_port_id);
        add_dependencies(src_rule, dst_rules);
    }
}

void DependencyUpdater::deleteLink(SwitchId src_switch_id, PortId src_port_id,
                                   SwitchId dst_switch_id, PortId dst_port_id)
{

}

std::map<std::pair<RuleId, RuleId>, DependencyPtr>
DependencyUpdater::dependencies()
{
    // TODO: refactor this in a more optimal way! (or maybe not)
    std::map<std::pair<RuleId, RuleId>, DependencyPtr> dependency_map;
    for (const auto& rule : network_.rules()) {
        for (const auto& dependency : rule->in_table_dependency_list_) {
            auto src_rule_id = dependency->src_rule->id();
            auto dst_rule_id = dependency->dst_rule->id();
            dependency_map[{src_rule_id, dst_rule_id}] = dependency;
        }
        for (const auto& dependency : rule->out_table_dependency_list_) {
            auto src_rule_id = dependency->src_rule->id();
            auto dst_rule_id = dependency->dst_rule->id();
            dependency_map[{src_rule_id, dst_rule_id}] = dependency;
        }
        for (const auto& dependency : rule->in_dependency_list_) {
            auto src_rule_id = dependency->src_rule->id();
            auto dst_rule_id = dependency->dst_rule->id();
            dependency_map[{src_rule_id, dst_rule_id}] = dependency;
        }
        for (const auto& dependency : rule->out_dependency_list_) {
            auto src_rule_id = dependency->src_rule->id();
            auto dst_rule_id = dependency->dst_rule->id();
            dependency_map[{src_rule_id, dst_rule_id}] = dependency;
        }
    }
    return dependency_map;
}
