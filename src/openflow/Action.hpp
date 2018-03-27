#pragma once

#include "../Types.hpp"
#include "../NetworkSpace.hpp"

#include <vector>

enum class ActionType
{
    PORT,
    TABLE,
    GROUP
};

enum class PortType
{
    NORMAL,
    DROP,
    IN_PORT,
    ALL,
    CONTROLLER
};

struct Action
{
    Action(const Action& other) = default;
    Action(Action&& other) noexcept = default;

    Action& operator=(const Action& other) = default;
    Action& operator=(Action&& other) noexcept = default;

    ActionType type;
    Transfer transfer;

protected:
    Action(ActionType type, Transfer transfer):
        type(type), transfer(std::move(transfer)) {}
};

struct PortActionBase : public Action
{
    PortActionBase(Transfer transfer, PortType port_type,
                   PortId port_id = SpecialPort::NONE):
        Action(ActionType::PORT, std::move(transfer)),
        port_type(port_type), port_id(port_id)
    {
        assert(port_type != PortType::NORMAL || port_id != SpecialPort::NONE);
        transfer.dstPort(port_id);
    }
    explicit PortActionBase(PortId port_id):
        PortActionBase(Transfer::portTransfer(port_id),
                       PortType::NORMAL, port_id) {}

    PortActionBase(const PortActionBase& other) = default;
    PortActionBase(PortActionBase&& other) noexcept = default;

    PortActionBase& operator=(const PortActionBase& other) = default;
    PortActionBase& operator=(PortActionBase&& other) noexcept = default;

    static PortActionBase dropAction() {
        return {Transfer::identityTransfer(), PortType::DROP};
    }
    static PortActionBase controllerAction() {
        return {Transfer::identityTransfer(), PortType::CONTROLLER};
    }

    PortType port_type;
    PortId port_id;
};

struct TableActionBase : public Action
{
    TableActionBase(Transfer transfer, TableId table_id):
        Action(ActionType::TABLE, std::move(transfer)),
        table_id(table_id) {}
    explicit TableActionBase(TableId table_id):
        TableActionBase(Transfer::identityTransfer(), table_id) {}

    TableActionBase(const TableActionBase& other) = default;
    TableActionBase(TableActionBase&& other) noexcept = default;

    TableActionBase& operator=(const TableActionBase& other) = default;
    TableActionBase& operator=(TableActionBase&& other) noexcept = default;

    TableId table_id;
};

struct GroupActionBase : public Action
{
    GroupActionBase(Transfer transfer, GroupId group_id):
        Action(ActionType::GROUP, std::move(transfer)),
        group_id(group_id) {}

    GroupActionBase(const GroupActionBase& other) = default;
    GroupActionBase(GroupActionBase&& other) noexcept = default;

    GroupActionBase& operator=(const GroupActionBase& other) = default;
    GroupActionBase& operator=(GroupActionBase&& other) noexcept = default;

    GroupId group_id;
};

struct ActionsBase
{
    std::vector<PortActionBase> port_actions;
    std::vector<GroupActionBase> group_actions;
    std::vector<TableActionBase> table_actions;
    // TODO: consider making one table action - since it can be only one
    //TableActionBase table_action;

    void operator+=(ActionsBase&& other) {
        port_actions.insert(
            port_actions.end(),
            std::make_move_iterator(other.port_actions.begin()),
            std::make_move_iterator(other.port_actions.end())
        );
        table_actions.insert(
            table_actions.end(),
            std::make_move_iterator(other.table_actions.begin()),
            std::make_move_iterator(other.table_actions.end())
        );
        group_actions.insert(
            group_actions.end(),
            std::make_move_iterator(other.group_actions.begin()),
            std::make_move_iterator(other.group_actions.end())
        );
    }

    bool empty() const {
        return port_actions.empty() &&
               group_actions.empty() &&
               table_actions.empty();
    }

    static ActionsBase dropAction() {
        ActionsBase actions;
        actions.port_actions.emplace_back(PortActionBase::dropAction());
        return std::move(actions);
    }
    static ActionsBase controllerAction() {
        ActionsBase actions;
        actions.port_actions.emplace_back(PortActionBase::controllerAction());
        return std::move(actions);
    }
    static ActionsBase portAction(PortId port_id) {
        ActionsBase actions;
        actions.port_actions.emplace_back(port_id);
        return std::move(actions);
    }
    static ActionsBase tableAction(TableId table_id) {
        ActionsBase actions;
        actions.table_actions.emplace_back(table_id);
        return std::move(actions);
    }
};
