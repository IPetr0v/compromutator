#include "NetworkSpace.hpp"

NetworkSpace::NetworkSpace():
    in_port_(SpecialPort::ANY), header_(HEADER_LENGTH)
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

NetworkSpace& NetworkSpace::operator-=(const NetworkSpace& right)
{
    header_ -= right.header_;
    return *this;
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

Transfer::Transfer():
    src_port_(SpecialPort::ANY), dst_port_(SpecialPort::NONE),
    header_changer_(HEADER_LENGTH)
{

}

Transfer::Transfer(const HeaderChanger& header_changer):
    src_port_(SpecialPort::ANY), dst_port_(SpecialPort::NONE),
    header_changer_(header_changer)
{

}

Transfer::Transfer(PortId src_port, PortId dst_port,
                   const HeaderChanger& header_changer):
    src_port_(src_port), dst_port_(dst_port), header_changer_(header_changer)
{
    
}

NetworkSpace Transfer::apply(NetworkSpace domain) const
{
    PortId port = dst_port_ == SpecialPort::NONE
                  ? domain.inPort() : dst_port_;
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
