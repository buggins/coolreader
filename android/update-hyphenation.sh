#!/bin/sh

hyph_dir="../cr3gui/data/hyph"
res_dir="./res/raw"

list="Bulgarian.pattern
Catalan.pattern
Dutch.pattern
English_US.pattern
Finnish.pattern
French.pattern
German.pattern
Greek.pattern
Hungarian.pattern
Polish.pattern
Russian_EnUS.pattern
Spanish.pattern
Swedish.pattern
Turkish.pattern
Ukrainian.pattern"

if [ ! -f local.properties ]
then
    echo "Can't find file \"local.properties\""
    echo "You must call this script from the \"android\" subfolder of the project directory."
    exit 1
fi

for f in ${list}
do
	echo "updating ${f}..."
	fname="${hyph_dir}/${f}"
	if [ ! -f "${fname}" ]
	then
		echo "Error: ${fname} NOT found!"
		continue
	fi
	if echo "${f}" | grep -v -E '^.*\.pattern$'
	then
		echo "Error: Unsupported file extension ${f}!"
		continue
	fi
	lf=`echo "${f}" | sed -e 's/\(.*\)\.pattern/\L\1_hyphen.pattern/'`
	cp -pv "${fname}" "${res_dir}/${lf}"
done
