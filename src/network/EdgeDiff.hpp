#pragma once

#include "Rule.hpp"

struct EdgeDiff
{
    std::list<Dependency> new_edges;
    std::list<Dependency> new_dependent_edges;
    std::list<Dependency> changed_edges;
    std::list<Dependency> removed_edges;
    std::list<Dependency> removed_dependent_edges;

    EdgeDiff& operator+=(EdgeDiff&& other);

    bool empty() const;
    void clear();
    friend std::ostream& operator<<(std::ostream& os, const EdgeDiff& diff);

private:
    void delete_intersection(std::list<Dependency>& src,
                             std::list<Dependency>& dst);
    void move_intersection(std::list<Dependency>& src,
                           std::list<Dependency>& dst);
    std::list<Dependency> pop_dependent(std::list<Dependency>& edges);
};
