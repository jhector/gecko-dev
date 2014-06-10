#!/bin/sh
GIPDL=./ipc/ipdl/dipdl.py

IPDL_SRS=$(find . -path ./ipc/ipdl/test -prune -o -path ./ipc/testshell -prune -o -name '*.ipdl' -print);
IPDL_HDR=$(find . -path ./ipc/ipdl/test -prune -o -path ./ipc/testshell -prune -o -name '*.ipdlh' -print);
IPDL_INC=( $(find . -path ./ipc/ipdl/test -prune -o -path -prune -o -name '*.ipdl*' -print | rev | cut -d '/' -f 2- | rev | uniq) );

INCLUDEDIRS=`printf -- " -I %s" ${IPDL_INC[*]}`;

if [ $# -gt 0 ]
then
    if [ $1 == 'all' ]
    then
        python $GIPDL $INCLUDEDIRS ${@:2} $IPDL_SRS;
    else
        python $GIPDL $INCLUDEDIRS $@;
    fi
else
    python $GIPDL --help

fi

