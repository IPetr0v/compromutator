#pragma once

#include <algorithm>
#include <list>

template<typename VertexDataType, typename EdgeDataType = int>
class Graph
{
public:
    struct Vertex;
    struct Edge;
    using VertexDescriptor = typename std::list<Vertex>::iterator;
    using EdgeDescriptor = typename std::list<Edge>::iterator;
    using AdjacencyPair = typename std::pair<VertexDescriptor, EdgeDescriptor>;
    using AdjacencyList = typename std::list<AdjacencyPair>;

    struct Vertex
    {
        explicit Vertex(VertexDataType&& data) : data(std::move(data)) {}

        AdjacencyList out_adjacency_list;
        AdjacencyList in_adjacency_list;
        VertexDataType data;
    };

    struct Edge
    {
        explicit Edge(VertexDescriptor src_vertex,
                      VertexDescriptor dst_vertex,
                      EdgeDataType&& data):
            src_vertex(src_vertex),
            dst_vertex(dst_vertex),
            data(std::move(data)) {}

        VertexDescriptor src_vertex;
        VertexDescriptor dst_vertex;
        EdgeDataType data;
    };

    struct AdjacencyIterator
    {
        explicit AdjacencyIterator(typename AdjacencyList::iterator iterator) :
            adjacency_iterator_(iterator) {}

        bool operator!=(const AdjacencyIterator& other)
        {
            return adjacency_iterator_ != other.adjacency_iterator_;
        }

        const AdjacencyIterator& operator++()
        {
            adjacency_iterator_++;
            return *this;
        }

    protected:
        typename AdjacencyList::iterator adjacency_iterator_;
    };

    struct VertexIterator : public AdjacencyIterator
    {
        explicit VertexIterator(typename AdjacencyList::iterator iterator) :
            AdjacencyIterator(iterator) {}

        VertexDescriptor& operator*() const
        {
            return this->adjacency_iterator_->first;
        }
    };

    struct EdgeIterator : public AdjacencyIterator
    {
        explicit EdgeIterator(typename AdjacencyList::iterator iterator) :
            AdjacencyIterator(iterator) {}

        EdgeDescriptor& operator*() const
        {
            return this->adjacency_iterator_->second;
        }
    };

    struct AdjacencyRange
    {
        explicit AdjacencyRange(AdjacencyList& list) :
            begin_(list.begin()), end_(list.end()) {}

    protected:
        typename AdjacencyList::iterator begin_;
        typename AdjacencyList::iterator end_;
    };

    struct VertexRange : public AdjacencyRange
    {
        explicit VertexRange(AdjacencyList& list) :
            AdjacencyRange(list) {}

        VertexIterator begin() {return VertexIterator(this->begin_);}
        VertexIterator end() { return VertexIterator(this->end_); }
    };

    struct EdgeRange : public AdjacencyRange
    {
        explicit EdgeRange(AdjacencyList& list) :
            AdjacencyRange(list) {}

        EdgeIterator begin() { return EdgeIterator(this->begin_); }
        EdgeIterator end() { return EdgeIterator(this->end_); }
    };

    VertexDescriptor addVertex(VertexDataType&& data)
    {
        return vertex_list_.emplace(vertex_list_.end(), std::move(data));
    }

    void deleteVertex(VertexDescriptor vertex)
    {
        deleteEdges(vertex);
        vertex_list_.erase(vertex);
    }

    EdgeDescriptor addEdge(VertexDescriptor src_vertex,
                           VertexDescriptor dst_vertex,
                           EdgeDataType&& data = EdgeDataType())
    {
        auto edge = edge_list_.emplace(edge_list_.end(),
            src_vertex, dst_vertex, std::move(data)
        );
        src_vertex->out_adjacency_list.emplace_back(dst_vertex, edge);
        dst_vertex->in_adjacency_list.emplace_back(src_vertex, edge);
        return edge;
    }

    void deleteEdge(VertexDescriptor src_vertex,
                    VertexDescriptor dst_vertex)
    {
        auto edge_pair = edge(src_vertex, dst_vertex);
        bool edge_exists = edge_pair.second;
        if (edge_exists) {
            auto edge = edge_pair.first;
            deleteEdge(edge);
        }
    }

    void deleteEdge(EdgeDescriptor edge)
    {
        auto src_vertex = edge->src_vertex;
        auto dst_vertex = edge->dst_vertex;
        auto& out_list = src_vertex->out_adjacency_list;
        auto& in_list = dst_vertex->in_adjacency_list;

        delete_from_list(edge, out_list);
        delete_from_list(edge, in_list);
        edge_list_.erase(edge);
    }

    void deleteOutEdges(VertexDescriptor vertex)
    {
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

    void deleteInEdges(VertexDescriptor vertex)
    {
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

    void deleteEdges(VertexDescriptor vertex)
    {
        deleteOutEdges(vertex);
        deleteInEdges(vertex);
    }

    VertexDataType& vertexData(VertexDescriptor vertex_desc)
    {
        return vertex_desc->data;
    }

    EdgeDataType& edgeData(EdgeDescriptor edge_desc)
    {
        return edge_desc->data;
    }

    std::pair<EdgeDescriptor, bool> edge(VertexDescriptor src_vertex,
                                         VertexDescriptor dst_vertex)
    {
        auto& src_out_list = src_vertex->out_adjacency_list;
        auto it = std::find_if(src_out_list.begin(), src_out_list.end(),
            [dst_vertex](AdjacencyPair pair) -> bool {
                return pair.first == dst_vertex;
            }
        );
        if (it != src_out_list.end()) {
            auto found_edge = it->second;
            return {found_edge, true};
        }
        return {EdgeDescriptor(nullptr), false};
    }

    VertexDescriptor srcVertex(EdgeDescriptor edge_desc)
    {
        return edge_desc->src_vertex;
    }

    VertexDescriptor dstVertex(EdgeDescriptor edge_desc)
    {
        return edge_desc->dst_vertex;
    }

    // TODO: return descriptors
    const std::list<Vertex>& vertices() {return vertex_list_;}

    VertexRange outVertices(VertexDescriptor vertex)
    {
        return VertexRange(vertex->out_adjacency_list);
    }

    VertexRange inVertices(VertexDescriptor vertex)
    {
        return VertexRange(vertex->in_adjacency_list);
    }

    const std::list<Edge>& edges() {return edge_list_;}

    EdgeRange outEdges(VertexDescriptor vertex)
    {
        return EdgeRange(vertex->out_adjacency_list);
    }

    EdgeRange inEdges(VertexDescriptor vertex)
    {
        return EdgeRange(vertex->in_adjacency_list);
    }

private:
    std::list<Vertex> vertex_list_;
    std::list<Edge> edge_list_;

    void delete_from_list(EdgeDescriptor edge, AdjacencyList& list)
    {
        list.erase(std::remove_if(list.begin(), list.end(),
                                  [edge](AdjacencyPair pair) -> bool {
                                      return pair.second == edge;
                                  }),
                   list.end());
    }

};
