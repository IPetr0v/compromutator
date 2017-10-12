#pragma once

#include "./header_space/HeaderSpace.hpp"

typedef uint64_t SwitchId;
typedef uint8_t TableId;
//typedef uint32_t GroupId;
typedef uint64_t RuleId;

typedef uint32_t PortId;

enum SpecialPort: PortId
{
    NONE = 0x00000000,
    ANY  = 0xFFFFFFFF
};

class NetworkSpace
{
public:
    NetworkSpace(HeaderSpace header):
        in_port_(SpecialPort::ALL),
        header_(header) {}
    NetworkSpace(PortId in_port, HeaderSpace header):
        in_port_(in_port),
        header_(header) {}
    
    inline const PortId inPort() {return !is_empty_ ? in_port_ : SpecialPort::NONE;}
    // TODO: check correctness of the empty HeaderSpace
    // TODO: check for emptiness in the HeaderSpace class
    inline const HeaderSpace& header() {return !is_empty_ ? header_ : ~HeaderSpace(header_.length());}
    inline bool& isEmpry() {return is_empty_;}

private:
    PortId in_port_;
    HeaderSpace header_;
    bool is_empty_;

};

class Transfer
{
public:
    Transfer(PortId src_port, PortId dst_port,
             HeaderChanger header_changer);
    
    NetworkSpace apply(NetworkSpace domain);
    NetworkSpace inverse(NetworkSpace domain);

private:
    PortId src_port_;
    PortId dst_port_;
    HeaderChanger header_changer_;

}

enum PortAction: PortId
{
    DROP       = 0x00000000,
    IN_PORT    = 0xfffffff8,
    ALL        = 0xfffffffc,
    CONTROLLER = 0xfffffffd
};

enum class ActionType
{
    PORT,
    TABLE,
    GROUP,
    CONTROLLER,
    DROP
};

struct Action
{
    ActionType type;
    Transfer transfer;
    PortId port_id;
    TableId table_id;
    GroupId group_id;
};
