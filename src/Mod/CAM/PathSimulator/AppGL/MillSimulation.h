#ifndef __millsimulation__h__
#define __millsimulation__h__

#include "MillMotion.h"
#include "GCodeParser.h"
#include "Shader.h"
#include "linmath.h"
#include "GlUtils.h"
#include "StockObject.h"
#include "MillPathSegment.h"
#include "EndMillFlat.h"
#include "EndMillBall.h"
#include "EndMillTaper.h"
#include "GuiDisplay.h"
#include <sstream>

namespace MillSim {

	class MillSimulation
	{
	public:
		MillSimulation();
		void ClearMillPathSegments();
		void SimNext();
		void InitSimulation();
		void AddTool(EndMill* tool);
		void Render();
		void ProcessSim(unsigned int time_ms);
		void HandleKeyPress(int key);
		void TiltEye(float tiltStep);
		void RotateEye(float rotStep);
		void InitDisplay();
		bool LoadGCodeFile(const char* fileName);
		bool AddGcodeLine(const char* line);
		void SetSimulationStage(float stage);
		void SetBoxStock(float x, float y, float z, float l, float w, float h);
		void MouseDrag(int buttons, int dx, int dy);
		void MouseHover(int px, int py);
		void MousePress(int button, bool isPressed);


	protected:
		void GlsimStart();
		void GlsimToolStep1(void);
		void GlsimToolStep2(void);
		void GlsimClipBack(void);
		void GlsimRenderStock(void);
		void GlsimRenderTools(void);
		void GlsimEnd(void);
		void renderSegmentForward(int iSeg);
		void renderSegmentReversed(int iSeg);
		void CalcSegmentPositions();
		EndMill* GetTool(int tool);


	protected:
		std::vector<EndMill*> mToolTable;
		Shader shader3D, shaderInv3D, shaderFlat;
		GCodeParser mCodeParser;
		GuiDisplay guiDisplay;
		std::vector<MillPathSegment*> MillPathSegments;
		std::ostringstream mFpsStream;

		MillMotion mZeroPos = { eNop, -1, 0, 0,  10 };
		MillMotion mCurMotion = { eNop, -1, 0, 0,  0, 0, 0, 0 };
		MillMotion mDestMotion = { eNop, -1, 0, 0, 0, 0, 0, 0 };

		StockObject mStockObject;
		StockObject mlightObject;

		vec3 lightColor = { 0.8f, 0.9f, 1.0f };
		vec3 lightPos = { 20.0f, 20.0f, 10.0f };
		vec3 ambientCol = { 0.3f, 0.3f, 0.5f };

		vec3 eye = { 0, 100, 40 };
		vec3 target = { 0, 0, -10 };
		vec3 upvec = { 0, 0, 1 };

		vec3 stockColor = { 0.7f, 0.7f, 0.7f };
		vec3 cutColor = { 0.4f, 0.7f, 0.4f };
		vec3 toolColor = { 0.4f, 0.4f, 0.7f };

		float mEyeDistance = 30;
		float mEyeRoration = 0;
		float mEyeInclination = PI / 6; // 30 degree
		float mEyeStep = PI / 36;  // 5 degree


		int mCurStep = 0;
		int mNTotalSteps = 0;
		int mPathStep = 0;
		int mSubStep = 0;
		int mNPathSteps = 0;
		int mDebug = 0;
		int mDebug1 = 0;
		int mDebug2 = 12;
		int mSimSpeed = 1;

		bool mIsInStock = false;
		bool mIsRotate = true;
		bool mSimPlaying = false;
		bool mSingleStep = false;
	};
}
#endif