#include "MillSimulation.h"
#include "OpenGlWrapper.h"
#include <vector>
#include <iostream>

namespace MillSim {

    MillSimulation::MillSimulation()
    {
        mCurMotion = { eNop, -1, 0, 0,  0, 0, 0, 0 };
    }

    void MillSimulation::ClearMillPathSegments() {
        for (std::vector<MillPathSegment*>::const_iterator i = MillPathSegments.begin(); i != MillPathSegments.end(); ++i) {
            MillSim::MillPathSegment* p = *i;
            delete p;
        }
        MillPathSegments.clear();
        guiDisplay.SetMillSimulator(this);
    }

    void MillSimulation::SimNext()
    {
        static int simDecim = 0;

        simDecim++;
        if (simDecim < 1)
            return;

        simDecim = 0;

        if (mCurStep < mNTotalSteps)
        {
            mCurStep += mSimSpeed;
            CalcSegmentPositions();
        }
    }

    void MillSimulation::InitSimulation()
    {
        ClearMillPathSegments();

        mDestMotion = mZeroPos;
        //gDestPos = curMillOperation->startPos;
        mCurStep = 0;
        mPathStep = -1;
        mNTotalSteps = 0;
        int nOperations = (int)mCodeParser.Operations.size();;
        for (int i = 0; i < nOperations; i++)
        {
            mCurMotion = mDestMotion;
            mDestMotion = mCodeParser.Operations[i];
            EndMill* tool = GetTool(mDestMotion.tool);
            if (tool != nullptr)
            {
                MillSim::MillPathSegment* segment = new MillSim::MillPathSegment(tool, &mCurMotion, &mDestMotion);
                segment->indexInArray = i;
                mNTotalSteps += segment->numSimSteps;
                MillPathSegments.push_back(segment);
            }
        }
        mNPathSteps = (int)MillPathSegments.size();
    }

    EndMill* MillSimulation::GetTool(int toolId)
    {
        for (int i = 0; i < mToolTable.size(); i++)
        {
            if (mToolTable[i]->mToolId == toolId)
            {
                return mToolTable[i];
            }
        }
        return nullptr;
    }

    void MillSimulation::AddTool(EndMill* tool)
    {
        mToolTable.push_back(tool);
    }

    void MillSimulation::GlsimStart()
    {
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    }

    void MillSimulation::GlsimToolStep1(void)
    {
        glCullFace(GL_BACK);
        glDepthFunc(GL_LESS);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_ZERO, GL_ZERO, GL_REPLACE);
        glDepthMask(GL_FALSE);
    }


    void MillSimulation::GlsimToolStep2(void)
    {
        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glDepthFunc(GL_GREATER);
        glCullFace(GL_FRONT);
        glDepthMask(GL_TRUE);
    }

    void MillSimulation::GlsimClipBack(void)
    {
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_ZERO);
        glDepthFunc(GL_LESS);
        glCullFace(GL_FRONT);
        glDepthMask(GL_FALSE);
    }


    void MillSimulation::GlsimRenderStock(void)
    {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glDepthFunc(GL_EQUAL);
        glCullFace(GL_BACK);
    }

    void MillSimulation::GlsimRenderTools(void)
    {
        glCullFace(GL_FRONT);
    }

    void MillSimulation::GlsimEnd(void)
    {
        glCullFace(GL_BACK);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glDisable(GL_STENCIL_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }

    void MillSimulation::renderSegmentForward(int iSeg)
    {
        MillSim::MillPathSegment* p = MillPathSegments.at(iSeg);
        int step = iSeg == mPathStep ? mSubStep : p->numSimSteps;
        int start = p->isMultyPart ? 1 : step;
        for (int i = start; i <= step; i++)
        {
            GlsimToolStep1();
            p->render(i);
            GlsimToolStep2();
            p->render(i);
        }
    }

    void MillSimulation::renderSegmentReversed(int iSeg)
    {
        MillSim::MillPathSegment* p = MillPathSegments.at(iSeg);
        int step = iSeg == mPathStep ? mSubStep : p->numSimSteps;
        int end = p->isMultyPart ? 1 : step;
        for (int i = step; i >= end; i--)
        {
            GlsimToolStep1();
            p->render(i);
            GlsimToolStep2();
            p->render(i);
        }
    }

    void MillSimulation::CalcSegmentPositions()
    {
        mSubStep = mCurStep;
        for (mPathStep = 0; mPathStep < mNPathSteps; mPathStep++)
        {
            MillSim::MillPathSegment* p = MillPathSegments[mPathStep];
            if (mSubStep < p->numSimSteps)
                break;
            mSubStep -= p->numSimSteps;
        }
        if (mPathStep >= mNPathSteps)
        {
            mPathStep = mNPathSteps - 1;
            mSubStep = MillPathSegments[mPathStep]->numSimSteps;
        }
        else
            mSubStep++;
    }

    void MillSimulation::Render()
    {
        mat4x4 matLookAt, model;
        mat4x4_identity(model);
        mat4x4_look_at(matLookAt, eye, target, upvec);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        mat4x4_rotate_X(matLookAt, matLookAt, mEyeInclination);
        mat4x4_rotate_Z(matLookAt, matLookAt, mEyeRoration);
        mat4x4_translate_in_place(matLookAt, -mStockObject.mCenter[0], -mStockObject.mCenter[1], -mStockObject.mCenter[2]);

        shaderFlat.Activate();
        shaderFlat.UpdateViewMat(matLookAt);

        GlsimStart();
        mStockObject.render();

        GlsimToolStep2();

        for (int i = 0; i <= mPathStep; i++)
            renderSegmentForward(i);

        for (int i = mPathStep; i >= 0; i--)
            renderSegmentForward(i);

        //for (int i = 0; i < mPathStep; i++)
        //    renderSegmentReversed(i);

        for (int i = mPathStep; i >= 0; i--)
            renderSegmentReversed(i);

        GlsimClipBack();
        mStockObject.render();

        // start coloring
        shader3D.Activate();
        shader3D.UpdateViewMat(matLookAt);
        shader3D.UpdateObjColor(stockColor);
        GlsimRenderStock();
        mStockObject.render();
        GlsimRenderTools();

        // render cuts (back faces of tools)

        shaderInv3D.Activate();
        shaderInv3D.UpdateViewMat(matLookAt);
        shaderInv3D.UpdateObjColor(cutColor);
        for (int i = 0; i <= mPathStep; i++)
        {
            MillSim::MillPathSegment* p = MillPathSegments.at(i);
            int step = (i == mPathStep) ? mSubStep : p->numSimSteps;
            int start = p->isMultyPart ? 1 : step;
            for (int j = start; j <= step; j++)
                MillPathSegments.at(i)->render(j);
        }

        GlsimEnd();

        glEnable(GL_CULL_FACE);

        if (mPathStep >= 0)
        {
            vec3 toolPos;
            MotionPosToVec(toolPos, &mDestMotion);
            MillSim::MillPathSegment* p = MillPathSegments.at(mPathStep);
            p->GetHeadPosition(toolPos);
            mat4x4 tmat;
            mat4x4_translate(tmat, toolPos[0], toolPos[1], toolPos[2]);
            //mat4x4_translate(tmat, toolPos.x, toolPos.y, toolPos.z);
            shader3D.Activate();
            shader3D.UpdateObjColor(toolColor);
            p->mEndmill->mToolShape.Render(tmat, identityMat);
        }

        shaderFlat.Activate();
        shaderFlat.UpdateObjColor(lightColor);
        mlightObject.render();

        if (mDebug > 0)
        {
            mat4x4 test;
            mat4x4_dup(test, model);
            mat4x4_translate_in_place(test, 20, 20, 3);
            mat4x4_rotate_Z(test, test, 30.f * 3.14f / 180.f);
            int dpos = mNPathSteps - mDebug2;
            MillSim::MillPathSegment* p = MillPathSegments.at(dpos);
            if (mDebug > p->numSimSteps)
                mDebug = 1;
            p->render(mDebug);
        }
        float progress = (float)mCurStep / mNTotalSteps;
        guiDisplay.Render(progress);
    }

    void MillSimulation::ProcessSim(unsigned int time_ms) {

        static int ancient = 0;
        static int last = 0;
        static int msec = 0;
        static int fps = 0;
        static int renderTime = 0;

        last = msec;
        msec = time_ms;
        if (mIsRotate) {
            mEyeRoration += (msec - last) / 4600.0f;
            while (mEyeRoration >= PI2)
                mEyeRoration -= PI2;
        }

        if (last / 1000 != msec / 1000) {
            float calcFps = 1000.0f * fps / (msec - ancient);
            mFpsStream.str("");
            mFpsStream << "fps: " << calcFps << "    rendertime:" << renderTime << "    zpos:" << mDestMotion.z << std::ends;
            ancient = msec;
            fps = 0;
        }

        if (mSimPlaying || mSingleStep)
        {
            SimNext();
            mSingleStep = false;
        }

        Render();

        ++fps;
    }

    void MillSimulation::HandleKeyPress(int key)
    {
        switch (key) {
        case ' ':
            mIsRotate = !mIsRotate;
            break;

        case 'S':
            mSimPlaying = true;
            break;

        case 'P':
            mSimPlaying = false;
            break;

        case 'T':
            mSimPlaying = false;
            mSingleStep = true;
            break;

        case'D':
            mDebug++;
            break;

        case'K':
            mDebug2++;
            gDebug = mNPathSteps - mDebug2;
            break;

        case 'F':
            if (mSimSpeed == 1) mSimSpeed = 10;
            else if (mSimSpeed == 10) mSimSpeed = 40;
            else mSimSpeed = 1;
            break;

        default:
            if (key >= '1' && key <= '9')
                mSimSpeed = key - '0';
            break;
        }
        guiDisplay.UpdatePlayState(mSimPlaying);
    }

    void MillSimulation::TiltEye(float tiltStep)
    {
        mEyeInclination += tiltStep;
        if (mEyeInclination > PI / 2)
            mEyeInclination = PI / 2;
        else if (mEyeInclination < -PI / 2)
            mEyeInclination = -PI / 2;
    }

    void MillSimulation::RotateEye(float rotStep)
    {
        mEyeRoration += rotStep;
        if (mEyeRoration > PI2)
            mEyeRoration = PI2;
        else if (mEyeRoration < 0)
            mEyeRoration = 0;
    }

    void MillSimulation::InitDisplay()
    {
        // gray background
        glClearColor(0.6f, 0.8f, 1.0f, 1.0f);

        // Setup projection
        mat4x4 projmat;
        mat4x4_perspective(projmat, 0.7f, 4.0f / 3.0f, 1.0f, 200.0f);

        // use shaders
        //   standard diffuse shader
        shader3D.CompileShader((char*)VertShader3DNorm, (char*)FragShaderNorm);
        shader3D.UpdateEnvColor(lightPos, lightColor, ambientCol);
        shader3D.UpdateProjectionMat(projmat);

        //   invarted normal diffuse shader for inner mesh
        shaderInv3D.CompileShader((char*)VertShader3DInvNorm, (char*)FragShaderNorm);
        shaderInv3D.UpdateEnvColor(lightPos, lightColor, ambientCol);
        shaderInv3D.UpdateProjectionMat(projmat);

        //   null shader to calculate meshes only (simulation stage)
        shaderFlat.CompileShader((char*)VertShader3DNorm, (char*)FragShaderFlat);
        shaderFlat.UpdateProjectionMat(projmat);

        glMatrixMode(GL_MODELVIEW);

        // setup light object and generate tools
        mlightObject.GenerateBoxStock(-0.5f, -0.5f, -0.5f, 1, 1, 1);
        for (int i = 0; i < mToolTable.size(); i++)
            mToolTable[i]->GenerateDisplayLists();

        // init gui elements
        guiDisplay.InutGui();

    }

    void MillSimulation::SetBoxStock(float x, float y, float z, float l, float w, float h)
    {
        mStockObject.GenerateBoxStock(x, y, z, l, w, h);
        float maxw = fmaxf(w, l);
        vec3_set(eye, 0, -2.0f * maxw, 0);
        vec3_set(lightPos, x, y, h + maxw / 3);
        mlightObject.SetPosition(lightPos);
    }

    void MillSimulation::MouseDrag(int buttons, int dx, int dy)
    {
        if (buttons & MS_MOUSE_MID)
        {
            TiltEye((float)dy / 100.0f);
            RotateEye((float)dx / 100.0f);
        }
        guiDisplay.MouseDrag(buttons, dx, dy);
    }

    void MillSimulation::MouseHover(int px, int py)
    {
        guiDisplay.MouseCursorPos(px, py);
    }

    void MillSimulation::MousePress(int button, bool isPressed)
    {
        guiDisplay.MousePressed(button, isPressed, mSimPlaying);
    }


    bool MillSimulation::LoadGCodeFile(const char* fileName)
    {
        if (mCodeParser.Parse(fileName))
        {
            std::cout << "GCode file loaded successfuly" << std::endl;
            return true;
        }
        return false;
    }

    bool MillSimulation::AddGcodeLine(const char* line)
    {
        return mCodeParser.AddLine(line);
    }

    void MillSimulation::SetSimulationStage(float stage)
    {
        mCurStep = (int)((float)mNTotalSteps * stage);
        CalcSegmentPositions();
    }

}