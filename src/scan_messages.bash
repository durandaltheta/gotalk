#!/bin/bash 
SOURCE_DIR=$1
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
MSG_PRE="message("
MSG_POST=")"
OUTPUT="$SCRIPT_DIR/message_definitions.tmp"
ENUM_HEADER="$SCRIPT_DIR/message_definitions.h"

echo "" > $OUTPUT
messages=$(grep -rnI "$MSG_PRE.*$MSG_POST;" "$SOURCE_DIR"/*)
while read -r line; do
	msg=$(echo $line | sed -e 's/$MSG_PRE\(.*\)$MSG_POST/\1/')
	echo "${msg}," >> $OUTPUT
done <<< "$messages"

difftxt=$(diff $OUTPUT $ENUM_HEADER)

if [ -z "$difftext" ]
then
	rm $ENUM_HEADER
	mv $OUTPUT $ENUM_HEADER
fi
