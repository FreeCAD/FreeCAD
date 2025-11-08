# ArchGen CAD - Quick Start Checklist

Use this checklist to get ArchGen CAD up and running quickly.

## ☐ Prerequisites

- [ ] CMake 3.22+ installed
- [ ] C++ compiler (GCC/Clang/MSVC)
- [ ] Python 3.8+ installed
- [ ] Qt 5.15+ installed
- [ ] FreeCAD build dependencies installed
- [ ] OpenRouter API account created
- [ ] OpenRouter API key obtained

## ☐ Backend Setup

### 1. Navigate to Backend Directory
```bash
cd src/Mod/ArchGenAI/backend/
```

### 2. Install Python Dependencies
```bash
pip install -r requirements.txt
```

### 3. Add Your Backend Files

- [ ] Copy your `main.py` to `src/Mod/ArchGenAI/backend/main.py`
- [ ] Copy your `api.py` to `src/Mod/ArchGenAI/backend/api.py`
- [ ] Create or copy `examples.json` with RAG training data

### 4. Configure API Key

Open `api.py` and update:
```python
OPENROUTER_API_KEY = "sk-or-v1-YOUR-ACTUAL-KEY-HERE"
```

### 5. Test Backend

```bash
# Start the server
python api.py

# You should see:
# 🚀 Starting Enhanced Text-to-CAD Workflow API
# 🌐 Server will start on: http://0.0.0.0:8000

# In another terminal, test:
curl http://localhost:8000/health
```

- [ ] Backend starts without errors
- [ ] Health endpoint returns `"status": "healthy"`

## ☐ Build ArchGen CAD

### 1. Create Build Directory
```bash
# From repository root
mkdir -p build
cd build
```

### 2. Configure CMake
```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

- [ ] CMake configuration succeeds
- [ ] ArchGenAI module is found
- [ ] No critical errors

### 3. Build Project
```bash
# Linux/macOS
make -j$(nproc)

# Windows
cmake --build . --config Release
```

- [ ] Build completes without errors
- [ ] ArchGenAI module is built

### 4. Verify Build
```bash
# Check module exists
ls Mod/ArchGenAI/

# Should see:
# Init.py, InitGui.py, ArchGen_Command.py, etc.
```

## ☐ First Run

### 1. Start Backend (if not running)
```bash
cd src/Mod/ArchGenAI/backend/
python api.py
```

### 2. Launch ArchGen CAD
```bash
# From build directory
./bin/FreeCAD  # Linux/macOS
# or
.\bin\Release\FreeCAD.exe  # Windows
```

- [ ] ArchGen CAD launches successfully
- [ ] No critical Python errors in console

### 3. Load ArchGen AI Module

In FreeCAD:
- Method 1: Open any workbench dropdown, select "ArchGen AI"
- Method 2: Wait for auto-initialization (if C++ integration done)

- [ ] ArchGen AI toolbar appears
- [ ] No errors in Python console

## ☐ Configure AI Features

### 1. Open Settings
Menu: `Tools → ArchGen AI → Settings`

Or toolbar: `⚙️ ArchGen Settings`

### 2. Configure API Connection
- [ ] API URL: `http://localhost:8000`
- [ ] Model: `qwen/qwen3-coder:free` (or your choice)
- [ ] Quality Threshold: `0.7`
- [ ] Max Iterations: `5`
- [ ] Temperature: `0.1`
- [ ] Click **Save**

### 3. Verify Configuration
Config file should be created at:
- Linux: `~/.local/share/FreeCAD/ArchGen_config_v3.json`
- macOS: `~/Library/Application Support/FreeCAD/ArchGen_config_v3.json`
- Windows: `%APPDATA%/FreeCAD/ArchGen_config_v3.json`

- [ ] Config file exists
- [ ] Contains correct settings

## ☐ Test AI Generation

### 1. Open AI Dialog

- Click toolbar: `🤖 Generate with AI`, or
- Press: `Ctrl+G`, or
- Menu: `Tools → ArchGen AI → Generate with AI`

- [ ] Dialog opens
- [ ] No errors

### 2. Generate Simple Test Model

Enter prompt:
```
Create a simple cube 10x10x10mm
```

Settings:
- [ ] Quality: `0.7 (Standard)`
- [ ] Max Iterations: `5`
- [ ] New Session: `✓ Checked`

Click: `🚀 Generate CAD Model`

### 3. Wait for Generation

You should see:
1. "Sending prompt to ArchGen AI..."
2. "Waiting for API response..."
3. "API response received..."
4. "Rendering model..."
5. "✅ Success! Model generated successfully"

- [ ] API connection succeeds
- [ ] Script is generated
- [ ] Script executes without errors
- [ ] Model appears in viewport

### 4. Verify Result

- [ ] New document created (e.g., "ArchGen_GeneratedModel_timestamp")
- [ ] Cube visible in 3D view
- [ ] Dimensions approximately correct
- [ ] No Python errors

## ☐ Test Advanced Features

### 1. Test Session Continuity

In same dialog (don't close):
- [ ] Uncheck "New Session"
- [ ] Enter: "Make it bigger, 20x20x20mm"
- [ ] Generate again
- [ ] Session ID remains the same
- [ ] Model updates

### 2. Test Settings

- [ ] Open Settings again
- [ ] Change quality threshold to `0.8`
- [ ] Save and close
- [ ] Generate another model
- [ ] Higher quality enforced (more iterations if needed)

### 3. Test AI Panel

Menu: `View → Panels → ArchGen AI Assistant` (if available)

- [ ] Panel docks to right side
- [ ] Quick prompt works
- [ ] Example buttons fill prompt
- [ ] Recent generations list updates

## ☐ Apply Custom Theme (Optional)

### 1. Open Preferences
Menu: `Edit → Preferences`

### 2. Go to Display
- [ ] Navigate to: `Display` → `Stylesheet`
- [ ] Find `ArchGen` in dropdown
- [ ] Select it
- [ ] Click `Apply` or `OK`

### 3. Restart ArchGen CAD
- [ ] Close and reopen
- [ ] Theme applied (dark with blue accents)

## ☐ Production Checklist

### Security
- [ ] API key not committed to git
- [ ] Config files in .gitignore
- [ ] Backend not exposed to internet (unless needed)

### Performance
- [ ] Backend server stable
- [ ] No memory leaks observed
- [ ] Generations complete in reasonable time

### Quality
- [ ] Test 5-10 different prompts
- [ ] Success rate acceptable (>70%)
- [ ] Quality scores generally >0.7

### Documentation
- [ ] Team knows how to use features
- [ ] Setup guide accessible
- [ ] Troubleshooting guide reviewed

## ☐ Optional Enhancements

### Custom Icons
- [ ] Create custom icons (ico, svg, icns)
- [ ] Add to `src/Gui/Icons/`
- [ ] Update references
- [ ] Rebuild

### C++ Branding
- [ ] Update `src/App/Application.cpp`
- [ ] Update `src/Gui/Application.cpp`
- [ ] Rebuild
- [ ] Verify executable name

### Start Page Customization
- [ ] Customize StartView widget (C++)
- [ ] Add ArchGen branding
- [ ] Rebuild

### Examples Database
- [ ] Expand `examples.json`
- [ ] Add 20+ quality examples
- [ ] Cover different categories
- [ ] Test RAG improvements

## 🚨 Troubleshooting

If something doesn't work, check:

### Build Issues
1. ✓ All dependencies installed?
2. ✓ CMake found ArchGenAI module?
3. ✓ Check build logs for errors

### Backend Issues
1. ✓ Python dependencies installed?
2. ✓ API key set correctly?
3. ✓ Port 8000 not blocked?
4. ✓ examples.json valid JSON?

### Connection Issues
1. ✓ Backend running?
2. ✓ API URL correct in settings?
3. ✓ Network/firewall not blocking?
4. ✓ Check backend logs

### Generation Issues
1. ✓ Prompt clear and specific?
2. ✓ Model available on OpenRouter?
3. ✓ Quality threshold not too high?
4. ✓ Check backend terminal for errors

## 📚 Reference Documents

- `IMPLEMENTATION_SUMMARY.md` - What's been done
- `ARCHGEN_SETUP_GUIDE.md` - Detailed setup guide
- `backend/BACKEND_SETUP.md` - Backend-specific help
- `backend/README.md` - Backend overview

## ✅ Success!

When all items are checked, you have:

✨ **A fully functional AI-powered CAD platform!**

Time to start creating amazing models with AI! 🚀

---

**Need Help?**

1. Check documentation files listed above
2. Review console/terminal logs
3. Test components individually
4. Verify API connectivity

**Estimated Setup Time**: 30-60 minutes

**Good luck!** 🎉
