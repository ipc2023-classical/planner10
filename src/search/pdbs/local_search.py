#import numpy as np
from itertools import permutations

from numpy import cos

# key: operators, value: lower bound
constraints = [ 
    ('a', 1),
    ('b', 1),
    ('c', 1),
    ('ab', 6),
    ('ac', 6),
    ('bc', 6),
]
def compute_operator_count(constraints):

    # key: operator, value: counter
    operator_count = dict()

    for constraint in constraints:
        for op in constraint:
            operator_count[op] = 0

    return {k: v for k, v in sorted(operator_count.items(), key = lambda item: item)}

def compute_operator_constraint(operator_count, constraints): 

    # key: operator, value: mentioned constraint
    operator_constraint = dict()

    for op in operator_count:
        operator_constraint[op] = list()
        for constraint in constraints:
            if op in constraint:
                operator_constraint[op].append(constraint)

    return operator_constraint

def compute(operator_count, constraints, operator_constraint):
    for constraint in constraints:
        var = 0 
        while constraints[constraint] > 0:
            
            operator_count[constraint[var]] += 1

            for other in operator_constraint[constraint[var]]:
                if constraints[other] > 0:
                    constraints[other] -= 1
            
            # print(constraint[var], constraints, operator_count)
            


            var = (var + 1) % len(constraint)

    h_value = 0
    for op in operator_count:
        h_value += operator_count[op]
            
    return h_value


constraint_permutations = permutations(constraints, len(constraints))

results = dict()



for perm in constraint_permutations: 
    
    constraints = dict(zip([k[0] for k in perm],  [v[1] for v in perm]))
    
    operator_count = compute_operator_count(constraints)
    operator_constrait = compute_operator_constraint(operator_count, constraints)

    
    results[perm] = compute(operator_count, constraints, operator_constrait)


results = {k: v for k, v in sorted(results.items(), key = lambda item: item[1])}

