	Q_OBJECT
	public:
		GuiNativeEvent(GUIApplicationNativeEventAware *app);
		~GuiNativeEvent();
		void initSpaceball(QMainWindow *window);
	private:
		GuiNativeEvent();
		GuiNativeEvent(const GuiNativeEvent&);
		GuiNativeEvent& operator=(const GuiNativeEvent&);
		GUIApplicationNativeEventAware *mainApp;
        static int  motionDataArray[6];

