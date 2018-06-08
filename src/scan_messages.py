import sys

if sys.argc < 2:
    exit(0)

script_dir = os.path.dirname(os.path.abspath(__file__))
src_dir = sys.argv[1]

msg_prescript = "message("
msg_postscript = ")"


def scan_source_for_messages():
    global src_dir
    files=get_files_to_scan(src_dir)

    messages=[]
    enums=[]
    definitions=[]

    for f in files:
        for message in scan_file_for_messages(f):
            messages += message

    for message in messages:
        msg_pieces = parse_message_content(message)
        enums+=msg_pieces[0]

    write_enums(enums)
    write_definitions(definitions)


def get_files_to_scan(path):
    files = []

    for root, directories, filenames in os.walk(path):
        for filename in filenames: 
            files+=os.path.join(root,filename) 

    return files


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


def write_enums(enums):
    return


def write_definitions(definitions):
    return


def main():
    scan_messages()

if __name__ == "__main__":
    main()
