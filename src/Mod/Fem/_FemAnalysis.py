class _FemAnalysis:
    "The FemAnalysis container object"
    def __init__(self, obj):
        self.Type = "FemAnalysis"
        obj.Proxy = self
        obj.addProperty("App::PropertyString", "OutputDir", "Base", "Directory where the jobs get generated")

    def execute(self, obj):
        return

    def onChanged(self, obj, prop):
        if prop in ["MaterialName"]:
            return

    def __getstate__(self):
        return self.Type

    def __setstate__(self, state):
        if state:
            self.Type = state
