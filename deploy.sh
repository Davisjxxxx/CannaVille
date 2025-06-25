#!/bin/bash
# CannaVille Deployment Script

echo "🌱 Deploying CannaVille Pro..."

# Install dependencies
echo "📦 Installing dependencies..."
npm install
pip3 install -r requirements.txt

# Create necessary directories
mkdir -p assets/{models,textures/ai_generated,sounds,shaders}
mkdir -p logs

# Download base models (you'll need to provide these)
echo "📥 Downloading base models..."
# wget -O assets/models/avatars/male_caucasian.glb "YOUR_MODEL_URL"

# Start AI texture generation service
echo "🤖 Starting AI services..."
python3 api/ai-texture-generator.py &
AI_PID=$!

# Start main server
echo "🚀 Starting main server..."
node server.js &
SERVER_PID=$!

echo "✅ CannaVille Pro deployed successfully!"
echo "🌐 Access at: http://localhost:3000"
echo "🤖 AI API at: http://localhost:5000"

# Save PIDs for cleanup
echo $AI_PID > ai.pid
echo $SERVER_PID > server.pid

echo "💡 To stop: ./stop.sh"
