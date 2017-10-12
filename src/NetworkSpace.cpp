#include "NetworkSpace.hpp"

Transfer::Transfer(PortId src_port, PortId dst_port,
                   HeaderChanger header_changer):
    src_port_(src_port),
    dst_port_(dst_port),
    header_changer_(header_changer)
{
    
}

NetworkSpace Transfer::apply(NetworkSpace domain)
{
    PortId port = dst_port_ == SpecialPort::NONE
                  ? domain.port : dst_port_;
    HeaderSpace header = header_changer_.identity()
                         ? domain.header
                         : header_changer_.apply(domain.header);
    return NetworkSpace(port, header);
}

NetworkSpace Transfer::inverse(NetworkSpace domain)
{
    PortId port = src_port_;
    HeaderSpace header = header_changer_.identity()
                         ? domain.header
                         : header_changer_.inverse(domain.header);
    return NetworkSpace(port, header);
}
