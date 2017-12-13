#pragma once

#include "./header_space/HeaderSpace.hpp"
#include <iostream>

typedef uint64_t SwitchId;
typedef uint32_t PortId;
typedef uint8_t TableId;
typedef uint32_t GroupId;
typedef uint64_t RuleId;

typedef uint16_t Priority;

const int HEADER_LENGTH = 1;

template <typename IdType>
class IdGenerator
{
public:
    IdGenerator(): last_id_((IdType)1) {}

    IdType getId() {return last_id_++;}
    void releaseId(IdType id) {/* TODO: implement release id */}

private:
    IdType last_id_;

};
