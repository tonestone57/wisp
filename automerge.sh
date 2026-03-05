#!/bin/bash
set -e

branches=(
    "origin/jules-fetch-js-timeout-watchdogs-3398543383356405323"
    "origin/fix-quickjs-event-target-dom-10201501675984517242"
    "origin/jules/memory-arenas-14531613996922608918"
    "origin/fix-quickjs-event-handling-10351501665377774555"
    "origin/jules/async-image-decoding-5619972000016902936"
    "origin/fix/urldb-pre-parsed-2339961998656720591"
)

git config --global merge.conflictstyle diff3
