#!/bin/bash

# Start a mix of TCP and UDP clients with different resolutions
for i in {1..10}; do
  # Alternate between TCP and UDP
  if [ $(($i % 2)) -eq 0 ]; then
    PROTOCOL="TCP"
  else
    PROTOCOL="UDP"
  fi
  
  # Alternate between resolutions
  case $(($i % 3)) in
    0) RESOLUTION="480p" ;;
    1) RESOLUTION="720p" ;;
    2) RESOLUTION="1080p" ;;
  esac
  
  echo "Starting client $i: $RESOLUTION $PROTOCOL"
  ./client 127.0.0.1 8080 $RESOLUTION $PROTOCOL > client_$i.log 2>&1 &
  
  # Wait a second between client starts to give time for connection setup
  sleep 1
done

echo "All clients started. Press Enter to check status..."
read

# Check which clients are running
ps aux | grep -E "\./client|\./server" | grep -v grep

echo "Press Enter to view client logs..."
read

# Show summaries of client logs
for i in {1..10}; do
  echo "=== Client $i ==="
  if [ -f client_$i.log ]; then
    head -n 10 client_$i.log
    echo "..."
  else
    echo "Log file not found"
  fi
  echo
done

echo "Press Enter to stop all clients and server..."
read

# Kill all clients and server
pkill -f "./client|./server"

echo "All processes stopped." 