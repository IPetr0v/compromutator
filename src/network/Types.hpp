#pragma once

#include "../Types.hpp"

#include <map>
#include <memory>

class Switch;
class Port;
class Table;
class Group {}; // TODO: implement groups
class Rule;
using SwitchPtr = Switch*;
using PortPtr = Port*;
using TablePtr = Table*;
using GroupPtr = Group*;
//using RulePtr = Rule*;
using RulePtr = std::shared_ptr<Rule>;

struct PortInfo
{
    PortInfo(PortId id, uint32_t speed): id(id), speed(speed) {}

    PortId id;
    uint32_t speed;
};

struct SwitchInfo
{
    SwitchInfo() {}
    SwitchInfo(SwitchId id, uint8_t table_number,
               std::vector<PortInfo> ports):
        id(id), table_number(table_number), ports(ports) {}
    SwitchInfo(SwitchId id, uint8_t table_number,
               std::vector<PortInfo>&& ports):
        id(id), table_number(table_number), ports(std::move(ports)) {}

    SwitchId id;
    uint8_t table_number;
    std::vector<PortInfo> ports;
};

template<typename Map>
class MapIterator
{
    using Iterator = typename Map::iterator;
    using Data = typename Map::mapped_type;

public:
    explicit MapIterator(Iterator iterator):
        iterator(iterator) {}

    bool operator!=(const MapIterator& other) const {
        return iterator != other.iterator;
    }
    const MapIterator& operator++() {
        iterator++;
        return *this;
    }
    Data& operator*() const {
        return iterator->second;
    }

private:
    Iterator iterator;

};

template<typename Map>
class MapRange
{
    using Iterator = typename Map::iterator;

public:
    explicit MapRange(Map& map):
        map_(map), begin_(map.begin()), end_(map.end()) {}
    MapRange(Map& map, Iterator begin, Iterator end):
        map_(map), begin_(begin), end_(end) {}

    bool empty() const {return not (begin_ != end_);}
    MapIterator<Map> begin() const {return MapIterator<Map>(begin_);}
    MapIterator<Map> end() const {return MapIterator<Map>(end_);}

private:
    Map& map_;
    Iterator begin_;
    Iterator end_;

};

using RuleMap = std::map<RuleId, RulePtr, std::greater<RuleId>>;
using RuleRange = MapRange<RuleMap>;
