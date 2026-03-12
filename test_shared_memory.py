import mmap
import struct
import time
import os
import argparse
import json
from datetime import datetime

# Define the struct format based on C++ layout
FORMAT = 'Q d Q Q d'
SIZE = struct.calcsize(FORMAT)

def read_pulse_data(duration_minutes):
    shmem_name = "Local\\NovaOptimizer_Pulse"
    try:
        shm = mmap.mmap(-1, 64, tagname=shmem_name, access=mmap.ACCESS_READ)
    except Exception as e:
        print(f"Could not open shared memory: {e}")
        return

    print(f"Connected to Shared Memory! Listening for pulse data for {duration_minutes} minutes...")
    last_seq = -1
    
    end_time = time.time() + (duration_minutes * 60)
    
    logs = []
    log_file_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "dashboard", "test_logs.json")
    
    # Initialize empty log file
    with open(log_file_path, "w") as f:
        json.dump([], f)

    try:
        while time.time() < end_time:
            data = shm.read(SIZE)
            shm.seek(0)
            
            unpacked = struct.unpack(FORMAT, data)
            seq, cpu, total_ram, used_ram, ram_percent = unpacked
            
            if seq != last_seq and seq % 2 == 0:
                log_entry = {
                    "timestamp": datetime.now().strftime("%H:%M:%S.%f")[:-3],
                    "sequence": seq,
                    "cpu": round(cpu, 1),
                    "ram": round(ram_percent, 1),
                    "used_mb": used_ram // (1024**2),
                    "total_mb": total_ram // (1024**2)
                }
                logs.append(log_entry)
                last_seq = seq
                
                # Keep only last 100 logs in memory/file to prevent bloat
                if len(logs) > 100:
                    logs.pop(0)
                    
                # Write to disk every few ticks so frontend can read it
                with open(log_file_path, "w") as f:
                    json.dump(logs, f)
            
            time.sleep(0.1)
    except KeyboardInterrupt:
        pass
    finally:
        shm.close()
        print("Test sequence finished.")
        # Mark as finished in the JSON log
        logs.append({"timestamp": datetime.now().strftime("%H:%M:%S.%f")[:-3], "status": "FINISHED"})
        with open(log_file_path, "w") as f:
            json.dump(logs, f)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Nova-Optimizer Python Shared Memory Reader')
    parser.add_argument('--duration', type=float, default=1.0, help='Duration to run the test in minutes')
    args = parser.parse_args()
    
    read_pulse_data(args.duration)
