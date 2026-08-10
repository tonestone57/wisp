#!/bin/bash
patch frontends/gtk/download.c patch.diff
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DWISP_LOG_LEVEL=INFO -DWISP_BUILD_GTK_FRONTEND=ON
cmake --build build -j$(nproc)
