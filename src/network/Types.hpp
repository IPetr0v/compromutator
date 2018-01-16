#pragma once

#include "../openflow/Types.hpp"

#include <map>

class Switch;
class Port;
class Table;
class Group {}; // TODO: implement groups
class Rule;
using SwitchPtr = Switch*;
using PortPtr = Port*;
using TablePtr = Table*;
using GroupPtr = Group*;
using RulePtr = Rule*;

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

    MapIterator<Map> begin() const {return MapIterator<Map>(begin_);}
    MapIterator<Map> end() const {return MapIterator<Map>(end_);}

private:
    Map& map_;
    Iterator begin_;
    Iterator end_;

};

using RuleMap = std::map<RuleId, RulePtr, std::greater<RuleId>>;
using RuleRange = MapRange<RuleMap>;
