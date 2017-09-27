#pragma once

#include "./header_space/HeaderSpace.hpp"

#include <map>
#include <memory>
#include <vector>

typedef uint64_t SwitchId;
typedef uint32_t PortId;
typedef uint8_t TableId;
typedef uint32_t GroupId;
typedef uint64_t RuleId;

class Transfer;

// TODO: maybe move to NetworkFwd.hpp
class Switch;
class Table;
class Rule;

typedef std::shared_ptr<Switch> SwitchPtr;
typedef std::shared_ptr<Table> TablePtr;
typedef std::shared_ptr<Rule> RulePtr;

/* Port numbering. Ports are numbered starting from 1. */
enum ReservedPort: PortId {
    /* Fake output "port" that represent packet drop */
    DROP            = 0x00000000,
    
    /* Maximum number of physical and logical switch ports. */
    //MAX        = 0xffffff00,

    /* Reserved OpenFlow Port (fake output "ports"). */
    //UNSET      = 0xfffffff7,  /* Output port not set in action-set.
    //                             used only in OXM_OF_ACTSET_OUTPUT. */
    IN_PORT    = 0xfffffff8,  /* Send the packet out the input port.  This
                                 reserved port must be explicitly used
                                 in order to send back out of the input
                                 port. */
    //TABLE      = 0xfffffff9,  /* Submit the packet to the first flow table
    //                             NB: This destination port can only be
    //                             used in packet-out messages. */
    //NORMAL     = 0xfffffffa,  /* Forward using non-OpenFlow pipeline. */
    //FLOOD      = 0xfffffffb,  /* Flood using non-OpenFlow pipeline. */
    ALL        = 0xfffffffc,  /* All standard ports except input port. */
    CONTROLLER = 0xfffffffd  /* Send to controller. */
    //LOCAL      = 0xfffffffe,  /* Local openflow "port". */
    //ANY        = 0xffffffff   /* Special value used in some requests when
    //                             no port is specified (i.e. wildcarded). */
};

struct Match
{
    Match(HeaderSpace _header):
        in_port(ReservedPort::ALL),
        header(_header) {}
    Match(PortId _in_port, HeaderSpace _header):
        in_port(_in_port),
        header(_header) {}
    
    PortId in_port;
    HeaderSpace header;
};

enum class ActionType
{
    SET,
    PORT,
    GROUP,
    TABLE,
    CONTROLLER,
    DROP
};

struct Action
{
    ActionType action_type;
};

struct SetAction: public Action
{
    HeaderChanger header_changer;
};

struct PortAction: public Action
{
    PortId port;
};

struct TableAction: public Action
{
    TableId table_id;
};

struct GroupAction: public Action
{
    GroupId group_id;
};
