// SPDX-License-Identifier: LGPL-2.1-or-later

#include "TranslationQtBridge.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>
#include <QTranslator>

#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include <Base/ServiceProvider.h>
#include <Base/Translation.h>

namespace
{

struct InstalledTranslator
{
    std::unique_ptr<QTranslator> translator;
};

std::mutex translatorsMutex;
std::unordered_map<std::string, std::vector<InstalledTranslator>> translatorsByFile;

bool installTranslatorImpl(const std::string& filename);
bool removeTranslatorsImpl(const std::vector<std::string>& filenames);

class QtTranslator final : public Base::Translation::Translator
{
public:
    std::string translate(
        std::string_view context,
        std::string_view sourceText,
        std::string_view disambiguation,
        int n
    ) const override
    {
        const std::string contextString(context);
        const std::string sourceTextString(sourceText);
        const std::string disambiguationString(disambiguation);

        const QString translated = QCoreApplication::translate(
            contextString.c_str(),
            sourceTextString.c_str(),
            disambiguation.empty() ? nullptr : disambiguationString.c_str(),
            n
        );

        const QByteArray translatedUtf8 = translated.toUtf8();
        return std::string(translatedUtf8.constData(), translatedUtf8.size());
    }

    bool installTranslator(std::string_view filename) const override
    {
        const std::string file(filename);
        QCoreApplication* app = QCoreApplication::instance();
        if (!app || QThread::currentThread() == app->thread()) {
            return installTranslatorImpl(file);
        }

        bool ok = false;
        QMetaObject::invokeMethod(
            app,
            [&ok, file]() { ok = installTranslatorImpl(file); },
            Qt::BlockingQueuedConnection
        );
        return ok;
    }

    bool removeTranslators(const std::vector<std::string>& filenames) const override
    {
        QCoreApplication* app = QCoreApplication::instance();
        if (!app || QThread::currentThread() == app->thread()) {
            return removeTranslatorsImpl(filenames);
        }

        bool ok = false;
        QMetaObject::invokeMethod(
            app,
            [&ok, filenames]() { ok = removeTranslatorsImpl(filenames); },
            Qt::BlockingQueuedConnection
        );
        return ok;
    }
};

bool installTranslatorImpl(const std::string& filename)
{
    QCoreApplication* app = QCoreApplication::instance();
    if (!app) {
        return false;
    }

    auto translator = std::make_unique<QTranslator>(nullptr);
    if (!translator->load(QString::fromUtf8(filename.c_str()))) {
        return false;
    }

    if (!app->installTranslator(translator.get())) {
        return false;
    }

    std::lock_guard<std::mutex> lock(translatorsMutex);
    translatorsByFile[filename].push_back({std::move(translator)});
    return true;
}

bool removeTranslatorsImpl(const std::vector<std::string>& filenames)
{
    QCoreApplication* app = QCoreApplication::instance();
    if (!app) {
        return false;
    }

    std::vector<std::unique_ptr<QTranslator>> toRemove;
    {
        std::lock_guard<std::mutex> lock(translatorsMutex);
        for (const auto& filename : filenames) {
            auto it = translatorsByFile.find(filename);
            if (it == translatorsByFile.end()) {
                continue;
            }
            for (auto& entry : it->second) {
                toRemove.push_back(std::move(entry.translator));
            }
            translatorsByFile.erase(it);
        }
    }

    bool ok = true;
    for (const auto& t : toRemove) {
        ok &= app->removeTranslator(t.get());
    }
    return ok;
}

}  // namespace

namespace App
{

void installTranslationQtBridge()
{
    static bool installed = false;
    if (installed) {
        return;
    }
    installed = true;

    static QtTranslator qtTranslator;
    Base::registerServiceImplementation<Base::Translation::Translator>(&qtTranslator);
    Base::Translation::setTranslator(Base::provideService<Base::Translation::Translator>());
}

}  // namespace App
