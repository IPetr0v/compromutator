#pragma once

#include "Rule.hpp"

struct EdgeDiff
{
    using DeletedEdge = std::pair<RulePtr, RulePtr>;

    std::list<EdgePtr> new_edges;
    std::list<EdgePtr> new_dependent_edges;
    std::list<EdgePtr> changed_edges;
    std::list<DeletedEdge> removed_edges;
    std::list<DeletedEdge> removed_dependent_edges;

    EdgeDiff& operator+=(EdgeDiff&& other) {
        // Delete nonexistent edges
        delete_intersection(new_edges, other.removed_edges);
        delete_intersection(new_edges, other.removed_dependent_edges);
        delete_intersection(new_dependent_edges, other.removed_edges);
        delete_intersection(new_dependent_edges, other.removed_dependent_edges);
        delete_intersection(changed_edges, other.removed_edges);
        delete_intersection(changed_edges, other.removed_dependent_edges);

        // Find implicitly dependent edges
        auto implicitly_dependent = move_dependent(other.new_edges);
        new_edges.insert(
            new_edges.end(),
            other.new_edges.begin(),
            other.new_edges.end());
        new_dependent_edges.insert(
            new_dependent_edges.end(),
            other.new_dependent_edges.begin(),
            other.new_dependent_edges.end());
        new_dependent_edges.insert(
            new_dependent_edges.end(),
            implicitly_dependent.begin(),
            implicitly_dependent.end());

        // Delete changed edges that are still new
        move_intersection(changed_edges, new_edges);
        move_intersection(changed_edges, new_dependent_edges);

        return *this;
    }

    bool empty() const {
        return new_edges.empty() &&
               new_dependent_edges.empty() &&
               changed_edges.empty() &&
               removed_edges.empty() &&
               removed_dependent_edges.empty();
    }
    void clear() {
        new_edges.clear();
        new_dependent_edges.clear();
        changed_edges.clear();
        removed_edges.clear();
        removed_dependent_edges.clear();
    }
    friend std::ostream& operator<<(std::ostream& os, const EdgeDiff& diff);

private:
    bool equals(const EdgePtr& first_edge, const EdgePtr& second_edge) const {
        return first_edge->src->rule == second_edge->src->rule &&
               first_edge->dst->rule == second_edge->dst->rule;
    }

    bool equals(const EdgePtr& first_edge,
                const DeletedEdge& second_edge) const {
        return first_edge->src->rule == second_edge.first &&
               first_edge->dst->rule == second_edge.second;
    }

    bool is_source(const EdgePtr& first_edge,
                   const EdgePtr& second_edge) const {
        return first_edge->dst->rule == second_edge->src->rule;
    }

    template<class SRC, class DST>
    void delete_intersection(std::list<SRC>& src, std::list<DST>& dst) {
        auto it = std::remove_if(src.begin(), src.end(),
        [this, &dst](SRC elem) {
            for (auto dst_it = dst.begin(); dst_it != dst.end(); dst_it++) {
                if (equals(elem, *dst_it)) {
                    dst.erase(dst_it);
                    return true;
                }
            }
            return false;
        });
        src.erase(it, src.end());
    }

    void move_intersection(std::list<EdgePtr>& src, std::list<EdgePtr>& dst) {
        auto it = std::remove_if(src.begin(), src.end(),
        [this, &dst](EdgePtr src_edge) {
            for (const auto& dst_edge: dst) {
                return equals(src_edge, dst_edge);
            }
            return false;
        });
        src.erase(it, src.end());
    }

    std::list<EdgePtr> move_dependent(std::list<EdgePtr>& edges) {
        auto it = std::remove_if(edges.begin(), edges.end(),
        [this](EdgePtr checked_edge) {
            // Edges connected to the new vertices are dependent
            // New vertices are defined as sources of new edges
            for (const auto& existing_edge: new_edges) {
                if (is_source(checked_edge, existing_edge)) {
                    return true;
                }
            }
            return false;
        });

        // Move edges
        std::list<EdgePtr> moved_edges;
        edges.splice(moved_edges.begin(), edges, it, edges.end());
        return moved_edges;
    }
};
