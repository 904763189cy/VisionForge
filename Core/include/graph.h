#ifndef __GRAPH__
#define __GRAPH__

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <variant>
#include <string>

#ifdef CORE_EXPORTS
#define COREDLL_API __declspec(dllexport)
#else
#define COREDLL_API __declspec(dllimport)
#endif

using NodeId = uint32_t;
using EdgeId = uint32_t;
using Variant = std::variant<int, double, std::string, bool>;
enum class COREDLL_API DataType { Tensor, pointcloud, image};


struct COREDLL_API Port {
    std::string name;  // 端口名字
    DataType type;     // 端口类型
};

struct COREDLL_API Node {
    NodeId id = 0;          // 节点ID   0 1 2 ...  图里唯一
    std::string type;       // 节点类型
    std::vector<Port> inputs;    // 节点输入端口
    std::vector<Port> outputs;   // 节点输出端口
    std::unordered_map<std::string, Variant> params;  //节点属性

    // runtime
    bool executed = false; 
};


struct COREDLL_API Edge {
    EdgeId id;        // 连接线id  0 1 2 ...  图里唯一

    NodeId fromNode;
    std::string fromPort;

    NodeId toNode;
    std::string toPort;
};


class COREDLL_API Graph {
public:

    // 零法则
    Graph() = default;
    ~Graph() = default;
    Graph(const Graph & other) = default;
    Graph& operator=(const Graph & other) = default;
    Graph(Graph&&) noexcept = default;
    Graph& operator=(Graph&&) noexcept = default;

   
    /// <summary>
    /// 为图加入节点
    /// </summary>
    /// <param name="type">节点类型名称</param>
    /// <param name="params">节点属性</param>
    NodeId add_node(const std::string& type, const std::unordered_map<std::string, Variant>& params);

    /// <summary>
    /// 移除节点，如果节点有连接线，还要删除连接线
    /// </summary>
    /// <param name="id"></param>
    void remove_node(NodeId id);

    /// <summary>
    /// 添加边
    /// </summary>
    /// <param name="from_node">出发点</param>
    /// <param name="from_port">出发点端口</param>
    /// <param name="to_node">目的点</param>
    /// <param name="to_port">目的点端口</param>
    EdgeId add_edge(NodeId fromNode, const std::string& fromPort, NodeId toNode, const std::string& toPort);
    
    /// <summary>
    /// 删除边
    /// </summary>
    /// <param name="from_node">出发点</param>
    /// <param name="from_port">出发点端口</param>
    /// <param name="to_node">目的点</param>
    /// <param name="to_port">目的点端口</param>
    void remove_edge(NodeId fromNode, const std::string& fromPort, NodeId toNode, const std::string& toPort);

    /// <summary>
    /// 修改节点属性
    /// </summary>
    /// <param name="nodeId">节点Id</param>
    /// <param name="params">属性参数</param>
    /// <returns></returns>
    bool alter_node(NodeId nodeId, const std::unordered_map<std::string, Variant>& params);

    /// <summary>
    /// 修改边的连接
    /// </summary>
    /// <param name="edgeId">边id</param>
    /// <param name="nodeId">新节点id</param>
    /// <param name="port">新节点端口名</param>
    /// <param name="start">连接起始节点</param>
    /// <returns></returns>
    bool alter_edge(EdgeId edgeId, NodeId nodeId, const std::string& port, bool start);

    /// <summary>
    /// 检测图是否能跑通
    /// </summary>
    /// <returns></returns>
    bool validate() const;
    
    /// <summary>
    /// 从指定文件读取图
    /// </summary>
    /// <param name="path">文件路径</param>
    /// <returns></returns>
    bool load_graph(const std::string& path);

    /// <summary>
    /// 将图保存到指定文件
    /// </summary>
    /// <param name="path">文件路径</param>
    /// <returns></returns>
    bool save_graph(const std::string& path) const;

    const std::vector<Node>& get_nodes() const { return m_nodes; };
    const std::vector<Edge>& get_edges() const { return m_edges; };

private:
    /// <summary>
    /// 分配节点ID
    /// </summary>
    /// <returns></returns>
    NodeId alloc_node_id();

    /// <summary>
    /// 分配边ID
    /// </summary>
    /// <returns></returns>
    EdgeId alloc_edge_id();

private:
    std::vector<Node> m_nodes;
    std::vector<Edge> m_edges;

    NodeId m_nextNodeId = 1;
    std::vector<NodeId> m_freeNodeIds;
    NodeId m_nextEdgeId = 1;
    std::vector<NodeId> m_freeEdgeIds;
};

#endif // !__GRAPH__
