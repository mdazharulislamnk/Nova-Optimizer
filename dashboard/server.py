import mmap
import struct
import json
import os
import subprocess
from http.server import HTTPServer, SimpleHTTPRequestHandler

FORMAT = 'Q d Q Q d'
SIZE = struct.calcsize(FORMAT)

ENGINE_PROCESS = None

def get_engine_path():
    # The exe is located at ../build/Release/Nova_Optimizer.exe relative to server.py
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base_dir, "build", "Release", "Nova_Optimizer.exe")

class DashboardServer(SimpleHTTPRequestHandler):
    def do_GET(self):
        global ENGINE_PROCESS
        if self.path == '/api/pulse':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            
            shmem_name = "Local\\NovaOptimizer_Pulse"
            try:
                # Read 64 bytes from named shared memory
                shm = mmap.mmap(-1, 64, tagname=shmem_name, access=mmap.ACCESS_READ)
                data = shm.read(SIZE)
                shm.close()
                seq, cpu, total_ram, used_ram, ram_percent = struct.unpack(FORMAT, data)
                
                response = {
                    "status": "online",
                    "sequence": seq,
                    "cpu_usage_percent": round(cpu, 1),
                    "total_ram_bytes": total_ram,
                    "used_ram_bytes": used_ram,
                    "ram_usage_percent": round(ram_percent, 1),
                    "process_running": (ENGINE_PROCESS is not None and ENGINE_PROCESS.poll() is None)
                }
            except Exception as e:
                # C++ engine is probably offline
                response = {
                    "status": "offline",
                    "cpu_usage_percent": 0.0,
                    "ram_usage_percent": 0.0,
                    "used_ram_bytes": 0,
                    "total_ram_bytes": 1, 
                    "error": str(e),
                    "process_running": (ENGINE_PROCESS is not None and ENGINE_PROCESS.poll() is None)
                }
            
            self.wfile.write(json.dumps(response).encode())
        elif self.path == '/api/test_logs':
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.send_header('Access-Control-Allow-Origin', '*')
            self.end_headers()
            
            try:
                log_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "test_logs.json")
                if os.path.exists(log_file):
                    with open(log_file, "r") as f:
                        logs = f.read()
                    self.wfile.write(logs.encode())
                else:
                    self.wfile.write(json.dumps([]).encode())
            except Exception as e:
                self.wfile.write(json.dumps([{"error": str(e)}]).encode())
        else:
            # Serve static files normally (index.html, style.css, script.js)
            super().do_GET()

    def do_POST(self):
        global ENGINE_PROCESS
        
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()

        if self.path == '/api/engine/start':
            if ENGINE_PROCESS is None or ENGINE_PROCESS.poll() is not None:
                engine_path = get_engine_path()
                if os.path.exists(engine_path):
                    # Start the engine asynchronously
                    ENGINE_PROCESS = subprocess.Popen(
                        [engine_path], 
                        creationflags=subprocess.CREATE_NEW_CONSOLE
                    )
                    self.wfile.write(json.dumps({"success": True, "message": "Engine Started"}).encode())
                else:
                    self.wfile.write(json.dumps({"success": False, "message": "Engine executable not found! Please build first."}).encode())
            else:
                self.wfile.write(json.dumps({"success": True, "message": "Engine already running"}).encode())
                
        elif self.path == '/api/engine/stop':
            if ENGINE_PROCESS is not None and ENGINE_PROCESS.poll() is None:
                # Send CTRL+C equivalent or terminate
                subprocess.run(['taskkill', '/F', '/T', '/PID', str(ENGINE_PROCESS.pid)], capture_output=True)
                ENGINE_PROCESS = None
                self.wfile.write(json.dumps({"success": True, "message": "Engine Stopped"}).encode())
            else:
                # Fallback taskkill by name just in case
                subprocess.run(['taskkill', '/F', '/IM', 'Nova_Optimizer.exe'], capture_output=True)
                ENGINE_PROCESS = None
                self.wfile.write(json.dumps({"success": True, "message": "Engine was not running or Force Stopped"}).encode())
                
        elif self.path == '/api/engine/start_test':
            content_length = int(self.headers.get('Content-Length', 0))
            if content_length > 0:
                post_data = self.rfile.read(content_length)
                try:
                    payload = json.loads(post_data.decode('utf-8'))
                    duration = payload.get('duration_minutes', 1.0)
                except json.JSONDecodeError:
                    duration = 1.0
            else:
                duration = 1.0
                
            test_script = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "test_shared_memory.py")
            if os.path.exists(test_script):
                # Run the test script asynchronously and don't block
                subprocess.Popen(
                    ["python", test_script, "--duration", str(duration)],
                    creationflags=subprocess.CREATE_NEW_CONSOLE
                )
                self.wfile.write(json.dumps({"success": True, "message": f"Test script started for {duration} minutes"}).encode())
            else:
                self.wfile.write(json.dumps({"success": False, "message": "Test script not found"}).encode())

if __name__ == '__main__':
    # Change working directory so SimpleHTTPRequestHandler serves files from here
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    server_address = ('', 8080)
    httpd = HTTPServer(server_address, DashboardServer)
    print("==========================================")
    print(" Nova-Optimizer Web Dashboard Server")
    print(" Serving at http://localhost:8080")
    print("==========================================")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        if ENGINE_PROCESS is not None and ENGINE_PROCESS.poll() is None:
            subprocess.run(['taskkill', '/F', '/T', '/PID', str(ENGINE_PROCESS.pid)], capture_output=True)
        print("\nShutting down dashboard server.")
