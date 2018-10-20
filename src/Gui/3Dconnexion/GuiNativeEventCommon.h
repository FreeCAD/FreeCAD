	public:
		GuiNativeEvent(GUIApplicationNativeEventAware *app);
		~GuiNativeEvent();
		void initSpaceball(QMainWindow *window);
	private:
		GuiNativeEvent();
		GuiNativeEvent(const GuiNativeEvent&);
		GuiNativeEvent& operator=(const GuiNativeEvent&);
		GUIApplicationNativeEventAware *mainApp;
        int  motionDataArray[6];

