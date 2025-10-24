from flask import Flask, request, jsonify
from datetime import datetime

app = Flask(__name__)

# In-memory storage for latest data
latest_data = {}

@app.route("/data", methods=["POST"])
def receive_data():
    global latest_data
    data = request.get_json()
    if not data:
        return jsonify({"status": "error", "message": "No JSON received"}), 400

    # Add timestamp ter
    data['timestamp'] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    latest_data = data
    print("✅ Data received:", data)
    return jsonify({"status": "success"}), 200

@app.route("/latest", methods=["GET"])
def get_latest():
    return jsonify(latest_data)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
