// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <App/Application.h>

#include "ProgressIndicator.h"


using namespace Part;

ScopedRecomputeProgress::ScopedRecomputeProgress() = default;

ScopedRecomputeProgress::ScopedRecomputeProgress(const char* text, ProgressFallback fallback)
{
    App::RecomputeProgressHandle* progress = App::currentRecomputeProgress();
    if (!progress) {
        if (fallback != ProgressFallback::createHandle) {
            return;
        }

        ownedProgress = std::make_unique<App::RecomputeProgressHandle>();
        progress = ownedProgress.get();
    }

    scope = std::make_unique<App::RecomputeProgressScope>(progress->makeScope(text));
}

ScopedRecomputeProgress::ScopedRecomputeProgress(
    std::unique_ptr<App::RecomputeProgressHandle> ownedProgress,
    std::unique_ptr<App::RecomputeProgressScope> scope
)
    : ownedProgress(std::move(ownedProgress))
    , scope(std::move(scope))
{}

ScopedRecomputeProgress::ScopedRecomputeProgress(ScopedRecomputeProgress&& other) noexcept = default;

ScopedRecomputeProgress& ScopedRecomputeProgress::operator=(
    ScopedRecomputeProgress&& other
) noexcept = default;

ScopedRecomputeProgress::~ScopedRecomputeProgress() = default;

ScopedRecomputeProgress::operator bool() const
{
    return static_cast<bool>(scope);
}

ScopedRecomputeProgress ScopedRecomputeProgress::makeScope(const char* text)
{
    if (!scope) {
        return {};
    }

    return ScopedRecomputeProgress(
        nullptr,
        std::make_unique<App::RecomputeProgressScope>(scope->makeScope(text))
    );
}

ScopedRecomputeProgress ScopedRecomputeProgress::makeStepScope(
    std::size_t stepIndex,
    std::size_t totalSteps,
    const char* text
)
{
    if (!scope) {
        return {};
    }

    return ScopedRecomputeProgress(
        nullptr,
        std::make_unique<App::RecomputeProgressScope>(scope->makeStepScope(stepIndex, totalSteps, text))
    );
}

void ScopedRecomputeProgress::setText(const char* text)
{
    if (scope) {
        scope->setText(text);
    }
}

void ScopedRecomputeProgress::setProgress(std::size_t progress)
{
    if (scope) {
        scope->setProgress(progress);
    }
}

void ScopedRecomputeProgress::complete()
{
    setProgress(100);
}

bool ScopedRecomputeProgress::wasCanceled() const
{
    return scope ? scope->wasCanceled() : App::currentRecomputeWasCanceled();
}

void ScopedRecomputeProgress::throwIfCanceled() const
{
    App::throwIfRecomputeCanceled();
}

ProgressIndicator::ProgressIndicator()
    : scope("Processing...", ProgressFallback::createHandle)
{}

ProgressIndicator::~ProgressIndicator() = default;
void ProgressIndicator::Show(const Message_ProgressScope& theScope, const Standard_Boolean isForce)
{
    (void)isForce;
    const char* name = theScope.Name();
    scope.setText(name ? name : "Processing...");
    std::size_t current = static_cast<std::size_t>(100. * theScope.Value() / theScope.MaxValue());
    if (current != currentStep) {
        currentStep = current;
        scope.setProgress(currentStep);
    }
}

Standard_Boolean ProgressIndicator::UserBreak()
{
    return scope.wasCanceled();
}

void ProgressIndicator::Reset()
{
    currentStep = 0;
    scope.complete();
}
