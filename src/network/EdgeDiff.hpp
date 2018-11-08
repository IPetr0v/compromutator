#pragma once

#include "Rule.hpp"

struct EdgeDiff
{
    //using DeletedEdge = std::pair<RulePtr, RulePtr>;

    //std::list<EdgePtr> new_edges;
    //std::list<EdgePtr> new_dependent_edges;
    //std::list<EdgePtr> changed_edges;
    //std::list<DeletedEdge> removed_edges;
    //std::list<DeletedEdge> removed_dependent_edges;

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
    //bool equals(const EdgePtr& first_edge, const EdgePtr& second_edge) const;
    //bool equals(const EdgePtr& first_edge,
    //            const DeletedEdge& second_edge) const;
    //bool is_source(const EdgePtr& first_edge,
    //               const EdgePtr& second_edge) const;

    //template<class SRC, class DST>
    //void delete_intersection(std::list<SRC>& src, std::list<DST>& dst) {
    //    auto it = std::remove_if(src.begin(), src.end(),
    //    [this, &dst](SRC elem) {
    //        for (auto dst_it = dst.begin(); dst_it != dst.end(); dst_it++) {
    //            if (equals(elem, *dst_it)) {
    //                dst.erase(dst_it);
    //                return true;
    //            }
    //        }
    //        return false;
    //    });
    //    src.erase(it, src.end());
    //}

    void delete_intersection(std::list<Dependency>& src,
                             std::list<Dependency>& dst);
    void move_intersection(std::list<Dependency>& src,
                           std::list<Dependency>& dst);
    std::list<Dependency> pop_dependent(std::list<Dependency>& edges);
};
