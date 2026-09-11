// SPDX-License-Identifier: LGPL-2.1-or-later

#include "TemplateTranslationService.h"

#include <QMap>
#include <QTranslator>

#include <Base/ServiceProvider.h>
#include <Mod/TechDraw/App/TemplateTranslator.h>

namespace
{
class TemplateTranslationService final: public TechDraw::TemplateTranslationService
{
public:
    QString translate(const QString& key, const QString& languageName) const override
    {
        static const QMap<QString, QString> locales {
            {QStringLiteral("English"), QStringLiteral("en")},
            {QStringLiteral("Afrikaans"), QStringLiteral("af")},
            {QStringLiteral("Català"), QStringLiteral("ca")},
            {QStringLiteral("Dansk"), QStringLiteral("da")},
            {QStringLiteral("Deutsch"), QStringLiteral("de")},
            {QStringLiteral("Español"), QStringLiteral("es-ES")},
            {QStringLiteral("Euskara"), QStringLiteral("eu")},
            {QStringLiteral("Filipino"), QStringLiteral("fil")},
            {QStringLiteral("Français"), QStringLiteral("fr")},
            {QStringLiteral("Galego"), QStringLiteral("gl")},
            {QStringLiteral("Hrvatski"), QStringLiteral("hr")},
            {QStringLiteral("Indonesia"), QStringLiteral("id")},
            {QStringLiteral("Italiano"), QStringLiteral("it")},
            {QStringLiteral("Lietuvių"), QStringLiteral("lt")},
            {QStringLiteral("Magyar"), QStringLiteral("hu")},
            {QStringLiteral("Nederlands"), QStringLiteral("nl")},
            {QStringLiteral("Norsk bokmål"), QStringLiteral("no")},
            {QStringLiteral("Polski"), QStringLiteral("pl")},
            {QStringLiteral("Português"), QStringLiteral("pt-PT")},
            {QStringLiteral("Română"), QStringLiteral("ro")},
            {QStringLiteral("Slovenčina"), QStringLiteral("sk")},
            {QStringLiteral("Slovenščina"), QStringLiteral("sl")},
            {QStringLiteral("Srpski"), QStringLiteral("sr-CS")},
            {QStringLiteral("Suomi"), QStringLiteral("fi")},
            {QStringLiteral("Svenska"), QStringLiteral("sv")},
            {QStringLiteral("Taqbaylit"), QStringLiteral("kab")},
            {QStringLiteral("Tiếng Việt"), QStringLiteral("vi")},
            {QStringLiteral("Türkçe"), QStringLiteral("tr")},
            {QStringLiteral("Valencian"), QStringLiteral("val-ES")},
            {QStringLiteral("Čeština"), QStringLiteral("cs")},
            {QStringLiteral("Ελληνικά"), QStringLiteral("el")},
            {QStringLiteral("Беларуская"), QStringLiteral("be")},
            {QStringLiteral("Български"), QStringLiteral("bg")},
            {QStringLiteral("Русский"), QStringLiteral("ru")},
            {QStringLiteral("Српски"), QStringLiteral("sr")},
            {QStringLiteral("Українська"), QStringLiteral("uk")},
            {QStringLiteral("العربية"), QStringLiteral("ar")},
            {QStringLiteral("ქართული"), QStringLiteral("ka")},
            {QStringLiteral("日本語"), QStringLiteral("ja")},
            {QStringLiteral("简体中文"), QStringLiteral("zh-CN")},
            {QStringLiteral("繁體中文"), QStringLiteral("zh-TW")},
            {QStringLiteral("한국어"), QStringLiteral("ko")},
        };
        const auto locale = locales.constFind(languageName);
        if (locale == locales.cend() || *locale == QLatin1String("en")) {
            return key;
        }

        // Do not install this translator: the drawing language is independent of the UI language.
        QTranslator translator;
        if (!translator.load(QStringLiteral(":/translations/TechDraw_%1.qm").arg(*locale))) {
            return key;
        }
        const QString translated =
            translator.translate("TechDraw::TemplateTranslator", key.toUtf8().constData());
        return translated.isEmpty() ? key : translated;
    }
};
}  // namespace

void TechDrawGui::registerTemplateTranslationService()
{
    static TemplateTranslationService service;
    Base::registerServiceImplementation<TechDraw::TemplateTranslationService>(&service);
}
