#pragma once

#include "header_space/HeaderSpace.hpp"
#include "Types.hpp"

enum SpecialPort: PortId
{
    NONE       = 0x00000000,
    ALL        = 0xFFFFFFFC,
    CONTROLLER = 0xFFFFFFFD,
    LOCAL      = 0xFFFFFFFE,
    ANY        = 0xFFFFFFFF
};

class Match
{
public:
    explicit Match(PortId in_port);
    explicit Match(BitMask&& header);
    Match(PortId in_port, BitMask&& header);
    Match(const Match&) = default;
    Match(Match&&) = default;
    static Match wholeSpace();

    Match& operator=(const Match& other) = default;
    Match& operator=(Match&& other) noexcept = default;

    bool operator==(const Match& other) const;
    bool operator<=(const Match& other) const;
    bool operator>=(const Match& other) const;

    PortId inPort() const {return in_port_;}
    BitMask header() const {return header_;}

    friend class NetworkSpace;

private:
    PortId in_port_;
    BitMask header_;

};

class NetworkSpace
{
public:
    explicit NetworkSpace(std::string str);
    explicit NetworkSpace(PortId in_port);
    explicit NetworkSpace(const HeaderSpace& header);
    explicit NetworkSpace(const Match& match);
    explicit NetworkSpace(Match&& match);
    NetworkSpace(PortId in_port, const HeaderSpace& header);
    NetworkSpace(PortId in_port, HeaderSpace&& header);
    NetworkSpace(const NetworkSpace& other) = default;
    NetworkSpace(NetworkSpace&& other) noexcept = default;
    static NetworkSpace emptySpace();
    static NetworkSpace wholeSpace();

    PortId inPort() const {return in_port_;}
    HeaderSpace header() const {return header_;}
    Match match() const;
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

    std::string toString() const;
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
    static Transfer portTransfer(PortId dst_port);

    Transfer& operator=(const Transfer& other) = default;
    Transfer& operator=(Transfer&& other) = default;

    void dstPort(PortId dst_port) {dst_port_ = dst_port;}

    // Transfer superposition
    Transfer operator*=(const Transfer& right);
    Transfer operator*(const Transfer& right) const;
    
    NetworkSpace apply(NetworkSpace domain) const;
    NetworkSpace inverse(NetworkSpace domain) const;

    std::string toString() const;
    friend std::ostream& operator<<(std::ostream& os,
                                    const Transfer& transfer);

private:
    PortId src_port_;
    PortId dst_port_;
    HeaderChanger header_changer_;

};
