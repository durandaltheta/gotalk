import sys

if sys.argc < 2:
    exit(0)


# It's easier to modify the definitions of messages() and listens() if I define 
# them in one place
class globals():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    src_dir = sys.argv[1]

    msg_pre = "message("
    msg_post = ");"

    listen_pre = "listen("
    listen_post = ");"


def scan_source_for_messages():
    files = get_files_to_scan(globals.src_dir)

    messages = []
    callbacks = []
    enums = []

    for f in files:
        for message in scan_file_for_pattern(f, globals.msg_pre, globals.msg_post):
            pieces = parse_pattern_content(message)
            messages += pieces
            enums = pieces[0]
        for listen in scan_file_for_pattern(f, globals.listen_pre, globals.listen_post):
            callbacks += parse_pattern_content(listen)[-1]

    definitions = generate_definitions(messages, callbacks)

    write_enums(enums)
    write_definitions(definitions)


def scan_file_for_pattern(path, prescript, postscript):
    f=open(path, 'r')
    lines=f.readlines()

    contents = []

    for line in lines:
        content = extract_content_from_pattern(line, prescript, postscript)
        if content is not None:
            contents += content

    return contents


def extract_content_from_pattern(line, prescript, postscript)
        import re 
        pattern = re.compile("\s*"+prescript+".*"+postscript)
        result = pattern.search(line)

        if result:
            line = line.lstrip()
            line = line[len(prescript):]
            post = line.find(postscript)
            line = line[:post-1]
            return line 
        else:
            return None


# return a list of the pattern's comma separated content
def parse_pattern_content(pattern):
    return [x.strip() for x in message.split(',')]


def generate_definitions(messages, callbacks):
    structs = generate_structs_definitions(messages, callbacks)
    say = generate_say_definition(messages)
    lis = generate_listen_definition(messages, callbacks)
    call = generate_callback_handler_definitions(messages, callbacks)
    reg = generate_register_listener_definition(messages, callbacks)

    definitions = structs + lis + say + call +reg
    return definitions


def generate_structs_definitions(messages, callbacks):
    structs = ""
    for message in messages:
        name = message[0]
        types = []

        if len(message) > 1:
            types = message[1:]
        structs += generate_msg_struct_definition(name, types)

        """
        void listen(chan msg_center, 
                    unsigned int source, 
                    msg_types type, 
                    unsigned int destination, 
                    ...);
        """
        for callback in callbacks:
            structs += generate_registration_struct_definition(name, types, callback)

    return structs


def generate_msg_struct_definition(name, types):
    # struct definition
    definition = "typedef struct {\n"
    definition += "\tunsigned int source;\n"

    for t in types:
        definition += "\t"t+" "+t+"v;\n"

    definition += "} talk_msg_"+name+";\n\n"


def generate_registration_struct_definition(name, types, callback):
    definition = "typedef struct {\n"

    for t in types:
        definition += "\t"t+" "+t+"v;\n"

    definition += "\t"+callback" f;\n"
    call_object_type = extract_content_from_pattern(callback, '(', ')')

    definition += "} talk_registration_"+name+"_"+call_object_type+";\n\n"


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

    cnt = 0
    for message in messages:
        name = message[0]
        types = []

        if len(message) > 1:
            types = message[1:]

        lis += "\t//malloc listen definition "+cnt+"\n"
        lis += "\ttalk_registration reg* = malloc(sizeof(talk_registration));\n"
        lis += "\t(*reg) = {type, source, destination, callback};\n"
        cnt += 1

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
