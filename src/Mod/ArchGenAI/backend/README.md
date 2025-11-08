# ArchGen AI Backend

This directory contains the AI backend components for ArchGen CAD.

## Files

- **api.py**: FastAPI server that provides the REST API for CAD generation
- **main.py**: Core AI workflow implementation with multi-agent system
- **examples.json**: Training examples for the RAG system
- **run_freecad.bat**: Windows batch script for FreeCAD execution

## Setup

1. Install dependencies:
```bash
pip install fastapi uvicorn requests langchain langchain-community sentence-transformers scikit-learn pydantic
```

2. Set your OpenRouter API key in api.py

3. Run the API server:
```bash
python api.py
```

4. The server will start on http://localhost:8000

## Usage

The FreeCAD workbench communicates with this backend via REST API calls to generate CAD models from text descriptions.
