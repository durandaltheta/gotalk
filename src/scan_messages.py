import sys

if sys.argc < 2:
    exit(0)

script_dir = os.path.dirname(os.path.abspath(__file__))
src_dir = sys.argv[1]

msg_prescript = "message("
msg_postscript = ");"


def scan_source_for_messages():
    global src_dir
    files = get_files_to_scan(src_dir)

    messages = []
    enums = []

    for f in files:
        for message in scan_file_for_messages(f):
            pieces = parse_message_content(message)
            messages += pieces
            enums = pieces[0]

    definitions = generate_definitions()

    write_enums(enums)
    write_definitions(definitions)


def scan_file_for_messages(path):
    global msg_prescript
    global msg_postscript

    import re 

    pattern = re.compile("\s*"+msg_prescript+".*"+msg_postscript)

    f=open(path, 'r')
    lines=f.readlines()
    messages=[]

    for line in lines:
        result = pattern.search(line)
        if result:
            line = line.lstrip()
            line = line[len(msg_prescript):]
            post = line.find(msg_postscript)
            line = line[:post-1]
            messages += line 

    return messages


def parse_message_content(message):
    return [x.strip() for x in message.split(',')]


def generate_definitions(messages):
    structs = generate_structs_definitions(messages)
    say = generate_say_definition(messages)
    lis = generate_listen_definition(messages)
    call = generate_callback_handler_definitions(messages)
    reg = generate_register_listener_definition(messages)

    definitions = structs + lis + say + call +reg
    return definitions


def generate_structs_definitions(messages):
    structs = ""
    for message in messages:
        name = message[0]
        types = []

        if len(message) > 1:
            types = message[1:]
        structs += generate_msg_struct_definition(name, types)
        structs += generate_registration_struct_definition(name, types)

    return structs


def generate_msg_struct_definition(name, types):
    # struct definition
    definition = "typedef struct {\n"
    definition += "\tunsigned int source;\n"

    for t in types:
        definition += "\t"t+" "+t+"v;\n"

    definition += "} talk_msg_"+name+";\n\n"


def generate_registration_struct_definition(name, types):
    definition = "typedef struct {\n"

    for t in types:
        definition += t+" "+t+"v;\n"

    definition += "} talk_registration_"+name+";\n\n"


def generate_say_definition(messages):
    say = "void say(chan msg_center, msg_types type, unsigned int source, ...) {\n"
    say += "\tint arg_count=1;\n"
    say += "\tva_list ap;\n\n"
    say += "\tswitch(type) {\n"

    for message in messages:
        name = message[0]
        types = []

        if len(message) > 1:
            types = message[1:]
        say += generate_say_case_definition(name, types)

    say += "\t\tdefault:\n"
    say += "\t\t\tva_start(ap, arg_count);\n"
    say += "\t\t\t_say(msg_center, type, source, va_arg(ap, void*));\n"
    say += "\t\t\tbreak;\n"
    say += "\t}\n\n"
    say += "\treturn;\n"
    say += "}\n\n\n"
    return say


def generate_say_case_definition(name, types):
    definition = "\t\tcase "+name+":\n"
    definition += "\t\t\targ_count = "+len(types)+";\n"
    definition += "\t\t\tva_start(ap, arg_count);\n"
    definition += "\t\t\ttalk_msg_"+name+" st;\n"

    for t in types:
        definition += "\t\t\tst."+t+"v = va_arg(ap, "+t+");\n"

    definition += "\t\t\t_say(msg_center, type, source, &st);\n"
    definition += "\t\t\tbreak;\n"
    return definition


def generate_listen_definition(messages):
    lis = "void listen(chan msg_center,\n"
    lis += "\tunsigned int source,\n"
    lis += "\tmsg_types type,\n"
    lis += "\tunsigned int destination,\n"
    lis += "\t...){\n"

    for message in messages:
        name = message[0]
        types = []

        if len(message) > 1:
            types = message[1:]

        lis += "\t//malloc 3\n"
        lis += "\ttalk_registration reg* = malloc(sizeof(talk_registration));\n"
        lis += "\t(*reg) = {type, source, destination, callback};\n"

    lis += "\ttalk_msg reg_msg = {REGISTER_listener, source, reg};\n"
    lis += "\tsay(msg_center, reg_msg);\n"
    lis += "\treturn;\n"
    lis += "}\n"
    return lis


def generate_register_listener_definition(messages):
    reg = "void register_listener(List* listener_list, talk_msg msg) {\n" 
    reg += "\tmsg_types type = msg.type;\n"
    reg += "\tchan ch = chmake(talk_msg, TALK_MESSAGE_MANAGER_BUFFER_SIZE);\n"
    reg += "\tList* node = create_list();\n"
    reg += "\t(*node).data = &ch;\n"
    reg += "\tinsert(listener_list, node, 0);\n"
    reg += "\ttalk_msg start_msg = {START_TALK, NULL, NULL};\n"
    reg += "\tchs(ch, talk_msg, start_msg);\n\n"
    reg += "\tswitch(type) {\n"

    for message in messages:
        name = message[0]
        types = []

        if len(message) > 1:
            types = message[1:]

        reg += generate_register_listener_case_definition(name)

    reg += "\t\tdefault:\n"
    reg += "\t\t\tgo(callback_handler(ch, reg));\n"
    reg += "\t\t\tbreak;\n"
    reg += "\t}\n\n"
    reg += "\treturn;\n"
    reg += "}\n\n\n"
    return reg


def generate_register_listener_case_definition(name):
    reg = "\t\tcase "+name+":\n"
    reg += "\t\t\tgo(callback_handler_"+name+"(ch, reg));\n"
    reg += "\t\t\tbreak;\n"
    return reg


def get_files_to_scan(path):
    files = []

    for root, directories, filenames in os.walk(path):
        for filename in filenames: 
            files+=os.path.join(root,filename) 

    return files


def write_enums(enums):
    return


def write_definitions(definitions):
    return


def main():
    scan_messages()


if __name__ == "__main__":
    main()
