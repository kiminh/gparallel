// solopointer1202@gmail.com
#pragma once
#include "log.h"

namespace galois::gparallel {

std::string demangle(const char* name);

template <template<class> class M> class input; 
template <template<class> class M> class output;
template <template<class> class M> class produce;
template <class M>
struct parameter_traits {
};

template < template <class> class M>
struct parameter_traits< input<M> > {
    typedef type_id<typename M<none_type>::meta_info, none_type> node_meta_id;
    typedef typename M<none_type>::meta_info meta_info;
    static const parameter_type ptype = parameter_type::INPUT;
    static int id() {
        return node_meta_id::instance().id();
    }
    static const char * name() {
        return node_meta_id::instance().name();
    }
};

template <template <class> class M>
struct parameter_traits< output<M> > {
    typedef type_id<typename M<none_type>::meta_info, none_type> node_meta_id;
    typedef typename M<none_type>::meta_info meta_info;
    static const parameter_type ptype = parameter_type::OUTPUT;
    static int id() {
        return node_meta_id::instance().id();
    }
    static const char* name() {
        return node_meta_id::instance().name();
    }
};

template <template <class> class M>
struct parameter_traits< produce<M> > {
    typedef type_id<typename M<none_type>::meta_info, none_type> node_meta_id;
    typedef typename M<none_type>::meta_info meta_info;
    static const parameter_type ptype = parameter_type::PRODUCE;
    static int id() {
        return node_meta_id::instance().id();
    }
    static const char* name() {
        return node_meta_id::instance().name();
    }
};

template <class A, class NT>
void record_io(io_description & iodes) {
    if (parameter_traits<A>::ptype == parameter_type::NONE) {
        return;
    }
    bool is_item = true;
    io_item link;
    link.id = parameter_traits<A>::id();
    link.type = parameter_traits<A>::ptype;
    link.meta_level = is_item ? meta_level_t::ITEM : meta_level_t::QUERY;

    if (link.type == parameter_type::INPUT) {
        iodes.input[link.id] = link;
    } else {
        iodes.output[link.id] = link;
    }
};
template <class STACK, class... MS>
struct pop_and_process_M {};
template <class STACK, class... MS>
struct push_next_M {};


// STACK: An abstract stack type, which story some info for Depth-First-Search
// D: Data type
// M: Data meta type
// MS_dep: The data meta types which depends it
// template_list: a list of any template
// meta_info_list: a special list for store <D, M, MS_dep ...>
//
// deducer<STACK, meta_info_list>
template <class STACK, class D, template <class> class M, template <class> class... MS_dep>
struct push_next_M<STACK, meta_info_list<D, M, MS_dep ...>> 
    : public pop_and_process_M<STACK, template_list<MS_dep...>> 
{
    template <class P>
    static void deduce(io_description & deps) {
#ifdef _DEBUG
        DEBUG("%s::deduce[normal]", demangle(typeid(push_next_M<STACK, meta_info_list<D, M, MS_dep ...>>).name()).c_str());
#endif
        pop_and_process_M<STACK, template_list<MS_dep...>>::template deduce<P>(deps);
    }
};

// end case
template <class STACK>
struct pop_and_process_M<STACK, template_list<>> : public STACK {
    template <class P>
    static void deduce(io_description & deps) {
#ifdef _DEBUG
        DEBUG("%s::deduce[call stack::deduce]", 
            demangle(typeid(pop_and_process_M<STACK, template_list<>>).name()).c_str());
#endif
        STACK::template deduce<P>(deps);
    }
};

// deducer<STACK, template_list>
template <class STACK, template <class> class M, template <class> class... MS_dep>
struct pop_and_process_M<STACK, template_list<M, MS_dep ...>> 
    : public M<push_next_M<pop_and_process_M<STACK, template_list<MS_dep ...>>, typename M<none_type>::meta_info>> 
{
    template <class P>
    static void deduce(io_description & deps) {
#ifdef _DEBUG
        DEBUG("%s::deduce[record_io(%s)]", 
            demangle(typeid(pop_and_process_M<STACK, template_list<M, MS_dep ...>>).name()).c_str(),
            demangle(typeid(M<none_type>).name()).c_str()
        );
#endif
        record_io<input<M>, P>(deps);
        push_next_M<pop_and_process_M<STACK, template_list<MS_dep ...>>, typename M<none_type>::meta_info>::template deduce<P>(deps);
    }
};

// start
template <class T> struct depth_first_search_of_meta {};
template <class D, template <class> class M, template <class> class... MS_dep>
struct depth_first_search_of_meta< meta_info_list<D, M, MS_dep ...> > 
    : public M<pop_and_process_M<storage_reference<D>, template_list<MS_dep ...>>> 
{
    depth_first_search_of_meta(const D * data) {
        this->reset(const_cast<D*>(data)); 
    }
    template <class P>
    static void deduce(io_description & deps) {
#ifdef _DEBUG
        DEBUG("%s::deduce[start]", demangle(typeid(depth_first_search_of_meta< meta_info_list<D, M, MS_dep ...> >).name()).c_str());
#endif
        pop_and_process_M<storage_reference<D>, template_list<MS_dep ...>>::template deduce<P>(deps);
    };
};



template <template<class> class M>
class input {
public:
    typedef typename M<none_type>::meta_info meta_info;
    typedef typename M<none_type>::meta_storage data_meta_storage;
    typedef depth_first_search_of_meta<meta_info> input_imp;
    input(const data_meta_storage * data) : _m(const_cast<data_meta_storage*>(data)) {}
    template <template <class> class AnyM>
    operator input<AnyM>() {
        return input<AnyM>(_m.data());
    }
private:
    input_imp _m;
};

template <template<class> class M>
class output {
public:
    typedef typename M<none_type>::meta_info meta_info;
    typedef typename M<none_type>::meta_storage data_meta_storage;
    typedef M<storage_reference<data_meta_storage>> * meta_imp_type;

    struct output_storage_t : public M<storage_reference<data_meta_storage>> {
        output_storage_t (data_meta_storage * data) { this->reset(data); }
    };

    output(meta_imp_type v) : _v(v) {}
    meta_imp_type _v;
};

template <template<class> class M>
class produce {
public:
    typedef typename M<none_type>::meta_info meta_info;
    typedef typename M<none_type>::meta_storage data_meta_storage;
};

}