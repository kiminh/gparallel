#pragma once
#include <string>
#include <vector>
#include <memory>
#include "node_deduce.h"
#include "node_info.h"
#include "util.h"


namespace galois::gparallel
{


class node_container
{
public: 
    bool init();
    template <class NT>
    void register_node_operator();
private:
    std::vector<std::shared_ptr<node_info>> _nodes;
    std::vector<std::shared_ptr<node_info>> _end_nodes;
    std::map<std::string, std::shared_ptr<node_info>> _name_node_map;
};

template <class ...NTS>
struct register_node {
    static void reg(node_container & c)
    {
        register_node<type_list<NTS ...>>::reg(c);
    };
};

template <class NT, class ...NTS>
struct register_node<type_list<NT, NTS ...>> {
    static void reg(node_container & c)
    {
        c.register_node_operator<NT>();
        register_node<NTS ...>::reg(c);
    };
};

template <>
struct register_node<type_list<>> {
    static void reg(node_container & ) { /* stop recursion */}
};

template <class NT>
void node_container::register_node_operator()
{
    std::string name = demangle(typeid(NT).name());
    auto new_node = std::make_shared<node_info>();
    _nodes.push_back(new_node);
    io_description vec;
    deduce_depends<auto_type, NT>::deduce_io(vec);
}
}
