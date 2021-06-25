import os
from postfix import convert_to_z3

file = open("sample.txt", 'r')
file_lines = file.readlines()

assigned = []
unassigned = []
problem = []


def stmt_type(s):
    return {
        '+': "arith",
        '-': "arith",
        '*': "arith",
        '/': 'arith',
        '||': 'logic',
        '&&': 'logic',
        '==': 'equal'
    }.get(s, None)


def is_assignment(s):
    return s == '='


def get_lr_values(s):
    lrs = s.split(" = ")
    ls = [lrs[0]]
    rs = []
    for var in lrs[1].split():
        if var not in ['+', '-', '*', '/', '||', '&&', '==', 'true', 'false']:
            try:
                string_int = int(var)
            except ValueError:
                if var not in rs:
                    rs.append(var)
    return ls, rs


def get_unassigned_values(u):
    uv = []
    for s in u:
        for var in s.split():
            if var not in ['+', '-', '*', '/', '||', '&&', '==', 'true', 'false']:
                try:
                    string_int = int(var)
                except ValueError:
                    scs = ['!', '(', ')']
                    for sc in scs:
                        var = var.replace(sc, '')
                    # var = var.replace('.', '_')
                    if var not in uv:
                        uv.append(var)
    return uv


def get_rs_not_in_ls(ls, rs):
    nlr = []
    for rv in rs:
        if rv not in ls:
            nlr.append(rv)
    return nlr


statements = []
for line in reversed(file_lines):
    if line.strip() != "!()":
        assign = False
        statement_type = None
        for word in line.strip().split():
            assign = assign or is_assignment(word)
            if statement_type is None:
                statement_type = stmt_type(word)
        if assign:
            assigned.append(line.strip())
        else:
            unassigned.append(line.strip())
        statements.append({"Statement": line.strip(), "Type": statement_type, "IsAssign": assign})

# for statement in statements:
#     print(statement)

# print("-------UNASSIGN-------")
# for unassign in unassigned:
    # print(unassign)
# print("-------ASSIGN-------")
lvalues = []
rvalues = []
for assign in assigned:
    # print(assign)
    l, r = get_lr_values(assign)
    lvalues = lvalues + l
    rvalues = rvalues + r

# print("-------LVALUES-------")
# print(lvalues)
# print("-------RVALUES-------")
# print(rvalues)
not_l_rvalues = get_rs_not_in_ls(lvalues, rvalues) + get_rs_not_in_ls(lvalues, get_unassigned_values(unassigned))
# print("-------NOT_LVALUES-------")
# print(not_l_rvalues)


not_l_statements = []
for not_l_rvalue in not_l_rvalues:
    statement = next(item for item in statements if not_l_rvalue in item["Statement"])
    if statement["Type"] is not None:
        not_l_statements.append({"Statement": not_l_rvalue, "Type": statement["Type"]})
    else:
        not_l_statements.append({"Statement": not_l_rvalue, "Type": "logic"})

assignment_statement = []
for assign in assigned:
    statement = next(item for item in statements if assign in item["Statement"])
    assignment_statement.append(statement)
    statements.remove(statement)

print("-------NOT_LVALUES_STATEMENTS-------")
for s in not_l_statements:
    print(s)

print("-------ASSIGNMENTS-------")
for s in assignment_statement:
    print(s)

print("-------PROBLEM-------")
for s in statements:
    print(s)

print("-------Generated-------")
generated_file = open("generated.py", "w")
generated_file.write("from z3 import *" + 2 * os.linesep)
for s in not_l_statements:
    if s["Type"] == "arith":
        generated_file.write("{} = Int('{}')".format(s["Statement"], s["Statement"]).replace(".", "_") + os.linesep)
    if s["Type"] == "logic":
        generated_file.write("{} = Bool('{}')".format(s["Statement"], s["Statement"]).replace(".", "_") + os.linesep)
generated_file.write(os.linesep)
for s in assignment_statement:
    generated_file.write(s["Statement"].replace("true", "True").replace("false", "False") + os.linesep)

generated_file.write(os.linesep)

generated_file.write("s = Solver()" + os.linesep)
generated_file.write("s.add(" + os.linesep)
for s in statements:
    if "||" in s["Statement"] or "&&" in s["Statement"] or "!" in s["Statement"]:
        s["Statement"] = convert_to_z3(s["Statement"].replace(".", "_").replace("()", ""))
    generated_file.write("\t{},".format(s["Statement"]).replace(".", "_").replace("()", "") + os.linesep)
generated_file.write(")" + os.linesep)

generated_file.write(os.linesep)
generated_file.write("print(s.check())" + os.linesep)
generated_file.write("print(s.model())" + os.linesep)
