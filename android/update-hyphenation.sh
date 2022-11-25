#!/bin/sh

hyph_dir="../cr3gui/data/hyph"
res_dir="./res/raw"

list=`ls ${hyph_dir}/*.pattern`

if [ ! -f local.properties ]
then
    echo "Can't find file \"local.properties\""
    echo "You must call this script from the \"android\" subfolder of the project directory."
    exit 1
fi

for f in ${list}
do
    aname=`basename $f`
    echo "updating ${aname}..."
    if [ ! -f "${f}" ]
    then
        echo "Error: ${fname} NOT found!"
        continue
    fi
    if echo "${f}" | grep -v -E '^.*\.pattern$'
    then
        echo "Error: Unsupported file extension ${f}!"
        continue
    fi
    dstname=`echo ${aname} | sed -e 's/[-,]/_/g'`
    cp -p "${f}" "${res_dir}/${dstname}"
done
