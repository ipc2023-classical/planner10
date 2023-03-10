#ifndef LOCAL_SEARCH_HEURISTIC_H
#define LOCAL_SEARCH_HEURISTIC_H

#include "pattern_database.h"

#include "../heuristic.h"

#include "../heuristics/ff_heuristic.h"

#include "types.h"

namespace options {
class OptionParser;
}

enum Order {
    SORT,
    RANDOM,
    DEFAULT
};

enum Decrement {
    BEFORE,
    ITERATIVE,
};

namespace pdbs {

class LocalSearch : public Heuristic {
    const size_t iterations;

    Order op_order;
    Order res_order;

    Decrement decrement_mode;

    int seed;

    bool use_ff;
    bool use_preferred;

    std::shared_ptr<PDBCollection> pdbs;

    // operadores
    std::vector<int> operators;
    std::vector<int> operator_cost;
    std::vector<int> operator_count;

    // restrições
    std::vector<std::vector<int>> restrictions;
    std::vector<std::vector<int>> restriction_operator;
    std::vector<std::vector<int>> restrictions_landmarks;

    // vetores auxiliares de memória
    std::vector<int> solution;
    std::vector<int> value_pdbs;
    std::vector<int> lower_bounds; 
    std::vector<int> restriction_order;

    std::shared_ptr<ff_heuristic::FFHeuristic> hff;
    std::shared_ptr<std::vector<bool>> hff_prefops; 

protected:
    virtual int compute_heuristic(const State &ancestor_state) override;
    
    int local_search();
    int generate_solution(const State &state);

    int set_value_pdbs(const State &state);
    int set_landmarks(const State &state);

    void get_restrictions();

    void print_info();

public:
    explicit LocalSearch(const options::Options &opts);
    virtual ~LocalSearch() = default;
};
}

#endif
