OPERATORS = {'!', '==', '||', '&&', '+', '-', '*', '/', '(', ')', '^'}  # set of operators

PRIORITY = {'+': 1, '-': 1, '*': 2, '/': 2, '^': 3, '||': 4, '&&': 5, '==': 6, '!': 7}  # dictionary having priorities

def normalize(expression):
    if '!' in expression:
        expression = expression.replace("!", "! ")
    if '(' in expression:
        expression = expression.replace("(", "( ")
    if ')' in expression:
        expression = expression.replace(")", " )")
    return expression

def infix_to_postfix(expression):  # input expression
    stack = []
    output = ''
    for ch in expression:
        if ch not in OPERATORS:
            output += ch + ' '
        elif ch == '(':
            stack.append('(')
        elif ch == ')':
            while stack and stack[-1] != '(':
                output += ' ' + stack.pop() + ' '
            stack.pop()
        else:
            while stack and stack[-1] != '(' and PRIORITY[ch] <= PRIORITY[stack[-1]]:
                output += ' ' + stack.pop() + ' '
            stack.append(ch)
    while stack:
        output += ' ' + stack.pop() + ' '
    return output

def postfix_to_z3(expression):
    stack = []
    output = ''
    for ch in expression:
        if ch not in OPERATORS:
            stack.append(ch)
        elif ch == '||':
            op1 = stack.pop()
            op2 = stack.pop()
            stack.append("Or({}, {})".format(op2, op1))
        elif ch == '&&':
            op1 = stack.pop()
            op2 = stack.pop()
            stack.append("And({}, {})".format(op2, op1))
        elif ch == '!':
            op1 = stack.pop()
            stack.append("Not({})".format(op1))
        else:
            op1 = stack.pop()
            op2 = stack.pop()
            stack.append("{} {} {}".format(op2, ch, op1))
    while stack:
        output += stack.pop() + ' '
    return output


def convert_to_z3(expression):
    postfix = infix_to_postfix(normalize(expression).split())
    z3 = postfix_to_z3(postfix.split())
    return z3
