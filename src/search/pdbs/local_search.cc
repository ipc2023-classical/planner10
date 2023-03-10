#include "local_search.h"

#include "pattern_generator.h"
#include "utils.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/logging.h"
#include "../utils/timer.h"

#include <iostream>
#include <limits>
#include <memory>

using namespace std;

namespace pdbs {

LocalSearch::LocalSearch(const Options &opts) : 
    Heuristic(opts), 
    iterations(opts.get<int>("n")),
    op_order(opts.get<Order>("op_order")), 
    res_order(opts.get<Order>("res_order")),
    decrement_mode(opts.get<Decrement>("decrement")),
    seed(opts.get<int>("seed")),
    use_ff(opts.get<bool>("ff")),
    use_preferred(opts.get<bool>("preferred")) {

    std::srand(seed);
    
    shared_ptr<PatternCollectionGenerator> pattern_generator = 
        opts.get<shared_ptr<PatternCollectionGenerator>>("patterns");
    PatternCollectionInformation pci = pattern_generator->generate(task);

    pdbs = pci.get_pdbs();

    get_restrictions();

    if (use_ff) {
        hff = make_shared<ff_heuristic::FFHeuristic>(opts);
    }
    
    print_info();
}

void LocalSearch::get_restrictions() {
    TaskProxy task_proxy = TaskProxy(*task);

    // operators and costs
    for (OperatorProxy op : task_proxy.get_operators()) {
        operators.push_back(op.get_id());
        operator_cost.push_back(op.get_cost());
    }

    // restrictions from pdbs
    for (const shared_ptr<pdbs::PatternDatabase> &pdb : *pdbs) {
        restrictions.emplace_back();
        vector<int> &curr = restrictions.back();     
        for (OperatorProxy op : task_proxy.get_operators()) {
            if (pdb->is_operator_relevant(op)) {
                curr.push_back(op.get_id());
            }
        }
    }
    
    // relevant restrictions for each operator 
    restriction_operator.resize(operators.size());
    for (size_t i = 0; i < restrictions.size(); i++) {
        for (size_t j = 0; j < restrictions[i].size(); j++) {
            restriction_operator[restrictions[i][j]].push_back(i);
        }
    }
 
    // restriction ordering index
    restriction_order.resize(restrictions.size());
    for (size_t i = 0; i < restrictions.size(); i++)
        restriction_order[i] = i;
    
    // sort restrictions by size
    if (res_order == Order::SORT) {
        std::stable_sort(restriction_order.begin(), restriction_order.end(), 
            [this](const int &a, const int &b) { 
                return restrictions[a].size() < restrictions[b].size();
            }
        );
    }

    // for each restriction, sort operators by mentions in restrictions
    if (op_order == Order::SORT) {
        for (int id_res : restriction_order) {
            std::stable_sort(restrictions[id_res].begin(), restrictions[id_res].end(),
                [this](const int &a, const int &b) {    
                    return restriction_operator[a].size() > restriction_operator[b].size();
                }
            );
        }
    }

    // auxiliary vectors to heuristic computation
    value_pdbs.resize(pdbs->size());
    lower_bounds.resize(pdbs->size());
    operator_count.resize(operators.size());

    // for debug
    if (false /*utils::Verbosity*/) {
        for (int k : restriction_order) {
            for (int i :  restrictions[k])
                cout << restriction_operator[i].size() << " ";
            cout << endl;
        }
    }
}

int LocalSearch::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);

    if (set_value_pdbs(state)) {
        return DEAD_END;
    }
        
    int best = numeric_limits<int>::max();
    for (size_t i = 0; i < iterations; i++) {
        best = min(best, generate_solution(state));
    }
        
    return best;
}

int LocalSearch::generate_solution(const State &state) {
    for (size_t j = 0; j < operator_count.size(); j++)
        operator_count[j] = 0;
    
    for (size_t j = 0; j < value_pdbs.size(); j++)
        lower_bounds[j] = value_pdbs[j];
    
    if (res_order == Order::RANDOM) {
        std::random_shuffle (restriction_order.begin(), restriction_order.end());
    } 

    if (use_ff) {
        for (const int id_op : hff->get_preferred_operators(state)) {
            operator_count[id_op]++;
            for (size_t i = 0; i < restriction_operator[id_op].size(); i++) {
                lower_bounds[restriction_operator[id_op][i]] -= operator_cost[id_op];
            }
        }
    }

    for (const int id_res : restriction_order) {
        
        if (decrement_mode == Decrement::BEFORE) {
            for (size_t i = 0; i < restrictions[id_res].size() && lower_bounds[id_res] > 0; i++)
                lower_bounds[id_res] -= operator_cost[restrictions[id_res][i]] * 
                                        operator_count[restrictions[id_res][i]];
        }

        if (op_order == Order::RANDOM) {
            std::random_shuffle (restrictions[id_res].begin(), restrictions[id_res].end());
        }
        
        int var = 0;    
        while (lower_bounds[id_res] > 0) {
            const int id_op = restrictions[id_res][var];
            operator_count[id_op]++;

            if (decrement_mode == Decrement::ITERATIVE) {
                for (size_t i = 0; i < restriction_operator[id_op].size(); i++)
                    lower_bounds[restriction_operator[id_op][i]] -= operator_cost[id_op];
            }

            if (decrement_mode == Decrement::BEFORE) {
                lower_bounds[id_res] -= operator_cost[id_op];
            }

            var = (var + 1) % restrictions[id_res].size();
        }
        
    }
    
    int h_value = 0;

    for (size_t i = 0; i < operator_count.size(); i++) {
        h_value += operator_cost[i] * operator_count[i];
    }

    if (use_preferred) {
        for (const OperatorProxy &op : task_proxy.get_operators()) {
            if (operator_count[op.get_id()] > 0)
                set_preferred(op);
        }
    }

    return h_value;
}

int LocalSearch::set_value_pdbs(const State &state) {
    for (size_t i = 0; i < pdbs->size(); ++i) {
        int h = (*pdbs)[i]->get_value(state.get_unpacked_values());
        
        if (h == numeric_limits<int>::max())
            return DEAD_END;

        value_pdbs[i] = h;
    }

    return 0;
}

void LocalSearch::print_info() {
    utils::g_log << "Operators: " << operators.size() << endl;
    utils::g_log << "Restrictions: " << restrictions.size() << endl;

    if (restriction_operator.size() > 0) {
        int mean_mentions = 0;
        for (size_t i = 0; i < restriction_operator.size(); i++)
            mean_mentions += restriction_operator[i].size();
        utils::g_log << "Mean mentions: " << (mean_mentions / restriction_operator.size()) << endl;
    }

    if (restrictions.size() > 0) {
        int mean_operators = 0;
        for (size_t i = 0; i < restrictions.size(); i++)
            mean_operators += restrictions[i].size();
        utils::g_log << "Mean operators: " << (mean_operators / restrictions.size()) << endl;   
    }
}

static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    parser.document_synopsis("", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "no");
    parser.document_property("consistent", "no");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    parser.add_option<shared_ptr<PatternCollectionGenerator>>(
        "patterns",
        "pattern generation method",
        "systematic(4)");
    parser.add_option<int>(
        "n", 
        "number of iterations of randomized order", 
        "1",  
        Bounds("1", "1000"));

    vector<string> order_opts;
    order_opts.push_back("sort");
    order_opts.push_back("random");
    order_opts.push_back("default");
    
    parser.add_enum_option<Order>("res_order",
                           order_opts,
                           "Restriction order",
                           "default");
    
    parser.add_enum_option<Order>("op_order",
                           order_opts,
                           "Operator order",
                           "default");
    
    vector<string> dec_opts;
    dec_opts.push_back("before");
    dec_opts.push_back("iterative");
    
    parser.add_enum_option<Decrement>("decrement",
                           dec_opts,
                           "Decrement mode",
                           "iterative");

    parser.add_option<int>("seed",
                           "random seed",
                           "0",
                            Bounds("0", "2147483646"));
    
    parser.add_option<bool>(
        "ff",
        "",
        "false");

    parser.add_option<bool>(
        "preferred",
        "set preferred opeartors",
        "false");
    
    Heuristic::add_options_to_parser(parser);

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<LocalSearch>(opts);
}

static Plugin<Evaluator> _plugin("lsh", _parse, "heuristics_pdb");
}
