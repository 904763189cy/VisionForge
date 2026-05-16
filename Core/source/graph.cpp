// 自定义头文件
#include "graph.h"

// C++标准库头文件
#include <stdexcept>
#include <fstream>

// 第三方库头文件
#include <nlohmann/json.hpp>

using json = nlohmann::json;
static json variant_to_json(const Variant& v);
static Variant json_to_variant(const json& j);

NodeId Graph::add_node(const std::string& type, const std::unordered_map<std::string, Variant>& params)
{  
   Node node;  
   node.id = alloc_node_id();  
   node.type = type;  
   node.params = params;  

   Port port;  
   for (const auto& [key, value] : params) {
       if ((key.rfind("IPN:", 0) == 0 || key.rfind("OPN:", 0) == 0)) {
           port.name = key.substr(4);
           if (std::holds_alternative<int>(value)) {
               port.type = static_cast<DataType>(std::get<int>(value));
           } 
           else{
               throw std::invalid_argument("Invalid DataType in Variant");
           }
           if (key[0] == 'I') // 'I' for input, 'O' for output
               node.inputs.emplace_back(port);
           else
               node.outputs.emplace_back(port);
       }
   }
   node.executed = false;  
   m_nodes.emplace_back(node);
   return node.id;
}

void Graph::remove_node(NodeId id)
{
    std::erase_if(m_edges, [&](const Edge& edge)
        {
            return edge.fromNode == id || edge.toNode == id;
        });

    std::erase_if(m_nodes, [&](const Node& node)
        {
            return node.id == id;
        });
}

EdgeId Graph::add_edge(NodeId fromNodeId, const std::string& fromPort, NodeId toNodeId, const std::string& toPort)
{
    // 检测是否存在出发点及其端口，目的点及其端口
    if (fromNodeId == toNodeId)
        return -1;

    bool existFrom = false;
    bool existTo = false;
    for (const Node& node : m_nodes) {
        if (node.id == fromNodeId)
        {
            for (const Port& port : node.outputs)
            {
                if (port.name == fromPort) {
                    existFrom = true;
                    break;
                }
            }
            if (existFrom == false)
                break;
        }
        if (node.id == toNodeId)
        {
            for (const Port& port : node.inputs)
            {
                if (port.name == toPort) {
                    existTo = true;
                    break;
                }
            }
            if (existTo == false)
                break;
        }
        if (existFrom && existTo)
            break;
    }
    if(!existFrom || !existTo)
        return -1;

    // 修改 m_edges
    Edge edge;
    edge.id = alloc_edge_id();
    edge.fromNode = fromNodeId;
    edge.fromPort = fromPort;
    edge.toNode = toNodeId;
    edge.toPort = toPort;
    m_edges.emplace_back(edge);
    return edge.id;

}

void Graph::remove_edge(NodeId fromNode, const std::string& fromPort, NodeId toNode, const std::string& toPort)
{
    std::erase_if(m_edges, [&](const Edge& edge)
        {
            return edge.fromNode == fromNode && edge.fromPort == fromPort && edge.toNode == toNode && edge.toPort == toPort;
        });
}

bool Graph::alter_node(NodeId nodeId, const std::unordered_map<std::string, Variant>& params)
{
    bool ret = false;
    for (Node& node : m_nodes) {
        if (node.id != nodeId)
            continue;
        for (const auto& [key, value] : params) {
            if ((key.rfind("IPN:", 0) == 0 || key.rfind("OPN:", 0) == 0))
                continue;
            node.params[key] = value;
        }
        ret = true;
        break;
    }
    return ret;
}

bool Graph::alter_edge(EdgeId edgeId, NodeId nodeId, const std::string& port, bool start)
{
    bool ret = false;
    bool existNode = false;
    for (const Node& node : m_nodes) {
        if (node.id != nodeId)
            continue;
        if (start) {
            for (const Port& portItem : node.outputs)
            {
                if (portItem.name == port) {
                    existNode = true;
                    break;
                }
            }
            if (existNode == false)
                break;
        }
        else {
            for (const Port& portItem : node.inputs)
            {
                if (portItem.name == port) {
                    existNode = true;
                    break;
                }
            }
            if (existNode == false)
                break;
        } 
        break;
    }
    if (existNode == false)
        return false;

    for (Edge& edge : m_edges) {
        if (edge.id != edgeId)
            continue;
        if (start) {
            edge.fromNode = nodeId;
            edge.fromPort = port;
        }
        else {
            edge.toNode = nodeId;
            edge.toPort = port;
        }
        ret = true;
        break;
    }

    return ret;
}

bool Graph::validate() const
{
    return false;
}

bool Graph::load_graph(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) return false;

    json j;
    ifs >> j;

    m_nodes.clear();
    m_edges.clear();
    m_freeNodeIds.clear();
    m_freeEdgeIds.clear();

    // ===== nodes =====
    for (const auto& nj : j.at("nodes"))
    {
        Node n;
        n.id = nj.at("id").get<NodeId>();
        n.type = nj.at("type").get<std::string>();

        // inputs
        for (const auto& p : nj.at("inputs"))
        {
            Port port;
            port.name = p.at("name").get<std::string>();
            port.type = static_cast<DataType>(p.at("type").get<int>());
            n.inputs.push_back(port);
        }

        // outputs
        for (const auto& p : nj.at("outputs"))
        {
            Port port;
            port.name = p.at("name").get<std::string>();
            port.type = static_cast<DataType>(p.at("type").get<int>());
            n.outputs.push_back(port);
        }

        // params
        const auto& params = nj.at("params");
        for (auto it = params.begin(); it != params.end(); ++it)
        {
            n.params[it.key()] = json_to_variant(it.value());
        }

        n.executed = false;
        m_nodes.push_back(std::move(n));
    }

    // ===== edges =====
    for (const auto& ej : j.at("edges"))
    {
        Edge e;
        e.id = ej.at("id").get<EdgeId>();
        e.fromNode = ej.at("fromNode").get<NodeId>();
        e.fromPort = ej.at("fromPort").get<std::string>();
        e.toNode = ej.at("toNode").get<NodeId>();
        e.toPort = ej.at("toPort").get<std::string>();
        m_edges.push_back(std::move(e));
    }

    // ===== meta =====
    const auto& meta = j.at("meta");
    m_nextNodeId = meta.at("nextNodeId").get<NodeId>();
    m_nextEdgeId = meta.at("nextEdgeId").get<EdgeId>();
    m_freeNodeIds = meta.at("freeNodeIds").get<std::vector<NodeId>>();
    m_freeEdgeIds = meta.at("freeEdgeIds").get<std::vector<NodeId>>();


    return true;
}

bool Graph::save_graph(const std::string& path) const
{
    json j;
    printf("save_graph\n");
    // nodes
    j["nodes"] = json::array();
    for (const auto& n : m_nodes)
    {
        json nj;
        nj["id"] = n.id;
        nj["type"] = n.type;

        nj["inputs"] = json::array();
        for (const auto& p : n.inputs)
        {
            nj["inputs"].push_back({
                {"name", p.name},
                {"type", static_cast<int>(p.type)}
                });
        }

        nj["outputs"] = json::array();
        for (const auto& p : n.outputs)
        {
            nj["outputs"].push_back({
                {"name", p.name},
                {"type", static_cast<int>(p.type)}
                });
        }

        nj["params"] = json::object();
        for (const auto& [k, v] : n.params)
        {
            nj["params"][k] = variant_to_json(v);
        }

        j["nodes"].push_back(nj);
    }

    // edges
    j["edges"] = json::array();
    for (const auto& e : m_edges)
    {
        j["edges"].push_back({
            {"id", e.id},
            {"fromNode", e.fromNode},
            {"fromPort", e.fromPort},
            {"toNode", e.toNode},
            {"toPort", e.toPort}
            });
    }

    // meta
    j["meta"] = {
        {"nextNodeId", m_nextNodeId},
        {"nextEdgeId", m_nextEdgeId},
        {"freeNodeIds", m_freeNodeIds},
        {"freeEdgeIds", m_freeEdgeIds}
    };

    // write file
    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;

    ofs << j.dump(4); 
    return true;

}

NodeId Graph::alloc_node_id() 
{
    if (!m_freeNodeIds.empty()) {
        NodeId id = m_freeNodeIds.back();
        m_freeNodeIds.pop_back();
        return id;
    }
    return m_nextNodeId++;
}

EdgeId Graph::alloc_edge_id()
{
    if (!m_freeEdgeIds.empty()) {
        EdgeId id = m_freeEdgeIds.back();
        m_freeEdgeIds.pop_back();
        return id;
    }
    return m_nextEdgeId++;
}

static json variant_to_json(const Variant& v)
{
    return std::visit([](auto&& arg) -> json {
        return arg;
        }, v);
}

static Variant json_to_variant(const json& j)
{
    if (j.is_boolean()) return j.get<bool>();
    if (j.is_number_integer()) return j.get<int>();
    if (j.is_number_float()) return j.get<double>();
    if (j.is_string()) return j.get<std::string>();

    return {};
}