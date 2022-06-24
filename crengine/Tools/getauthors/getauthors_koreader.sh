#!/bin/sh

# Helper script to fetch authors from KOReader's crengine
# @pkb make merge commit from koreader/crengine during manual direct file coping, so we lost git history,
# To retrive this we must fetch this history from original KOReader's crengine fork repository.
# First commit date to this repo: "Fri Jan 31 19:58:49 2014 +0800"
# Last commit date - merge commit date.

# commit eb5b148082664f43885b48463810f32faaff38e1
# Author: Konstantin Potapov <pkbo@users.sourceforge.net>
# Date:   Sat Jan 4 12:19:17 2020 +0600
#
#     Merged koreader changes
#

srctree="../../.."

koreader_remote_exist=
git remote | grep koreader && koreader_remote_exist=yes
if [ "x${koreader_remote_exist}" != "xyes" ]
then
    git remote add koreader https://github.com/koreader/crengine.git
    git fetch koreader
fi

# all except thirdparty
git shortlog -s -e -n --group=author --no-merges koreader/master \
    --since="Fri Jan 31 19:58:49 2014 +0800" --until="Sat Jan 4 12:19:17 2020 +0600" \
    -- ':!thirdparty' \
    ${srctree}
