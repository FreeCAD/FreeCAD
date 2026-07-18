// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Base/Translation.h>

#include <string>
#include <string_view>
#include <vector>

namespace Base::Translation::Test
{

class ScopedTranslator
{
public:
    explicit ScopedTranslator(const Base::Translation::Translator* translator)
        : previous {Base::Translation::getTranslator()}
    {
        Base::Translation::setTranslator(translator);
    }

    ~ScopedTranslator()
    {
        Base::Translation::setTranslator(previous);
    }

    ScopedTranslator(const ScopedTranslator&) = delete;
    ScopedTranslator(ScopedTranslator&&) = delete;
    ScopedTranslator& operator=(const ScopedTranslator&) = delete;
    ScopedTranslator& operator=(ScopedTranslator&&) = delete;

private:
    const Base::Translation::Translator* previous;
};

class RecordingTranslator final: public Base::Translation::Translator
{
public:
    enum class TranslateMode
    {
        Passthrough,
        Constant,
        WrapSource
    };

    TranslateMode translateMode {TranslateMode::Passthrough};
    std::string constantTranslation;
    std::string sourcePrefix;
    std::string sourceSuffix;

    bool installResult {false};
    bool removeResult {false};

    mutable int translateCalls {0};
    mutable std::string lastContext;
    mutable std::string lastSourceText;
    mutable std::string lastDisambiguation;
    mutable int lastN {-1};

    mutable int installCalls {0};
    mutable std::string lastInstalledFilename;

    mutable int removeCalls {0};
    mutable std::vector<std::string> lastRemovedFilenames;

    std::string translate(
        std::string_view context,
        std::string_view sourceText,
        std::string_view disambiguation,
        int n
    ) const override
    {
        ++translateCalls;
        lastContext = std::string(context);
        lastSourceText = std::string(sourceText);
        lastDisambiguation = std::string(disambiguation);
        lastN = n;

        switch (translateMode) {
            case TranslateMode::Passthrough:
                return std::string(sourceText);
            case TranslateMode::Constant:
                return constantTranslation;
            case TranslateMode::WrapSource:
                return sourcePrefix + std::string(sourceText) + sourceSuffix;
        }
        return std::string(sourceText);
    }

    bool installTranslator(std::string_view filename) const override
    {
        ++installCalls;
        lastInstalledFilename = std::string(filename);
        return installResult;
    }

    bool removeTranslators(const std::vector<std::string>& filenames) const override
    {
        ++removeCalls;
        lastRemovedFilenames = filenames;
        return removeResult;
    }
};

}  // namespace Base::Translation::Test
