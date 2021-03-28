from pprint import pprint
import re
import os
import copy


class FileSystem:
    def __init__(self):
        self.folder = "temp/"
        self.command_file = "Result.txt"
        self.symbol_file = "symbols.txt"
        self.constraint_file = "constraints.txt"
        self.function_parameters_file = "params.txt"
        self.function_parameter_types_file = "param_types.txt"

    def clear_file(self, name):
        file = ""
        if name == "constraints":
            file = self.constraint_file
        elif name == "params":
            file = self.folder + self.function_parameters_file
        elif name == "paramTypes":
            file = self.folder + self.function_parameter_types_file
        elif name == "symbols":
            file = self.folder + self.symbol_file
        open(file, "w").close()

    def read_constraints(self):
        return self.read_multiple_parameters(self.constraint_file)

    def read_function_parameters(self):
        return self.read_parameters(self.folder + self.function_parameters_file)

    def read_function_parameter_types(self):
        return self.read_parameters(self.folder + self.function_parameter_types_file)

    def read_var_in_func_parameters(self):
        return self.read_multiple_parameters(self.folder + self.function_parameters_file)

    def read_var_in_func_parameter_types(self):
        return self.read_multiple_parameters(self.folder + self.function_parameter_types_file)

    def write_function_parameters(self, parameters):
        self.write_parameters(self.folder + self.function_parameters_file, parameters)

    def write_function_parameter_types(self, parameters):
        self.write_parameters(self.folder + self.function_parameter_types_file, parameters)

    def write_constraints(self, constraints):
        self.write_multiple_parameters(self.constraint_file, constraints)

    def read_parameters(self, file):
        parameter_file = open(file, 'r')
        lines = parameter_file.readlines()
        parameters = []
        for line in lines:
            if line.strip() != "---------------":
                parameters.append(line.strip())
        return parameters

    def read_multiple_parameters(self, file):
        parameter_file = open(file, 'r')
        lines = parameter_file.readlines()
        parameters_list = []
        parameters = []
        for line in lines:
            if line.strip() == "---------------":
                parameters_list.append(parameters)
                parameters = []
            else:
                parameters.append(line.strip())
        return parameters_list

    def write_parameters(self, file, parameters):
        parameter_file = open(file, "w")
        for parameter in parameters:
            parameter_file.write(parameter + os.linesep)
        parameter_file.write("---------------" + os.linesep)
        parameter_file.close()

    def write_multiple_parameters(self, file, multiple_parameters):
        parameter_file = open(file, "w")
        for parameters in multiple_parameters:
            for parameter in parameters:
                parameter_file.write(parameter + os.linesep)
            parameter_file.write("---------------" + os.linesep)
        parameter_file.close()

    def write_main_constraints(self, file, title, constraints):
        parameter_file = open(file, "w")
        parameter_file.write(title + os.linesep + os.linesep)
        for parameters in constraints:
            for parameter in parameters:
                parameter_file.write(parameter + os.linesep)
            parameter_file.write("---------------" + os.linesep)
        parameter_file.close()


def read_commands_from_file():
    command_group = []
    commands = []
    command_file = open('Result.txt', 'r')
    lines = command_file.readlines()
    for i, line in enumerate(lines):
        split_line = line.strip().split(" ")
        if split_line[0] == "from":
            if i != 0:
                command_group.append(commands)
                commands = []
            continue
        commands.append(line.strip()[2:])
    command_group.append(commands)
    return command_group


def make_command_lists(command_string):
    command_list = [[]]
    parsed_command = parser.parse_command(command_string)

    call = False
    var_in_func = False
    in_var_func = False

    for i in range(0, len(parsed_command), 2):
        in_var_func = False
        if i + 2 < len(parsed_command):
            file_name = parser.extract_file_path(parsed_command[i])
            function_name = parser.extract_element_name(parsed_command[i])
            command = parsed_command[i + 1]
            incident = parser.extract_element_name(parsed_command[i + 2])

            c = Command(file_name, function_name, incident, command)

            if call:
                call = False
                params = fs.read_function_parameters()
                param_types = fs.read_function_parameter_types()
                c.set_parameters(params)
                c.set_parameter_types(param_types)

            if var_in_func:
                in_var_func = True
                var_in_func = False
                var_in_func_params = fs.read_var_in_func_parameters()
                var_in_func_param_types = fs.read_var_in_func_parameter_types()

                new_command_list = []

                for j, param in enumerate(var_in_func_params):
                    copy_command_list = copy.deepcopy(command_list)

                    sub_command = Command(file_name, function_name, incident, command)
                    sub_command.set_parameters(param)
                    sub_command.set_parameter_types(var_in_func_param_types[j])

                    for cl in copy_command_list:
                        cl.append(sub_command)
                        new_command_list.append(cl)
                command_list = new_command_list

            if command == "CALL":
                call = True
                run = "./cfg " + file_name + " -- " + function_name + " " + incident + " FUNCTION 0"
                print("Executing: " + run)
                os.system(run)
            elif command == "VARINFFUNC":
                var_in_func = True
                run = "./cfg " + file_name + " -- " + function_name + " " + incident + " VARINFUNCEXTEND 0"
                print("Executing: " + run)
                os.system(run)

            if not in_var_func:
                for cl in command_list:
                    cl.append(c)
    return command_list


def run_command_lists(command_lists):
    constraint_list = []
    for list_count, command_list in enumerate(command_lists):
        fs.clear_file("symbols")
        fs.clear_file("constraints")

        is_root = 0
        for num, command in enumerate(reversed(command_list)):
            if num == len(command_list) - 1:
                is_root = 1
            command.run(is_root, list_count)
        new_constraint_list = fs.read_constraints()
        for constraint in new_constraint_list:
            constraint_list.append(constraint)
    return constraint_list


def make_command_group(groups):
    command_lists = []
    command_groups = []
    for group in groups:
        for command_string in group:
            command_lists.append(make_command_lists(command_string))
        command_groups.append(command_lists)
        command_lists = []
    return command_groups


class Parser:
    def __init__(self):
        self.pattern = re.compile(r'(?<!^)(?=[A-Z])')

    def parse_command(self, command):
        return command.split(" ")

    def extract_element_name(self, name):
        split_name = name.split("::")
        return split_name[-1]

    def extract_file_path(self, name):
        split_name = name.split("::")
        class_name = self.pattern.sub('_', split_name[1]).lower()
        return "autonomoose_core/" + split_name[0] + "/src/" + class_name + ".cpp"


class Command:
    def __init__(self, file_name, function_name, incident, command):
        self.file_name = file_name
        self.function_name = function_name
        self.incident = incident
        self.command = command
        self.params = []
        self.param_types = []

    def __repr__(self):
        return str(self.__dict__)

    def run(self, is_root, command_list_number):
        fs.clear_file("params")
        fs.clear_file("paramTypes")

        if len(self.params):
            fs.write_function_parameters(self.params)
            fs.write_function_parameter_types(self.param_types)

        run = "./cfg " + self.file_name + " -- " + self.function_name + " " + self.incident + " " + self.command + " " \
              + str(is_root) + " " + str(command_list_number)
        print("Executing: " + run)
        os.system(run)

    def set_parameters(self, parameters):
        self.params = parameters

    def set_parameter_types(self, parameter_types):
        self.param_types = parameter_types


fs = FileSystem()
parser = Parser()

groups = read_commands_from_file()

for i, group in enumerate(groups):
    for j, command_string in enumerate(group):
        command_lists = make_command_lists(command_string)
        constraints = run_command_lists(command_lists)
        os.makedirs("output/" + str(i) + "/" + str(j))
        fs.write_main_constraints("output/" + str(i) + "/" + str(j) + "/constraints.txt", command_string, constraints)

# command_groups = make_command_group(read_commands_from_file())
# for i, group in enumerate(command_groups):
#     print("COMMAND GROUP #" + str(i))
#     for j, command_list in enumerate(group):
#         print("COMMAND LIST #" + str(j))
#         pprint(command_list)
