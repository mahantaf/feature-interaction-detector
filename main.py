from pprint import pprint

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


def parse_command(command):
    return command.split(" ")


class CommandList:
    def __init__(self, commands):
        self.commands = commands


class Command:
    def __init__(self, function_name, incident, command):
        self.function_name = function_name
        self.incident = incident
        self.command = command
        self.params = []
        self.paramTypes = []

    def __repr__(self):
        return str(self.__dict__)


groups = read_commands_from_file()
# print(groups)

commands = []
commandList = []
commandGroups = []

for group in groups:
    for command_list in group:
        parsed_command = parse_command(command_list)
        for i in range(0, len(parsed_command), 2):
            if i + 2 < len(parsed_command):
                function_name = parsed_command[i]
                command = parsed_command[i + 1]
                incident = parsed_command[i + 2]
                commands.append(Command(function_name, incident, command))
                i = i + 2
        commandList.append(commands)
        commands = []
    commandGroups.append(commandList)
    commandList = []


pprint(commandGroups)

# print(commandGroups)
