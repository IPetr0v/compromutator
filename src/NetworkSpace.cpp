#include "NetworkSpace.hpp"

#include <string>

std::string getPortString(PortId port_id)
{
    std::string port_string;
    switch(port_id) {
    case SpecialPort::NONE:
        port_string = "NONE";
        break;
    case SpecialPort::ANY:
        port_string = "ANY";
        break;
    default:
        port_string = std::to_string(port_id);
        break;
    }
    return port_string;
}

Match::Match(PortId in_port, BitMask&& header):
    in_port_(in_port), header_(std::move(header))
{

}

bool Match::operator==(const Match& other) const
{
    return in_port_ == other.in_port_ && header_ == other.header_;
}

bool Match::operator<=(const Match& other) const
{
    // Ports are not comparable (can be only equal or non-equal)
    return in_port_ == other.in_port_ && header_ <= other.header_;
}

bool Match::operator>=(const Match& other) const
{
    return in_port_ == other.in_port_ && header_ >= other.header_;
}

NetworkSpace::NetworkSpace(std::string str):
    in_port_(SpecialPort::ANY), header_(std::move(str))
{

}

NetworkSpace::NetworkSpace(PortId in_port):
    in_port_(in_port),
    header_(HeaderSpace::wholeSpace(HeaderSpace::GLOBAL_LENGTH))
{

}

NetworkSpace::NetworkSpace(const HeaderSpace& header):
    in_port_(SpecialPort::ANY), header_(header)
{
    
}

NetworkSpace::NetworkSpace(Match&& match):
    in_port_(match.in_port_), header_(std::move(match.header_))
{

}

NetworkSpace::NetworkSpace(PortId in_port, const HeaderSpace& header):
    in_port_(in_port), header_(header)
{
    
}

NetworkSpace::NetworkSpace(PortId in_port, HeaderSpace&& header):
    in_port_(in_port), header_(std::move(header))
{

}

NetworkSpace NetworkSpace::emptySpace()
{
    return NetworkSpace(
        SpecialPort::NONE,
        HeaderSpace::emptySpace(HeaderSpace::GLOBAL_LENGTH)
    );
}

NetworkSpace NetworkSpace::wholeSpace()
{
    // We represent empty network space as any getPort with an empty header
    return NetworkSpace(
        SpecialPort::ANY,
        HeaderSpace::wholeSpace(HeaderSpace::GLOBAL_LENGTH)
    );
}

NetworkSpace& NetworkSpace::operator+=(const NetworkSpace& right)
{
    header_ += right.header_;
    return *this;
}

NetworkSpace& NetworkSpace::operator-=(const NetworkSpace& right)
{
    header_ -= right.header_;
    return *this;
}

NetworkSpace NetworkSpace::operator+(const NetworkSpace& right)
{
    NetworkSpace domain(in_port_, header());
    domain += right;
    return domain;
}

NetworkSpace NetworkSpace::operator-(const NetworkSpace& right)
{
    NetworkSpace domain(in_port_, header());
    domain -= right;
    return domain;
}

NetworkSpace NetworkSpace::operator&(const NetworkSpace& right)
{
    // TODO: make it simpler (remove multiple if)
    PortId new_in_port;
    if (in_port_ == SpecialPort::NONE ||
        right.in_port_ == SpecialPort::NONE)
        new_in_port = SpecialPort::NONE;
    else if (in_port_ == SpecialPort::ANY)
        new_in_port = right.in_port_;
    else if (right.in_port_ == SpecialPort::ANY)
        new_in_port = in_port_;
    else
        new_in_port = (in_port_ == right.in_port_) ? in_port_
                                                   : SpecialPort::NONE;
    HeaderSpace new_domain = header_ & right.header_;

    bool is_empty = new_in_port == SpecialPort::NONE || new_domain.empty();
    return is_empty ? NetworkSpace::emptySpace()
                    : NetworkSpace(new_in_port, header_ & right.header_);
}

std::string NetworkSpace::toString() const
{
    return std::string() +
        "(" + getPortString(in_port_) + "|" + header_.toString() + ")";
}

std::ostream& operator<<(std::ostream& os, const NetworkSpace& domain)
{
    os << domain.toString();
    return os;
}

Transfer::Transfer(const HeaderChanger& header_changer):
    src_port_(SpecialPort::ANY), dst_port_(SpecialPort::NONE),
    header_changer_(header_changer)
{

}

Transfer::Transfer(PortId src_port, PortId dst_port,
                   HeaderChanger&& header_changer):
    src_port_(src_port), dst_port_(dst_port),
    header_changer_(std::move(header_changer))
{
    
}

Transfer Transfer::identityTransfer()
{
    return Transfer(
        SpecialPort::ANY, SpecialPort::NONE,
        HeaderChanger::identityHeaderChanger(HeaderSpace::GLOBAL_LENGTH)
    );
}

Transfer Transfer::portTransfer(PortId dst_port)
{
    return Transfer(
        SpecialPort::ANY, dst_port,
        HeaderChanger::identityHeaderChanger(HeaderSpace::GLOBAL_LENGTH)
    );
}

Transfer Transfer::operator*=(const Transfer& right)
{
    dst_port_ = right.dst_port_;
    header_changer_ *= right.header_changer_;
    return *this;
}

Transfer Transfer::operator*(const Transfer& right) const
{
    Transfer transfer(*this);
    transfer.dst_port_ = right.dst_port_;
    transfer.header_changer_ *= right.header_changer_;
    return std::move(transfer);
}

NetworkSpace Transfer::apply(NetworkSpace domain) const
{
    PortId port = (dst_port_ == SpecialPort::NONE)
                  ? domain.inPort()
                  : dst_port_;
    HeaderSpace header = header_changer_.identity()
                         ? domain.header()
                         : header_changer_.apply(domain.header());
    return NetworkSpace(port, header);
}

NetworkSpace Transfer::inverse(NetworkSpace domain) const
{
    PortId port = src_port_;
    HeaderSpace header = header_changer_.identity()
                         ? domain.header()
                         : header_changer_.inverse(domain.header());
    return NetworkSpace(port, header);
}

std::string Transfer::toString() const
{
    return std::string() +
        "(" + getPortString(src_port_)
            + "->" + getPortString(dst_port_)
            + "|" + header_changer_.toString() + ")";
}

std::ostream& operator<<(std::ostream& os, const Transfer& transfer)
{
    os << "(" << getPortString(transfer.src_port_)
       << "->" << getPortString(transfer.dst_port_)
       << "|" << transfer.header_changer_ << ")";
    return os;
}
