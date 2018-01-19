#pragma once

#include "header_space/HeaderSpace.hpp"
#include "openflow/Types.hpp"

enum SpecialPort: PortId
{
    NONE = 0x00000000,
    ANY  = 0xFFFFFFFF
};

class NetworkSpace
{
public:
    explicit NetworkSpace(std::string str);
    explicit NetworkSpace(PortId in_port);
    explicit NetworkSpace(const HeaderSpace& header);
    NetworkSpace(PortId in_port, const HeaderSpace& header);
    NetworkSpace(const NetworkSpace& other) = default;
    NetworkSpace(NetworkSpace&& other) noexcept = default;
    static NetworkSpace emptySpace();
    static NetworkSpace wholeSpace();

    PortId inPort() const {return in_port_;}
    HeaderSpace header() const {return header_;}
    bool empty() const {
        return in_port_ == SpecialPort::NONE || header_.empty();
    }

    NetworkSpace& operator=(const NetworkSpace& other) = default;
    NetworkSpace& operator=(NetworkSpace&& other) noexcept = default;

    bool operator==(const NetworkSpace& other) const {
        return in_port_ == other.in_port_ && header_ == other.header_;
    }
    bool operator!=(const NetworkSpace &other) const {
        return !(*this == other);
    }

    NetworkSpace& operator+=(const NetworkSpace& right);
    NetworkSpace& operator-=(const NetworkSpace& right);
    NetworkSpace operator+(const NetworkSpace& right);
    NetworkSpace operator-(const NetworkSpace& right);
    NetworkSpace operator&(const NetworkSpace& right);

    friend std::ostream& operator<<(std::ostream& os,
                                    const NetworkSpace& domain);

private:
    PortId in_port_;
    HeaderSpace header_;

};

class Transfer
{
public:
    explicit Transfer(const HeaderChanger& header_changer);
    Transfer(PortId src_port, PortId dst_port,
             HeaderChanger&& header_changer);
    Transfer(const Transfer& other) = default;
    Transfer(Transfer&& other) noexcept = default;
    static Transfer identityTransfer();

    Transfer& operator=(const Transfer& other) = default;
    Transfer& operator=(Transfer&& other) = default;

    // Transfer superposition
    Transfer operator*(const Transfer& right) const;
    
    NetworkSpace apply(NetworkSpace domain) const;
    NetworkSpace inverse(NetworkSpace domain) const;

private:
    PortId src_port_;
    PortId dst_port_;
    HeaderChanger header_changer_;

};
