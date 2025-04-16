#!/bin/bash

# Test script for multithreaded client-server system
# Simulates multiple clients sending various shell commands

NUM_CLIENTS=3

# Define commands each client will run
commands=(
  "echo Hello from client"
  "ls"
  "pwd"
  "ps aux | grep ssh"
  "ls -l | wc -l"
  "cat input | uniq | sort -r > output"
  "ls > out.txt; cat out.txt"
  "echo \"test\" > file.txt; cat file.txt"
  "wrongcommand"
  "ls |"
  "| grep hello"
  "touch testfile.txt"
  "echo \"abc\" > testfile.txt; cat testfile.txt"
  "rm testfile.txt"
  "ls testfile.txt"
  "top -bn1 | head -n 5"
  "df -h"
)

# Run multiple clients in parallel
for i in $(seq 1 $NUM_CLIENTS); do
  (
    echo "[CLIENT $i] Starting"

    for cmd in "${commands[@]}"; do
      echo "[CLIENT $i] Sending: $cmd"
      echo "$cmd"
      sleep 1
    done

    echo "exit"
    echo "[CLIENT $i] Sent exit command"
  ) | nc 127.0.0.1 8080 > "client_$i_output.txt" &

done

# Wait for all background jobs to finish
wait

echo "[TEST] All clients finished."

# Optional: display client output summaries
for i in $(seq 1 $NUM_CLIENTS); do
  echo
  echo "========== OUTPUT FROM CLIENT $i =========="
  cat "client_$i_output.txt"
done
