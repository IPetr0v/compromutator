#include "EdgeDiff.hpp"

EdgeDiff& EdgeDiff::operator+=(EdgeDiff&& other)
{

    for (const auto& edge : new_edges) {
        assert(not edge.domain.empty());
    }
    // Delete nonexistent edges
    delete_intersection(new_edges, other.removed_edges);
    delete_intersection(new_edges, other.removed_dependent_edges);
    delete_intersection(new_dependent_edges, other.removed_edges);
    delete_intersection(new_dependent_edges, other.removed_dependent_edges);
    delete_intersection(changed_edges, other.removed_edges);
    delete_intersection(changed_edges, other.removed_dependent_edges);

    // Find implicitly dependent edges
    auto implicitly_dependent = pop_dependent(other.new_edges);
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
    changed_edges.insert(
        changed_edges.end(),
        other.changed_edges.begin(),
        other.changed_edges.end());

    // Delete changed edges that are still new
    move_intersection(changed_edges, new_edges);
    move_intersection(changed_edges, new_dependent_edges);

    for (const auto& edge : new_edges) {
        assert(not edge.domain.empty());
    }

    return *this;
}

bool EdgeDiff::empty() const
{
    return new_edges.empty() &&
           new_dependent_edges.empty() &&
           changed_edges.empty() &&
           removed_edges.empty() &&
           removed_dependent_edges.empty();
}

void EdgeDiff::clear()
{
    new_edges.clear();
    new_dependent_edges.clear();
    changed_edges.clear();
    removed_edges.clear();
    removed_dependent_edges.clear();
}

std::ostream& operator<<(std::ostream& os, const EdgeDiff& diff)
{
    os<<"+"<<diff.new_edges.size()+diff.new_dependent_edges.size()
      <<"("<<diff.new_edges.size()<<")"
      <<" ~"<<diff.changed_edges.size()
      <<" -"<<diff.removed_edges.size()+diff.removed_dependent_edges.size()
      <<"("<<diff.removed_edges.size()<<")";
    return os;
}

//bool EdgeDiff::equals(const EdgePtr& first_edge,
//                      const EdgePtr& second_edge) const
//{
//    return *first_edge->src->rule == *second_edge->src->rule &&
//           *first_edge->dst->rule == *second_edge->dst->rule;
//}
//
//bool EdgeDiff::equals(const EdgePtr& first_edge,
//                      const DeletedEdge& second_edge) const
//{
//    return *first_edge->src->rule == *second_edge.first &&
//           *first_edge->dst->rule == *second_edge.second;
//}
//
//bool EdgeDiff::is_source(const EdgePtr& first_edge,
//                         const EdgePtr& second_edge) const
//{
//    return *first_edge->dst->rule == *second_edge->src->rule;
//}

void EdgeDiff::delete_intersection(std::list<Dependency>& src,
                                   std::list<Dependency>& dst) {
    auto it = std::remove_if(src.begin(), src.end(),
    [&dst](const Dependency& elem) {
        for (auto dst_it = dst.begin(); dst_it != dst.end(); dst_it++) {
            if (elem == *dst_it) {
                dst.erase(dst_it);
                return true;
            }
        }
        return false;
    });
    src.erase(it, src.end());
}

void EdgeDiff::move_intersection(std::list<Dependency>& src,
                                 std::list<Dependency>& dst)
{
    auto it = std::remove_if(src.begin(), src.end(),
    [&dst](const Dependency& src_edge) {
        for (const auto& dst_edge: dst) {
            return src_edge == dst_edge;
        }
        return false;
    });
    src.erase(it, src.end());
}

std::list<Dependency> EdgeDiff::pop_dependent(std::list<Dependency>& edges)
{
    auto it = std::remove_if(edges.begin(), edges.end(),
    [this](const Dependency& checked_edge) {
        // Edges connected to the new vertices are dependent
        // New vertices are defined as sources of new edges
        for (const auto& existing_edge: new_edges) {
            if (checked_edge.incident(existing_edge)) {
                return true;
            }
        }
        return false;
    });

    // Move edges
    std::list<Dependency> moved_edges;
    edges.splice(moved_edges.begin(), edges, it, edges.end());
    return moved_edges;
}
