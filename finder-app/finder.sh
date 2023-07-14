#!/bin/sh

usage="Usage: $0 /path/to/dir text_to_search"

if [[ ( $@ == "--help") || ($@ == "-h") ]] then
    echo $usage
    echo "Flags:"
    printf "\t--help, -h\t\tHelp.\n"
    exit 0 
fi

if [[ $# -ne 2 ]] then
    echo "$0 needs exactly 2 arguments, you provided $#"
    echo $usage
    exit 1
else
    filesdir=$1
    searchstr=$2
fi

if [[ -d $filesdir ]] then
    files=`find $filesdir -type f 2> /dev/null | wc -l` 
    matching_lines=`grep -r $searchstr $filesdir 2> /dev/null | wc -l`
    echo "The number of files are $files and the number of matching lines are $matching_lines"
else 
    if [[ -e $filesdir ]] then
        echo "$filesdir exists, but is not a directory."
    else
        echo "$filesdir does not exist."
    fi
    echo $usage
    exit 1
fi

exit 0
