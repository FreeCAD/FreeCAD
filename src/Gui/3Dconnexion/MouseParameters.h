
/*
Development tools and related technology provided under license from 3Dconnexion.
(c) 1992 - 2012 3Dconnexion. All rights reserved
*/


#pragma once

#include "I3dMouseParams.h"


class MouseParameters: public I3dMouseParam
{
public:
    MouseParameters();
    ~MouseParameters();

    // I3dmouseSensor interface
    bool IsPanZoom() const;
    bool IsRotate() const;
    ESpeed GetSpeed() const;

    void SetPanZoom(bool isPanZoom);
    void SetRotate(bool isRotate);
    void SetSpeed(ESpeed speed);

    // I3dmouseNavigation interface
    ENavigation GetNavigationMode() const;
    EPivot GetPivotMode() const;
    EPivotVisibility GetPivotVisibility() const;
    bool IsLockHorizon() const;

    void SetLockHorizon(bool bOn);
    void SetNavigationMode(ENavigation navigation);
    void SetPivotMode(EPivot pivot);
    void SetPivotVisibility(EPivotVisibility visibility);

private:
    MouseParameters(const MouseParameters&);
    const MouseParameters& operator=(const MouseParameters&);

    ENavigation fNavigation;
    EPivot fPivot;
    EPivotVisibility fPivotVisibility;
    bool fIsLockHorizon;

    bool fIsPanZoom;
    bool fIsRotate;
    ESpeed fSpeed;
};
