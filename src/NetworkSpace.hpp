#pragma once

#include "./header_space/HeaderSpace.hpp"
#include "Common.hpp"

typedef uint32_t PortId;

enum SpecialPort: PortId
{
    NONE = 0x00000000,
    ANY  = 0xFFFFFFFF
};

class NetworkSpace
{
public:
    NetworkSpace(HeaderSpace header);
    NetworkSpace(PortId in_port, HeaderSpace header);
    
    inline PortId inPort() const {return in_port_;}
    inline HeaderSpace header() const {return header_;}
    inline bool empty() const {
        return in_port_ == SpecialPort::NONE || header_.empty();
    }
    
    NetworkSpace& operator-=(const NetworkSpace& right);
    NetworkSpace operator&(const NetworkSpace& right);

private:
    PortId in_port_;
    HeaderSpace header_;

};

class Transfer
{
public:
    Transfer(PortId src_port, PortId dst_port,
             HeaderChanger header_changer);
    
    NetworkSpace apply(NetworkSpace domain) const;
    NetworkSpace inverse(NetworkSpace domain) const;

private:
    PortId src_port_;
    PortId dst_port_;
    HeaderChanger header_changer_;

};

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
