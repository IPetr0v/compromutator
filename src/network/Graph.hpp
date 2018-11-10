#pragma once

#include <algorithm>
#include <list>
#include <type_traits>
#include <utility>

struct EmptyVertex {};

template<typename VertexData, typename EdgeData = EmptyVertex>
class Graph {
public:
    struct Vertex;
    struct Edge;
    using VertexPtr = typename std::list<Vertex>::iterator;
    using EdgePtr   = typename std::list<Edge>::iterator;
    using AdjacencyPair = typename std::pair<VertexPtr, EdgePtr>;
    using AdjacencyList = typename std::list<AdjacencyPair>;

    struct Vertex: public VertexData {
        template<typename... Args>
        explicit Vertex(Args... args):
            VertexData(std::forward<Args>(args)...) {}

    private:
        AdjacencyList out_adjacency_list;
        AdjacencyList in_adjacency_list;

        friend class Graph;
    };

    struct Edge: EdgeData {
        template<typename... Args>
        Edge(VertexPtr src, VertexPtr dst, Args... args):
            EdgeData(std::forward<Args>(args)...), src(src), dst(dst) {}

        VertexPtr src;
        VertexPtr dst;
    };

    template<class Elem>
    struct Iterator {
        explicit Iterator(typename AdjacencyList::iterator iterator):
            adjacency_iterator_(iterator) {}

        bool operator!=(const Iterator<Elem>& other) {
            return adjacency_iterator_ != other.adjacency_iterator_;
        }

        const Iterator<Elem>& operator++() {
            adjacency_iterator_++;
            return *this;
        }

        Iterator<Elem> operator++(int) {
            auto iter_copy = *this;
            adjacency_iterator_++;
            return iter_copy;
        }

        Elem& operator*() const {
            return getElem<Elem>();
        }

    private:
        typename AdjacencyList::iterator adjacency_iterator_;

        template<
            typename T,
            typename std::enable_if_t<
                std::is_same<T, VertexPtr>::value
            >* = nullptr
        >
        Elem& getElem() const {
            return adjacency_iterator_->first;
        }

        template<
            typename T,
            typename std::enable_if_t<
                std::is_same<T, EdgePtr>::value
            >* = nullptr
        >
        Elem& getElem() const {
            return adjacency_iterator_->second;
        }
    };

    using VertexIterator = Iterator<VertexPtr>;
    using EdgeIterator   = Iterator<EdgePtr>;

    template<class Elem>
    struct Range {
        explicit Range(AdjacencyList& list):
            begin_(list.begin()), end_(list.end()) {}

        Iterator<Elem> begin() {return Iterator<Elem>(this->begin_);}
        Iterator<Elem> end()   {return Iterator<Elem>(this->end_);}
        bool empty() const {return not (begin_ != end_);}

    protected:
        typename AdjacencyList::iterator begin_;
        typename AdjacencyList::iterator end_;
    };

    using VertexRange = Range<VertexPtr>;
    using EdgeRange   = Range<EdgePtr>;

    template<typename... Args>
    VertexPtr addVertex(Args... args) {
        return vertex_list_.emplace(vertex_list_.end(),
            std::forward<Args>(args)...
        );
    }

    void deleteVertex(VertexPtr vertex) {
        deleteEdges(vertex);
        vertex_list_.erase(vertex);
    }

    template<typename... Args>
    EdgePtr addEdge(VertexPtr src, VertexPtr dst, Args... args) {
        auto edge = edge_list_.emplace(edge_list_.end(),
            src, dst, std::forward<Args>(args)...
        );
        src->out_adjacency_list.emplace_back(dst, edge);
        dst->in_adjacency_list.emplace_back(src, edge);
        return edge;
    }

    void deleteEdge(VertexPtr src, VertexPtr dst) {
        auto edge_pair = edge(src, dst);
        bool edge_exists = edge_pair.second;
        if (edge_exists) {
            auto edge = edge_pair.first;
            deleteEdge(edge);
        }
    }

    void deleteEdge(EdgePtr edge) {
        auto& out_list = edge->src->out_adjacency_list;
        auto& in_list = edge->dst->in_adjacency_list;

        delete_from_list(edge, out_list);
        delete_from_list(edge, in_list);
        edge_list_.erase(edge);
    }

    void deleteOutEdges(VertexPtr vertex) {
        for (auto adjacency_pair : vertex->out_adjacency_list) {
            auto dst_vertex = adjacency_pair.first;
            auto edge = adjacency_pair.second;

            // Delete the edge from the adjacency list
            auto& dst_in_list = dst_vertex->in_adjacency_list;
            delete_from_list(edge, dst_in_list);

            // Delete the edge
            edge_list_.erase(edge);
        }
        vertex->out_adjacency_list.clear();
    }

    void deleteInEdges(VertexPtr vertex) {
        for (auto adjacency_pair : vertex->in_adjacency_list) {
            auto src_vertex = adjacency_pair.first;
            auto edge = adjacency_pair.second;

            // Delete the edge from the adjacency list
            auto& src_out_list = src_vertex->out_adjacency_list;
            delete_from_list(edge, src_out_list);

            // Delete the edge
            edge_list_.erase(edge);
        }
        vertex->in_adjacency_list.clear();
    }

    void deleteEdges(VertexPtr vertex) {
        deleteOutEdges(vertex);
        deleteInEdges(vertex);
    }

    std::pair<EdgePtr, bool> edge(VertexPtr src, VertexPtr dst) {
        auto& src_out_list = src->out_adjacency_list;
        auto it = std::find_if(src_out_list.begin(), src_out_list.end(),
            [dst](AdjacencyPair pair) -> bool {
                return pair.first == dst;
            }
        );
        if (it != src_out_list.end()) {
            auto found_edge = it->second;
            return {found_edge, true};
        }
        return {EdgePtr(nullptr), false};
    }

    VertexPtr srcVertex(EdgePtr edge_desc) {
        return edge_desc->src_vertex;
    }

    VertexPtr dstVertex(EdgePtr edge_desc) {
        return edge_desc->dst_vertex;
    }

    // TODO: return descriptors
    const std::list<Vertex>& vertices() const {return vertex_list_;}

    VertexRange outVertices(VertexPtr vertex) {
        return VertexRange(vertex->out_adjacency_list);
    }

    VertexRange inVertices(VertexPtr vertex) {
        return VertexRange(vertex->in_adjacency_list);
    }

    const std::list<Edge>& edges() {return edge_list_;}

    EdgeRange outEdges(VertexPtr vertex) {
        return EdgeRange(vertex->out_adjacency_list);
    }

    EdgeRange inEdges(VertexPtr vertex) {
        return EdgeRange(vertex->in_adjacency_list);
    }

    size_t outDegree(VertexPtr vertex) const {
        return vertex->out_adjacency_list.size();
    }

    size_t inDegree(VertexPtr vertex) const {
        return vertex->in_adjacency_list.size();
    }

private:
    std::list<Vertex> vertex_list_;
    std::list<Edge> edge_list_;

    void delete_from_list(EdgePtr edge, AdjacencyList& list) {
        list.erase(std::remove_if(list.begin(), list.end(),
                                  [edge](AdjacencyPair pair) -> bool {
                                      return pair.second == edge;
                                  }),
                   list.end());
    }

};
