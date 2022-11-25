#!/bin/sh

srctree="../../.."

# all except thirdparty
git shortlog -s -e -n --group=author --no-merges \
    -- ':!thirdparty' ':!thirdparty_unman' ':!cr3android' \
    ${srctree}
