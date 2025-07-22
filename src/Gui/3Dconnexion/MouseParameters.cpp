
/*
Development tools and related technology provided under license from 3Dconnexion.
(c) 1992 - 2012 3Dconnexion. All rights reserved
*/


#include "PreCompiled.h"
#include "MouseParameters.h"

MouseParameters::MouseParameters(): fNavigation(kObjectMode)
	  , fPivot(kAutoPivot)
	  , fPivotVisibility(kShowPivot)
	  , fIsLockHorizon(true)
	  , fIsPanZoom(true)
	  , fIsRotate(true)
	  , fSpeed(kLowSpeed)
{
}

MouseParameters::~MouseParameters()
{
}

bool MouseParameters::IsPanZoom()  const
{
	return fIsPanZoom;
}

bool MouseParameters::IsRotate()  const
{
	return fIsRotate;
}

MouseParameters::ESpeed MouseParameters::GetSpeed()  const
{
	return fSpeed;
}

void MouseParameters::SetPanZoom(bool isPanZoom)
{
	fIsPanZoom=isPanZoom;
}

void MouseParameters::SetRotate(bool isRotate)
{
	fIsRotate=isRotate;
}

void MouseParameters::SetSpeed(ESpeed speed)
{
	fSpeed=speed;
}


MouseParameters::ENavigation MouseParameters::GetNavigationMode()  const
{
	return fNavigation;
}

MouseParameters::EPivot MouseParameters::GetPivotMode()  const
{
	return fPivot;
}

MouseParameters::EPivotVisibility MouseParameters::GetPivotVisibility()  const
{
	return fPivotVisibility;
}

bool MouseParameters::IsLockHorizon()  const
{
	return fIsLockHorizon;
}

void MouseParameters::SetLockHorizon(bool bOn)
{
	fIsLockHorizon=bOn;
}

void MouseParameters::SetNavigationMode(ENavigation navigation)
{
	fNavigation=navigation;
}

void MouseParameters::SetPivotMode(EPivot pivot)
{
   if (fPivot!=kManualPivot || pivot!=kAutoPivotOverride)
	  fPivot = pivot;
}

void MouseParameters::SetPivotVisibility(EPivotVisibility visibility)
{
	fPivotVisibility = visibility;
}
