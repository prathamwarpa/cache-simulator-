from flask import Flask, request, jsonify, send_from_directory
import subprocess
import os

app = Flask(__name__, static_folder='.')

@app.route('/')
def index():
    return send_from_directory('.', 'index.html')

@app.route('/simulate', methods=['POST'])
def simulate():
    # 1. Save uploaded trace.txt
    trace_file = request.files['trace']
    trace_file.save('trace.txt')

    # 2. Grab slider data from the UI
    blk = request.form['blk']
    l1s = request.form['l1size']
    l1a = request.form['l1assoc']
    l2s = request.form['l2size']
    l2a = request.form['l2assoc']

    # 3. Run your C++ executable with the UI variables
    exe_name = './cache_sim' if os.name != 'nt' else 'cache_sim.exe'
    subprocess.run([exe_name, blk, l1s, l1a, l2s, l2a])

    # 4. Automatically read the output files to send back to the UI
    with open('results.csv', 'r') as f:
        csv_data = f.read()
    
    traces = {}
    for pol in ['lru', 'fifo', 'opt', 'lfu']:
        try:
            with open(f'{pol}_output.txt', 'r') as f:
                traces[pol.upper()] = f.read()
        except FileNotFoundError:
            traces[pol.upper()] = ""
            
    return jsonify({'csv': csv_data, 'traces': traces})

if __name__ == '__main__':
    print("Dashboard running at http://localhost:8080")
    app.run(debug=True, port=8080)