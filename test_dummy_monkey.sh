#!/bin/bash
python3 -m http.server 8000 > py_server.log 2>&1 &
PY_PID=$!
sleep 1
./build/frontends/monkey/nsmonkey -v http://localhost:8000/css3test.html --enable_javascript=1 > run_local10.log 2>&1 &
MONKEY_PID=$!
sleep 10
kill $MONKEY_PID
kill $PY_PID
cat run_local10.log | grep -v "GENERIC POLL TIMED" | grep -v "fetch_dispatch_jobs" | grep -v "fetch_fdset" | grep -v "monkey_run" | grep -v "monkey_schedule_run" | head -n 100
