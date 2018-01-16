#include "NetworkSpace.hpp"

NetworkSpace::NetworkSpace(std::string str):
    in_port_(SpecialPort::ANY), header_(std::move(str))
{

}

NetworkSpace::NetworkSpace(const HeaderSpace& header):
    in_port_(SpecialPort::ANY), header_(header)
{
    
}

NetworkSpace::NetworkSpace(PortId in_port, const HeaderSpace& header):
    in_port_(in_port), header_(header)
{
    
}

NetworkSpace NetworkSpace::emptySpace()
{
    return NetworkSpace(SpecialPort::ANY,
                        HeaderSpace::emptySpace(HEADER_LENGTH));
}

NetworkSpace NetworkSpace::wholeSpace()
{
    // We represent empty network space as any getPort with an empty header
    return NetworkSpace(SpecialPort::ANY,
                        HeaderSpace::wholeSpace(HEADER_LENGTH));
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
    
    return NetworkSpace(new_in_port, header_ & right.header_);
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
    return Transfer(SpecialPort::ANY, SpecialPort::NONE,
                    HeaderChanger::identityHeaderChanger(HEADER_LENGTH));
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
