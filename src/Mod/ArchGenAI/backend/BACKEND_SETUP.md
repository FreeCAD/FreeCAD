# ArchGen AI Backend Setup Guide

## Quick Start

This backend provides the AI-powered text-to-CAD generation service for ArchGen CAD.

## Prerequisites

```bash
pip install fastapi uvicorn requests langchain langchain-community \
            sentence-transformers scikit-learn pydantic numpy torch
```

## Required Files

Place these files in this directory (`src/Mod/ArchGenAI/backend/`):

### 1. main.py
Your complete AI workflow implementation with:
- Multi-agent orchestration
- RAG system integration
- Validation pipeline
- Quality assessment
- Iterative refinement

### 2. api.py
FastAPI server implementation with endpoints:
- `/generate-cad` - Main generation endpoint
- `/sessions` - Session management
- `/conversation/{session_id}` - Conversation history
- `/health` - Health check
- `/rag/stats` - RAG statistics

### 3. examples.json
Training examples for the RAG system. Format:
```json
[
    {
        "prompt": "Create a hexagonal bolt with M8 threading",
        "script": "import FreeCAD as App\n...",
        "tags": ["bolt", "threading", "mechanical"],
        "category": "mechanical",
        "components": ["threads", "hexagonal_head"],
        "chain_of_thoughts": "First create cylinder, then add threads...",
        "engineering_concepts": ["threading", "bolt_standard"],
        "freecad_apis": ["Part.makeCylinder", "Part.makeThread"],
        "validation_criteria": ["proper_threading", "correct_dimensions"]
    }
]
```

### 4. run_freecad.bat (Windows only)
Batch script for executing FreeCAD scripts on Windows:
```batch
@echo off
"C:\Path\To\FreeCAD\bin\FreeCAD.exe" -c %1
```

## Configuration

### 1. Set Your API Key

Edit `api.py` and set your OpenRouter API key:

```python
OPENROUTER_API_KEY = "sk-or-v1-your-actual-api-key-here"
```

### 2. Configure Model

Default model in `api.py`:
```python
DEFAULT_MODEL = "qwen/qwen3-coder:free"
```

Available free models:
- `qwen/qwen3-coder:free` - Fast, code-focused
- `deepseek/deepseek-chat-v3.1:free` - Balanced
- `meta-llama/llama-3.1-8b-instruct:free` - General purpose

### 3. Adjust Settings

In `api.py`:
```python
MAX_SESSIONS = 100  # Maximum concurrent sessions
SESSION_TIMEOUT_HOURS = 24  # Session expiry
```

## Running the Backend

### Development Mode

```bash
cd src/Mod/ArchGenAI/backend/
python api.py
```

The server will start on `http://localhost:8000`

You should see:
```
🚀 Starting Enhanced Text-to-CAD Workflow API
🔑 API Key: sk-or-v1...
📝 RAG Examples: examples.json
🌐 Server will start on: http://0.0.0.0:8000
📚 Docs will be available at: http://0.0.0.0:8000/docs
```

### Production Mode

Using uvicorn directly with more options:

```bash
uvicorn api:app --host 0.0.0.0 --port 8000 --workers 4
```

### Using ngrok (for remote access)

```bash
# Install ngrok
# Download from https://ngrok.com/

# Start ngrok tunnel
ngrok http 8000

# Use the ngrok URL in ArchGen CAD settings
# Example: https://abc123.ngrok.io
```

## Testing the API

### 1. Health Check

```bash
curl http://localhost:8000/health
```

Expected response:
```json
{
    "status": "healthy",
    "system_initialized": true,
    "rag_system_loaded": true,
    "orchestrator_ready": true
}
```

### 2. Test Generation

```bash
curl -X POST http://localhost:8000/generate-cad \
  -H "Content-Type: application/json" \
  -d '{
    "prompt": "Create a simple cube 10x10x10mm",
    "new_session": true,
    "model_name": "qwen/qwen3-coder:free",
    "min_quality_score": 0.7
  }'
```

### 3. View API Documentation

Open in browser: http://localhost:8000/docs

This provides interactive API documentation with:
- All endpoints
- Request/response schemas
- Try-it-out functionality

## File Structure

```
backend/
├── BACKEND_SETUP.md (this file)
├── README.md (overview)
├── api.py (FastAPI server)
├── main.py (AI workflow)
├── examples.json (RAG training data)
├── run_freecad.bat (Windows helper)
└── __pycache__/ (auto-generated)
```

## API Request Examples

### Generate CAD Model

```python
import requests

response = requests.post(
    "http://localhost:8000/generate-cad",
    json={
        "prompt": "Design a gear with 20 teeth and 5mm thickness",
        "new_session": True,
        "model_name": "qwen/qwen3-coder:free",
        "min_quality_score": 0.7,
        "max_iterations": 5,
        "temperature": 0.1
    }
)

data = response.json()
if data["success"]:
    print(f"Script generated! Score: {data['final_score']}")
    print(f"Iterations: {data['iterations']}")
    print(f"Script:\n{data['freecad_script']}")
else:
    print(f"Failed: {data['error_summary']}")
```

### Continue Conversation

```python
# Use the session_id from previous response
response = requests.post(
    "http://localhost:8000/generate-cad",
    json={
        "prompt": "Make it bigger, 50mm diameter",
        "session_id": "previous-session-id-here",
        "new_session": False
    }
)
```

### Get Conversation History

```python
session_id = "your-session-id"
response = requests.get(f"http://localhost:8000/conversation/{session_id}")
conversation = response.json()

print(f"Total messages: {conversation['total_messages']}")
for msg in conversation['messages']:
    print(f"{msg['role']}: {msg['content'][:50]}...")
```

## Environment Variables (Optional)

You can use environment variables instead of hardcoding:

```bash
export OPENROUTER_API_KEY="sk-or-v1-your-key"
export DEFAULT_MODEL="qwen/qwen3-coder:free"
export API_PORT="8000"
```

Then in `api.py`:
```python
import os
OPENROUTER_API_KEY = os.getenv("OPENROUTER_API_KEY")
DEFAULT_MODEL = os.getenv("DEFAULT_MODEL", "qwen/qwen3-coder:free")
```

## Logging

The backend logs important events. To increase verbosity:

```python
# In api.py or main.py
logging.basicConfig(
    level=logging.DEBUG,  # Change from INFO to DEBUG
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
```

## Performance Tuning

### 1. RAG System

```python
# In main.py - LocalJSONRAG class
def __init__(self, json_file_path: str, embedding_model: str = "all-MiniLM-L6-v2"):
    # For faster embeddings (less accurate):
    # embedding_model = "paraphrase-MiniLM-L3-v2"

    # For better quality (slower):
    # embedding_model = "all-mpnet-base-v2"
```

### 2. Model Selection

Fastest to slowest:
1. `qwen/qwen3-coder:free` - Fast, code-focused
2. `deepseek/deepseek-chat-v3.1:free` - Balanced
3. `meta-llama/llama-3.1-8b-instruct:free` - Slower, more capable

### 3. Quality vs Speed

```python
# Fast generation (lower quality)
min_quality_score = 0.6
max_iterations = 3

# Balanced (recommended)
min_quality_score = 0.7
max_iterations = 5

# High quality (slower)
min_quality_score = 0.8
max_iterations = 8
```

## Troubleshooting

### Import Errors

```bash
# If you get "ModuleNotFoundError"
pip install --upgrade pip
pip install -r requirements.txt  # Create this file with dependencies
```

### API Key Invalid

Error: "OpenRouter API key is required"

Solution:
1. Get API key from https://openrouter.ai/
2. Set in api.py: `OPENROUTER_API_KEY = "sk-or-v1-..."`
3. Restart the server

### RAG System Not Loading

Error: "JSON file not found: examples.json"

Solution:
1. Create `examples.json` with at least one example
2. Or use empty array: `[]`
3. Restart the server

### Port Already in Use

Error: "Address already in use"

Solution:
```bash
# Find process using port 8000
lsof -i :8000  # Linux/Mac
netstat -ano | findstr :8000  # Windows

# Kill the process or use different port
uvicorn api:app --port 8001
```

## Security Considerations

### 1. API Key Protection

Never commit API keys to version control:
```bash
# Add to .gitignore
*.env
config.json
```

### 2. Input Validation

The backend includes:
- Script sanitization
- Dangerous pattern removal
- Timeout limits
- Session cleanup

### 3. Network Security

For production:
```python
# Add authentication
from fastapi import Depends, HTTPException
from fastapi.security import HTTPBearer

security = HTTPBearer()

@app.post("/generate-cad")
async def generate_cad(request: CADGenerationRequest, credentials = Depends(security)):
    # Verify credentials
    ...
```

## Monitoring

### View Active Sessions

```bash
curl http://localhost:8000/sessions
```

### Check RAG Statistics

```bash
curl http://localhost:8000/rag/stats
```

### Clean Up Old Sessions

```bash
curl -X POST http://localhost:8000/sessions/cleanup
```

## Next Steps

1. ✅ Install dependencies
2. ✅ Copy main.py and api.py to this directory
3. ✅ Set your API key
4. ✅ Create examples.json with training data
5. ✅ Run the server: `python api.py`
6. ✅ Test the API: http://localhost:8000/docs
7. ✅ Configure ArchGen CAD to use this API
8. ✅ Start generating CAD models!

## Support

For issues:
1. Check server logs in terminal
2. Test with curl/Postman first
3. Verify API key and model availability
4. Check examples.json format

---

**Happy AI CAD Generation!** 🚀
