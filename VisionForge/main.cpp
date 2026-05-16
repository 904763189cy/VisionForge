#include "graph.h"   

#include <iostream>
#include <unordered_map>
#include <string>

int main()
{
    Graph graph;

    // =========================
    // 1. 创建节点 A
    // =========================
    std::unordered_map<std::string, Variant> paramsA;
    paramsA["kernel"] = 3;
    paramsA["sigma"] = 1.5;
    paramsA["IPN:input"] = (int)DataType::image;
    paramsA["OPN:output"] = (int)DataType::image;

    NodeId nodeA = graph.add_node("Blur", paramsA);

    // =========================
    // 2. 创建节点 B
    // =========================
    std::unordered_map<std::string, Variant> paramsB;
    paramsB["threshold"] = 0.8;
    paramsB["enabled"] = true;
    paramsB["IPN:input"] = (int)DataType::image;
    paramsB["OPN:output"] = (int)DataType::image;

    NodeId nodeB = graph.add_node("Threshold", paramsB);

    // =========================
    // 2. 创建节点 C
    // =========================
    std::unordered_map<std::string, Variant> paramsC;
    paramsC["threshold"] = 0.8;
    paramsC["enabled"] = true;
    paramsC["IPN:input"] = (int)DataType::image;

    NodeId nodeC = graph.add_node("C", paramsC);

    // =========================
    // 3. 添加一条边 A -> B
    // =========================
    EdgeId edge = graph.add_edge(
        nodeA, "output",   // from node + port
        nodeB, "input"     // to node + port
    );

    std::cout << "Created edge id: " << edge << std::endl;

    // =========================
    // 4. 保存 Graph 到文件
    // =========================
    std::string path = "graph.json";

    if (graph.save_graph(path))
    {
        std::cout << "Graph saved successfully to " << path << std::endl;
    }
    else
    {
        std::cout << "Failed to save graph" << std::endl;
    }

    Graph graphA;
    graphA.load_graph(path);
    std::unordered_map<std::string, Variant> paramsC1;
    paramsC1["threshold"] = 0.5;
    graphA.alter_node(nodeC, paramsC1);
    graphA.save_graph("graphA.json");
    

    return 0;
}