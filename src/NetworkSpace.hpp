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
    NetworkSpace();
    explicit NetworkSpace(PortId in_port);
    explicit NetworkSpace(const HeaderSpace& header);
    NetworkSpace(PortId in_port, const HeaderSpace& header);
    
    PortId inPort() const {return in_port_;}
    HeaderSpace header() const {return header_;}
    bool empty() const {
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
    Transfer();
    explicit Transfer(const HeaderChanger& header_changer);
    Transfer(PortId src_port, PortId dst_port,
             const HeaderChanger& header_changer);
    
    NetworkSpace apply(NetworkSpace domain) const;
    NetworkSpace inverse(NetworkSpace domain) const;

    HeaderChanger headerChanger() const {return header_changer_;}

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
    GROUP
};

struct Action
{
    ActionType type;
    Transfer transfer;
    PortId port_id;
    TableId table_id;
    GroupId group_id;
};
