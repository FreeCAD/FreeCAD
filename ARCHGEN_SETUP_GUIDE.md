# ArchGen CAD Setup and Build Guide

## Overview

This guide explains the complete setup for ArchGen CAD, an AI-powered fork of FreeCAD with integrated text-to-CAD generation capabilities.

## What Has Been Implemented

### 1. Core Branding Changes ✅

**Root CMakeLists.txt** (`/CMakeLists.txt`)
- Project name changed from `FreeCAD` to `ArchGenCAD`
- Added `PACKAGE_NAME` and `APPLICATION_NAME` variables
- Package string now uses "ArchGen CAD" branding

### 2. ArchGenAI Module ✅

Complete Python module created at `src/Mod/ArchGenAI/` with the following files:

#### Module Files
- **Init.py**: Module initialization and metadata
- **InitGui.py**: GUI initialization and workbench registration
- **ArchGen_Global.py**: Global toolbar and panel integration
- **ArchGen_Command.py**: Main AI generation command with three subcommands:
  - `ArchGen_Generate`: Opens AI generation dialog
  - `ArchGen_Settings`: Configuration management
  - `ArchGen_About`: About dialog
- **ArchGen_UI.py**: Dockable AI assistant panel
- **CMakeLists.txt**: Build configuration for the module

#### Features Implemented
- ✅ Conversational AI interface with session management
- ✅ Quality threshold controls (0.6 - 0.9)
- ✅ Adjustable max iterations (1-10)
- ✅ Temperature control for LLM
- ✅ Chat history with message persistence
- ✅ Configuration file management
- ✅ API integration with timeout handling
- ✅ Script sanitization for security
- ✅ FreeCAD script execution with error handling
- ✅ Dockable AI panel with quick actions
- ✅ Example prompts for easy start

### 3. Custom Theme ✅

**ArchGen.qss** (`src/Gui/Stylesheets/ArchGen.qss`)
- Modern dark theme with blue (#4a90e2) accents
- Custom styling for all UI components
- Special styling for AI components (#ArchGenAIPrompt, #ArchGenAIButton)
- Registered in Stylesheets CMakeLists.txt

### 4. Module Registration ✅

- ArchGenAI added to `src/Mod/CMakeLists.txt`
- Module builds unconditionally (always included)

## What Needs to Be Done

### 1. Backend API Setup ⚠️

The backend AI workflow needs to be set up separately:

**Required Files** (to be placed in `src/Mod/ArchGenAI/backend/`):
- **main.py**: Multi-agent AI workflow implementation
- **api.py**: FastAPI server for REST API
- **examples.json**: RAG training examples
- **run_freecad.bat**: Windows FreeCAD execution wrapper

**Installation Steps**:

```bash
cd src/Mod/ArchGenAI/backend/

# Install Python dependencies
pip install fastapi uvicorn requests langchain langchain-community \
            sentence-transformers scikit-learn pydantic numpy

# Copy your main.py and api.py files to this directory
# (The complete implementations were provided in your original message)

# Update API key in api.py:
# OPENROUTER_API_KEY = "your-api-key-here"

# Create examples.json with your RAG training data

# Run the API server
python api.py
```

The API will start on `http://localhost:8000` by default.

### 2. C++ Integration for Global Toolbar (Optional Advanced)

For the AI toolbar to initialize automatically on startup, add this to `src/Gui/MainWindow.cpp`:

```cpp
// After existing UI setup (around line 150 in MainWindow constructor)
void MainWindow::initializeArchGenAIToolbar()
{
    try {
        PyRun_SimpleString("import sys; sys.path.append('Mod/ArchGenAI')");
        PyRun_SimpleString("import ArchGen_Global; ArchGen_Global.initialize_ai_toolbar()");
    }
    catch (...) {
        Base::Console().Warning("Failed to initialize ArchGen AI toolbar\n");
    }
}
```

And call it in the constructor:
```cpp
initializeArchGenAIToolbar();
```

**Note**: Currently, the toolbar will be initialized when the ArchGenAI workbench is first loaded. The C++ integration makes it available immediately.

### 3. Visual Assets (TODO)

Create and add these files:

**Icons** (to be created):
- `src/Gui/Icons/archgen.ico` - Windows application icon
- `src/Gui/Icons/archgen.svg` - Linux application icon
- `src/Gui/Icons/archgen.icns` - macOS application icon
- `src/Mod/ArchGenAI/Resources/archgen-ai-icon.svg` - AI command icon

**Splash Screen**:
- `src/Gui/Images/splash_image.png` - ArchGen startup splash screen

### 4. Application Core Identity (Advanced)

For complete rebranding, update these C++ files:

**src/App/Application.cpp** (around line 150):
```cpp
std::map<std::string,std::string> cfg;
cfg["ExeName"] = "ArchGenCAD";
cfg["ExeVendor"] = "ArchGen";
cfg["AppName"] = "ArchGen CAD";
cfg["ConsoleBanner"] = "ArchGen CAD 1.0 - The AI-Powered CAD Platform\n";
```

**src/Gui/Application.cpp** (around line 200):
```cpp
Config()["AppName"] = "ArchGen CAD";
Config()["CopyrightInfo"] = "© 2024 ArchGen. Built on FreeCAD technology.";
```

## Building ArchGen CAD

### Prerequisites

Same as FreeCAD:
- CMake 3.22 or higher
- C++17 compatible compiler
- Qt 5.15 or higher
- Python 3.8+
- All FreeCAD dependencies (OpenCASCADE, Coin3D, etc.)

### Build Instructions

#### Linux/macOS

```bash
# Clone the repository
cd ~/ArchgenCAD

# Create build directory
mkdir build
cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build (use all available cores)
make -j$(nproc)

# Install (optional)
sudo make install
```

#### Windows

```bash
# Using Visual Studio
mkdir build
cd build

cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release

# Or using MSBuild directly
MSBuild ArchGenCAD.sln /p:Configuration=Release
```

### Running ArchGen CAD

After building:

```bash
# Linux/macOS
./build/bin/FreeCAD  # Note: executable name may still be FreeCAD

# Windows
.\build\bin\Release\FreeCAD.exe
```

## Using the AI Features

### First-Time Setup

1. **Start the Backend API**:
   ```bash
   cd src/Mod/ArchGenAI/backend/
   python api.py
   ```
   The API should start on http://localhost:8000

2. **Launch ArchGen CAD**

3. **Configure AI Settings**:
   - Open: Menu → Tools → ArchGen AI → Settings
   - Set API URL: http://localhost:8000
   - Choose your preferred model
   - Set quality threshold (0.7 recommended)

4. **Generate Your First Model**:
   - Click the "🤖 Generate with AI" toolbar button, or
   - Press Ctrl+G, or
   - Menu → Tools → ArchGen AI → Generate with AI

### Configuration File

Settings are stored in:
- **Linux**: `~/.local/share/FreeCAD/ArchGen_config_v3.json`
- **macOS**: `~/Library/Application Support/FreeCAD/ArchGen_config_v3.json`
- **Windows**: `%APPDATA%/FreeCAD/ArchGen_config_v3.json`

Example configuration:
```json
{
    "api_url": "http://localhost:8000",
    "session_id": "uuid-here",
    "model_name": "qwen/qwen3-coder:free",
    "quality_threshold": 0.7,
    "max_iterations": 5,
    "temperature": 0.1
}
```

## AI Generation Workflow

The AI generation uses a multi-agent workflow:

1. **Prompt Validation**: Validates and enhances your prompt
2. **CAD Generation**: Generates FreeCAD Python script using RAG
3. **Syntax Validation**: Checks Python syntax
4. **Execution Validation**: Tests script execution
5. **Quality Assessment**: Evaluates the generated model
6. **Refinement** (if needed): Iteratively improves the script

Quality thresholds:
- **0.6**: Fast, acceptable quality
- **0.7**: Standard, good quality (recommended)
- **0.8**: High quality, slower
- **0.9**: Very high quality, slowest

## Troubleshooting

### API Connection Failed

**Error**: "Network error: Connection refused"

**Solutions**:
1. Ensure backend API is running: `python api.py`
2. Check API URL in settings matches the running server
3. Verify firewall isn't blocking port 8000

### Module Not Loading

**Error**: "ArchGen AI toolbar not found"

**Solutions**:
1. Check build completed successfully
2. Verify ArchGenAI module exists in installation
3. Check FreeCAD console for Python errors

### Script Generation Failed

**Error**: "API returned failure without details"

**Solutions**:
1. Check backend API logs for errors
2. Verify OpenRouter API key is set correctly
3. Try a simpler prompt first
4. Increase max iterations in settings

### Theme Not Applied

**Issue**: ArchGen theme not appearing in preferences

**Solutions**:
1. Ensure `ArchGen.qss` is in Stylesheets directory
2. Rebuild the application
3. Select theme: Edit → Preferences → Display → Stylesheet

## Development

### Adding New Features

The modular structure makes it easy to extend:

- **New Commands**: Add to `ArchGen_Command.py`
- **UI Enhancements**: Modify `ArchGen_UI.py`
- **Backend Changes**: Update `backend/main.py` or `backend/api.py`
- **Theme Customization**: Edit `src/Gui/Stylesheets/ArchGen.qss`

### Debug Mode

Enable verbose logging:

```python
# In ArchGen_Command.py
import logging
logging.basicConfig(level=logging.DEBUG)
```

## Architecture Overview

```
ArchGen CAD
├── FreeCAD Core (C++)
│   ├── Application (branded as ArchGen CAD)
│   ├── Gui (with custom theme)
│   └── Modules
├── ArchGenAI Module (Python)
│   ├── Commands (Generate, Settings, About)
│   ├── UI (Dialog, Panel)
│   ├── Global Integration (Toolbar)
│   └── Backend Integration (API client)
└── Backend API (FastAPI)
    ├── AI Workflow (Multi-agent)
    ├── RAG System (examples.json)
    └── FreeCAD Script Generation
```

## API Endpoints

The backend provides these endpoints:

- `GET /` - API information and status
- `POST /generate-cad` - Generate CAD script from prompt
- `GET /sessions` - List active sessions
- `GET /conversation/{session_id}` - Get conversation history
- `DELETE /conversation/{session_id}` - Clear conversation
- `GET /health` - Health check
- `GET /rag/stats` - RAG system statistics

## Credits

- **ArchGen CAD**: AI-powered enhancements and branding
- **FreeCAD**: Base CAD platform
- **LangChain**: AI workflow orchestration
- **OpenRouter**: LLM API access

## License

ArchGen CAD is built on FreeCAD and inherits its LGPL license.

## Support

For issues and questions:
- Check the troubleshooting section above
- Review FreeCAD console for error messages
- Examine backend API logs
- Verify all dependencies are installed

---

**Version**: 3.0
**Last Updated**: 2024
**Status**: Development Build
