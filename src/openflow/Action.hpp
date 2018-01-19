#pragma once

#include "Types.hpp"
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
        port_type(port_type), port_id(port_id) {
        assert(port_type != PortType::NORMAL || port_id != SpecialPort::NONE);
    }
    PortActionBase(PortId port_id):
        PortActionBase(Transfer::identityTransfer(),
                       PortType::NORMAL, port_id) {}

    PortActionBase(const PortActionBase& other) = default;
    PortActionBase(PortActionBase&& other) noexcept = default;

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
    TableActionBase(TableId table_id):
        TableActionBase(Transfer::identityTransfer(), table_id) {}

    TableActionBase(const TableActionBase& other) = default;
    TableActionBase(TableActionBase&& other) noexcept = default;

    TableId table_id;
};

struct GroupActionBase : public Action
{
    GroupActionBase(Transfer transfer, GroupId group_id):
        Action(ActionType::GROUP, std::move(transfer)),
        group_id(group_id) {}
    GroupActionBase(const GroupActionBase& other) = default;
    GroupActionBase(GroupActionBase&& other) noexcept = default;

    GroupId group_id;
};

struct ActionsBase
{
    std::vector<PortActionBase> port_actions;
    std::vector<TableActionBase> table_actions;
    std::vector<GroupActionBase> group_actions;

    static ActionsBase dropAction() {
        ActionsBase actions;
        actions.port_actions.emplace_back(PortActionBase::dropAction());
        return actions;
    }
    static ActionsBase controllerAction() {
        ActionsBase actions;
        actions.port_actions.emplace_back(PortActionBase::controllerAction());
        return actions;
    }
    static ActionsBase portAction(PortId port_id) {
        ActionsBase actions;
        actions.port_actions.emplace_back(port_id);
        return actions;
    }
    static ActionsBase tableAction(TableId table_id) {
        ActionsBase actions;
        actions.port_actions.emplace_back(table_id);
        return actions;
    }
};
