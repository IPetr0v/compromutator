#pragma once

#include "Types.hpp"
#include "Vertex.hpp"
#include "../flow_predictor/Node.hpp"
#include "../openflow/Action.hpp"
#include "../NetworkSpace.hpp"

#include <iostream>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct PortAction : public PortActionBase
{
    PortAction(PortActionBase&& base, PortPtr port = nullptr):
        PortActionBase(std::move(base)), port(port) {
        // If the port type is normal then the getPort pointer should not be null
        assert(port_type != PortType::NORMAL || port != nullptr);
    }
    PortAction(const PortAction& other) = default;
    PortAction(PortAction&& other) noexcept = default;

    PortPtr port;
};

struct TableAction : public TableActionBase
{
    explicit TableAction(TableActionBase&& base,
                         TablePtr table = TablePtr(nullptr)):
        TableActionBase(std::move(base)), table(table) {
        //assert(table != nullptr);
    }
    TableAction(const TableAction& other) = default;
    TableAction(TableAction&& other) noexcept = default;

    TablePtr table;
};

struct GroupAction : public GroupActionBase
{
    GroupAction(GroupActionBase&& base, GroupPtr group):
        GroupActionBase(std::move(base)), group(group) {
        assert(group != nullptr);
    }
    GroupAction(const GroupAction& other) = default;
    GroupAction(GroupAction&& other) noexcept = default;

    GroupPtr group;
};

struct Actions
{
    std::vector<PortAction> port_actions;
    std::vector<TableAction> table_actions;
    std::vector<GroupAction> group_actions;

    std::shared_ptr<PortAction> getPortAction(PortId port_id) const;

    bool empty() const {
        return port_actions.empty() &&
               table_actions.empty() &&
               group_actions.empty();
    }
    uint64_t size() const {
        return port_actions.size() +
               table_actions.size() +
               group_actions.size();
    }

    static Actions dropAction() {
        Actions actions;
        actions.port_actions.emplace_back(PortActionBase::dropAction());
        return std::move(actions);
    }
    static Actions controllerAction() {
        Actions actions;
        actions.port_actions.emplace_back(PortActionBase::controllerAction());
        return std::move(actions);
    }
    static Actions portAction(PortId port_id, PortPtr port) {
        Actions actions;
        actions.port_actions.emplace_back(
            PortAction(PortActionBase(port_id), port));
        return std::move(actions);
    }

    static Actions tableAction(TableId table_id,
                               TablePtr table = TablePtr(nullptr)) {
        Actions actions;
        actions.table_actions.emplace_back(
            TableAction(TableActionBase(table_id), table));
        return std::move(actions);
    }

    static Actions noActions() {
        return Actions();
    }
};

enum class RuleType
{
    FLOW,
    GROUP,
    BUCKET,
    SOURCE,
    SINK
};

struct Descriptor;
struct RuleInfo;
using RuleInfoPtr = std::shared_ptr<RuleInfo>;

class Rule
{
public:
    Rule(RuleType type, SwitchPtr sw, TablePtr table, Priority priority,
         Cookie cookie, Match&& match, Actions&& actions);
    Rule(RuleType type, SwitchPtr sw, TablePtr table, Priority priority,
         Cookie cookie, NetworkSpace&& domain, Actions&& actions);
    Rule(const RulePtr other, Cookie cookie, const NetworkSpace& domain);
    ~Rule();

    RuleInfoPtr info() const;

    RuleType type() const {return type_;}
    RuleId id() const {return id_;}
    SwitchPtr sw() const {return sw_;}
    TablePtr table() const {return table_;}

    Priority priority() const {return priority_;}
    Cookie cookie() const {return cookie_;}
    Match match() const {return match_;}
    NetworkSpace domain() const {return domain_;}
    PortId inPort() const {return domain_.inPort();}
    const Actions& actions() const {return actions_;}
    ActionsBase actionsBase() const;
    uint64_t multiplier() const {return actions_.size();}

    bool isTableMiss() const;

    size_t hash() const;

    std::string toString() const;
    friend std::ostream& operator<<(std::ostream& os, const Rule& rule);
    friend std::ostream& operator<<(std::ostream& os, const RulePtr rule);

    struct PtrComparator {
        bool operator()(RulePtr first, RulePtr second) const {
            return first->id_ < second->id_;
        }
    };
    struct PtrEqualityComparator {
        bool operator()(RulePtr first, RulePtr second) const {
            return first->id_ == second->id_;
        }
    };
    struct PtrHash {
        size_t operator()(RulePtr rule) const {
            return rule->hash();
        }
    };

private:
    RuleType type_;
    RuleId id_;
    TablePtr table_;
    SwitchPtr sw_;

    Priority priority_;
    Cookie cookie_;
    Match match_;
    NetworkSpace domain_;
    Actions actions_;

    static IdGenerator<uint64_t> id_generator_;

    friend class DependencyGraph;
    friend class PathScan;
    VertexPtr vertex_;
    RuleMappingDescriptor rule_mapping_;
};

struct RuleInfo
{
    RuleInfo(SwitchId switch_id, TableId table_id, Priority priority,
             Cookie cookie, Match match, ActionsBase actions,
             RuleId rule_id = {0, 0, 0, 0});

    SwitchId switch_id;
    TableId table_id;
    Priority priority;
    Cookie cookie;
    Match match;
    ActionsBase actions;

    bool operator==(const RuleInfo& other) const;
    friend std::ostream& operator<<(std::ostream& os, const RuleInfo& rule);
private:
    RuleId rule_id_;
};
