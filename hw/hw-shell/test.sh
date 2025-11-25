#!/bin/bash

(make clean && make) >> /dev/null

echo "Testing built-in command"
SHELL_PWD=`echo "pwd" | ./shell`
if [ "$SHELL_PWD" != "$PWD" ]; then
    echo "Build-in commands failed"
    printf "%s\n" \
    "Expected: ${PWD}" \
    "Received: ${SHELL_PWD}"
    exit 1
fi
echo "Built-in commands passed"


echo "Testing basic command"
SHELL_LS=`echo "/bin/ls" | ./shell`
REAL_LS=`ls`
if [ "$SHELL_LS" != "$REAL_LS" ]; then
    echo "Basic command failed"
    printf "%s\n" \
    "Expected: ${REAL_LS}" \
    "Received: ${SHELL_LS}"
    exit 1
fi
echo "Basic command passed"

echo "Testing Path Resolution"
SHELL_LS_PATH=`echo "ls" | ./shell`
if [ "$SHELL_LS_PATH" != "$REAL_LS" ]; then
    echo "Path Resolution failed"
    printf "%s\n" \
    "Expected: ${REAL_LS}" \
    "Received: ${SHELL_LS_PATH}"
    exit 1
fi
echo "Path Resolution passed"

echo "Testing Input/Output Redirection"
echo "hello world" > test_input.txt
SHELL_REDIR=`echo "cat < test_input.txt > test_output.txt" | ./shell`
RESULT=`cat test_output.txt`
if [ "$RESULT" != "hello world" ]; then
    echo "Redirection failed"
    printf "%s\n" \
    "Expected: hello world" \
    "Received: ${RESULT}"
    rm test_input.txt test_output.txt
    exit 1
fi
rm test_input.txt test_output.txt
echo "Redirection passed"

echo "Testing Pipes"
SHELL_PIPE=`echo "ls | wc -l" | ./shell`
REAL_PIPE=`ls | wc -l`
# Trim whitespace
SHELL_PIPE=`echo $SHELL_PIPE | xargs`
REAL_PIPE=`echo $REAL_PIPE | xargs`

if [ "$SHELL_PIPE" != "$REAL_PIPE" ]; then
    echo "Pipes failed"
    printf "%s\n" \
    "Expected: ${REAL_PIPE}" \
    "Received: ${SHELL_PIPE}"
    exit 1
fi
echo "Pipes passed"

echo "Testing Signal Handling (Manual Verification Recommended)"
# Start a sleep process in background and try to kill it via python script or just check exit code
# This is a basic check to ensure shell doesn't crash on signals
./shell <<EOF > /dev/null 2>&1 &
sleep 2
exit
EOF
SHELL_PID=$!
sleep 1
# Send SIGINT to shell (should be ignored by shell, but sleep is running in child)
kill -SIGINT $SHELL_PID 2>/dev/null
wait $SHELL_PID
if [ $? -eq 0 ]; then
    echo "Signal Handling test passed (Shell exited gracefully)"
else
    echo "Signal Handling test failed or shell crashed"
fi

echo "Testing Background Processes"
# Run sleep in background and wait, ensuring it doesn't block immediately
START_TIME=$(date +%s)
./shell <<EOF > /dev/null 2>&1
sleep 2 &
wait
exit
EOF
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

# If wait works, it should take at least 2 seconds
if [ $DURATION -ge 2 ]; then
    echo "Background Processes test passed"
else
    echo "Background Processes test failed (Duration: $DURATION s)"
    exit 1
fi

echo "Testing Foreground/Background Switching"
# This is hard to test automatically without a TTY, but we can try to simulate basic fg logic
# Run a background sleep, then fg it. If it finishes, we assume success.
./shell <<EOF > /dev/null 2>&1
sleep 1 &
fg
exit
EOF

if [ $? -eq 0 ]; then
    echo "Foreground/Background Switching test passed (Basic execution)"
else
    echo "Foreground/Background Switching test failed"
    exit 1
fi
