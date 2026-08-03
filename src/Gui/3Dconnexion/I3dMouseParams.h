
/*
Development tools and related technology provided under license from 3Dconnexion.
(c) 1992 - 2012 3Dconnexion. All rights reserved
*/

#pragma once

/*
   Parameters for the 3D mouse based on the SDK from 3Dconnexion
*/

class I3dMouseSensor
{
public:
    enum ESpeed
    {
        kLowSpeed = 0,
        kMidSpeed,
        kHighSpeed
    };

    virtual bool IsPanZoom() const = 0;
    virtual bool IsRotate() const = 0;
    virtual ESpeed GetSpeed() const = 0;

    virtual void SetPanZoom(bool isPanZoom) = 0;
    virtual void SetRotate(bool isRotate) = 0;
    virtual void SetSpeed(ESpeed speed) = 0;

protected:
    virtual ~I3dMouseSensor()
    {}
};


class I3dMouseNavigation
{
public:
    enum EPivot
    {
        kManualPivot = 0,
        kAutoPivot,
        kAutoPivotOverride
    };

    enum ENavigation
    {
        kObjectMode = 0,
        kCameraMode,
        kFlyMode,
        kWalkMode,
        kHelicopterMode
    };

    enum EPivotVisibility
    {
        kHidePivot = 0,
        kShowPivot,
        kShowMovingPivot
    };


    virtual ENavigation GetNavigationMode() const = 0;
    virtual EPivot GetPivotMode() const = 0;
    virtual EPivotVisibility GetPivotVisibility() const = 0;
    virtual bool IsLockHorizon() const = 0;

    virtual void SetLockHorizon(bool bOn) = 0;
    virtual void SetNavigationMode(ENavigation navigation) = 0;
    virtual void SetPivotMode(EPivot pivot) = 0;
    virtual void SetPivotVisibility(EPivotVisibility visibility) = 0;

protected:
    virtual ~I3dMouseNavigation()
    {}
};


class I3dMouseParam: public I3dMouseSensor, public I3dMouseNavigation
{
public:
    virtual ~I3dMouseParam()
    {}
};
