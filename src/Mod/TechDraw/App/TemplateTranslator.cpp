/* SPDX - License - Identifier: LGPL - 2.1 - or -later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pierre-Louis Boyer                                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"

#include "TemplateTranslator.h"
#include <algorithm> // For std::sort
#include <QSet>
#include <Base/Console.h>

namespace TechDraw
{   
TechDrawExport const char* LanguageEnums[] = {
    "English",
    "Afrikaans",
    "Català",
    "Dansk",
    "Deutsch",
    "Español",
    "Euskara",
    "Filipino",
    "Français",
    "Galego",
    "Hrvatski",
    "Indonesia",
    "Italiano",
    "Lietuvių",
    "Magyar",
    "Nederlands",
    "Norsk bokmål",
    "Polski",
    "Português",
    "Română",
    "Slovenčina",
    "Slovenščina",
    "Srpski",
    "Suomi",
    "Svenska",
    "Taqbaylit",
    "Tiếng Việt",
    "Türkçe",
    "Valencian",
    "Čeština",
    "Ελληνικά",    // Greek
    "Беларуская",  // Belarusian
    "Български",   // Bulgarian
    "Русский",     // Russian
    "Српски",      // Serbian
    "Українська",  // Ukrainian
    "العربية",     // Arabic
    "ქართული",     // Georgian
    "日本語",      // Japanese
    "简体中文",    // Chinese Simplified
    "繁體中文",    // Chinese Traditional
    "한국어",      // Korean
    nullptr};

TemplateTranslator::TemplateTranslator()
{
    initializeTranslations();
}

void TemplateTranslator::initializeTranslations()
{
    // --- Placeholder Keys (matching SVGs) ---
    const QString KEY_SCALE = QStringLiteral("Scale:");
    const QString KEY_SHEET = QStringLiteral("Sheet:");
    const QString KEY_TITLE = QStringLiteral("Title, supplementary title:");
    const QString KEY_DATE = QStringLiteral("Issue date:");
    const QString KEY_CREATED_BY = QStringLiteral("Created by:");
    const QString KEY_APPROVED_BY = QStringLiteral("Approved by:");
    const QString KEY_DRAWING_NUMBER = QStringLiteral("Drawing number:");
    const QString KEY_MATERIAL = QStringLiteral("Part Material:");
    const QString KEY_REVISION = QStringLiteral("Revision:");
    const QString KEY_GENERAL_TOLERANCES = QStringLiteral("General tolerances:");
    const QString KEY_OWNER = QStringLiteral("Owner:");
    const QString KEY_DOC_TYPE = QStringLiteral("Document type:");
    const QString KEY_DEPARTEMENT = QStringLiteral("Responsible department:");
    const QString KEY_STATUS = QStringLiteral("Document status:");
    const QString KEY_LANGUAGE = QStringLiteral("Language:");

    
    // --- English (en) ---
    const QString LANG_NAME_ENGLISH = QStringLiteral("English");
    m_translations[KEY_SCALE][LANG_NAME_ENGLISH] = KEY_SCALE;
    m_translations[KEY_SHEET][LANG_NAME_ENGLISH] = KEY_SHEET;
    m_translations[KEY_TITLE][LANG_NAME_ENGLISH] = KEY_TITLE;
    m_translations[KEY_DATE][LANG_NAME_ENGLISH] = KEY_DATE;
    m_translations[KEY_CREATED_BY][LANG_NAME_ENGLISH] = KEY_CREATED_BY;
    m_translations[KEY_APPROVED_BY][LANG_NAME_ENGLISH] = KEY_APPROVED_BY;
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_ENGLISH] = KEY_DRAWING_NUMBER;
    m_translations[KEY_MATERIAL][LANG_NAME_ENGLISH] = KEY_MATERIAL;
    m_translations[KEY_REVISION][LANG_NAME_ENGLISH] = KEY_REVISION;
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_ENGLISH] = KEY_GENERAL_TOLERANCES;
    m_translations[KEY_OWNER][LANG_NAME_ENGLISH] = KEY_OWNER;
    m_translations[KEY_DOC_TYPE][LANG_NAME_ENGLISH] = KEY_DOC_TYPE;
    m_translations[KEY_DEPARTEMENT][LANG_NAME_ENGLISH] = KEY_DEPARTEMENT;
    m_translations[KEY_STATUS][LANG_NAME_ENGLISH] = KEY_STATUS;
    m_translations[KEY_LANGUAGE][LANG_NAME_ENGLISH] = KEY_LANGUAGE;

        
    // --- Afrikaans ---
    const QString LANG_NAME_AFRIKAANS = QStringLiteral("Afrikaans");
    m_translations[KEY_SCALE][LANG_NAME_AFRIKAANS] = QStringLiteral("Skaal:");
    m_translations[KEY_SHEET][LANG_NAME_AFRIKAANS] = QStringLiteral("Blad:");
    m_translations[KEY_TITLE][LANG_NAME_AFRIKAANS] = QStringLiteral("Titels, aanvullende titel:");
    m_translations[KEY_DATE][LANG_NAME_AFRIKAANS] = QStringLiteral("Uitgawedatum:");
    m_translations[KEY_CREATED_BY][LANG_NAME_AFRIKAANS] = QStringLiteral("Geskep deur:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_AFRIKAANS] = QStringLiteral("Goedgekeur deur:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_AFRIKAANS] = QStringLiteral("Tekeningnommer:");
    m_translations[KEY_MATERIAL][LANG_NAME_AFRIKAANS] = QStringLiteral("Onderdeelmateriaal:");
    m_translations[KEY_REVISION][LANG_NAME_AFRIKAANS] = QStringLiteral("Hersiening:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_AFRIKAANS] = QStringLiteral("Algemene toleransies:");
    m_translations[KEY_OWNER][LANG_NAME_AFRIKAANS] = QStringLiteral("Eienaar:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_AFRIKAANS] = QStringLiteral("Dokumenttipe:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_AFRIKAANS] = QStringLiteral("Departement:");
    m_translations[KEY_STATUS][LANG_NAME_AFRIKAANS] = QStringLiteral("Dokumentstatus:");
    m_translations[KEY_LANGUAGE][LANG_NAME_AFRIKAANS] = QStringLiteral("Taal:");

    // --- Català ---
    const QString LANG_NAME_CATALA = QStringLiteral("Català");
    m_translations[KEY_SCALE][LANG_NAME_CATALA] = QStringLiteral("Escala:");
    m_translations[KEY_SHEET][LANG_NAME_CATALA] = QStringLiteral("Full:");
    m_translations[KEY_TITLE][LANG_NAME_CATALA] = QStringLiteral("Títols, títol suplementari:");
    m_translations[KEY_DATE][LANG_NAME_CATALA] = QStringLiteral("Data d'emissió:");
    m_translations[KEY_CREATED_BY][LANG_NAME_CATALA] = QStringLiteral("Creat per:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_CATALA] = QStringLiteral("Aprovat per:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_CATALA] = QStringLiteral("Número de dibuix:");
    m_translations[KEY_MATERIAL][LANG_NAME_CATALA] = QStringLiteral("Material de la peça:");
    m_translations[KEY_REVISION][LANG_NAME_CATALA] = QStringLiteral("Revisió:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_CATALA] = QStringLiteral("Toleràncies generals:");
    m_translations[KEY_OWNER][LANG_NAME_CATALA] = QStringLiteral("Propietari:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_CATALA] = QStringLiteral("Tipus de document:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_CATALA] = QStringLiteral("Departament:");
    m_translations[KEY_STATUS][LANG_NAME_CATALA] = QStringLiteral("Estat del document:");
    m_translations[KEY_LANGUAGE][LANG_NAME_CATALA] = QStringLiteral("Idioma:");

    // --- Dansk ---
    const QString LANG_NAME_DANSK = QStringLiteral("Dansk");
    m_translations[KEY_SCALE][LANG_NAME_DANSK] = QStringLiteral("Skala:");
    m_translations[KEY_SHEET][LANG_NAME_DANSK] = QStringLiteral("Ark:");
    m_translations[KEY_TITLE][LANG_NAME_DANSK] = QStringLiteral("Titler, supplerende titel:");
    m_translations[KEY_DATE][LANG_NAME_DANSK] = QStringLiteral("Udgivelsesdato:");
    m_translations[KEY_CREATED_BY][LANG_NAME_DANSK] = QStringLiteral("Oprettet af:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_DANSK] = QStringLiteral("Godkendt af:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_DANSK] = QStringLiteral("Tegningsnummer:");
    m_translations[KEY_MATERIAL][LANG_NAME_DANSK] = QStringLiteral("Materiale:");
    m_translations[KEY_REVISION][LANG_NAME_DANSK] = QStringLiteral("Revision:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_DANSK] = QStringLiteral("Generelle tolerancer:");
    m_translations[KEY_OWNER][LANG_NAME_DANSK] = QStringLiteral("Ejer:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_DANSK] = QStringLiteral("Dokumenttype:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_DANSK] = QStringLiteral("Afdeling:");
    m_translations[KEY_STATUS][LANG_NAME_DANSK] = QStringLiteral("Dokumentstatus:");
    m_translations[KEY_LANGUAGE][LANG_NAME_DANSK] = QStringLiteral("Sprog:");

    // --- Deutsch ---
    const QString LANG_NAME_DEUTSCH = QStringLiteral("Deutsch");
    m_translations[KEY_SCALE][LANG_NAME_DEUTSCH] = QStringLiteral("Maßstab:");
    m_translations[KEY_SHEET][LANG_NAME_DEUTSCH] = QStringLiteral("Blatt:");
    m_translations[KEY_TITLE][LANG_NAME_DEUTSCH] = QStringLiteral("Titel, Zusatztitel:");
    m_translations[KEY_DATE][LANG_NAME_DEUTSCH] = QStringLiteral("Ausgabedatum:");
    m_translations[KEY_CREATED_BY][LANG_NAME_DEUTSCH] = QStringLiteral("Erstellt von:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_DEUTSCH] = QStringLiteral("Genehmigt von:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_DEUTSCH] = QStringLiteral("Zeichnungsnummer:");
    m_translations[KEY_MATERIAL][LANG_NAME_DEUTSCH] = QStringLiteral("Werkstoff:");
    m_translations[KEY_REVISION][LANG_NAME_DEUTSCH] = QStringLiteral("Revision:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_DEUTSCH] = QStringLiteral("Allgemeine Toleranzen:");
    m_translations[KEY_OWNER][LANG_NAME_DEUTSCH] = QStringLiteral("Eigentümer:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_DEUTSCH] = QStringLiteral("Dokumenttyp:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_DEUTSCH] = QStringLiteral("Abteilung:");
    m_translations[KEY_STATUS][LANG_NAME_DEUTSCH] = QStringLiteral("Dokumentstatus:");
    m_translations[KEY_LANGUAGE][LANG_NAME_DEUTSCH] = QStringLiteral("Sprache:");
    
    // --- Español ---
    const QString LANG_NAME_ESPANOL = QStringLiteral("Español");
    m_translations[KEY_SCALE][LANG_NAME_ESPANOL] = QStringLiteral("Escala:");
    m_translations[KEY_SHEET][LANG_NAME_ESPANOL] = QStringLiteral("Hoja:");
    m_translations[KEY_TITLE][LANG_NAME_ESPANOL] = QStringLiteral("Títulos, título suplementario:");
    m_translations[KEY_DATE][LANG_NAME_ESPANOL] = QStringLiteral("Fecha de emisión:");
    m_translations[KEY_CREATED_BY][LANG_NAME_ESPANOL] = QStringLiteral("Creado por:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_ESPANOL] = QStringLiteral("Aprobado por:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_ESPANOL] = QStringLiteral("Número de plano:");
    m_translations[KEY_MATERIAL][LANG_NAME_ESPANOL] = QStringLiteral("Material de la pieza:");
    m_translations[KEY_REVISION][LANG_NAME_ESPANOL] = QStringLiteral("Revisión:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_ESPANOL] = QStringLiteral("Tolerancias generales:");
    m_translations[KEY_OWNER][LANG_NAME_ESPANOL] = QStringLiteral("Propietario:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_ESPANOL] = QStringLiteral("Tipo de documento:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_ESPANOL] = QStringLiteral("Departamento:");
    m_translations[KEY_STATUS][LANG_NAME_ESPANOL] = QStringLiteral("Estado del documento:");
    m_translations[KEY_LANGUAGE][LANG_NAME_ESPANOL] = QStringLiteral("Idioma:");
    
    // --- Euskara ---
    const QString LANG_NAME_EUSKARA = QStringLiteral("Euskara");
    m_translations[KEY_SCALE][LANG_NAME_EUSKARA] = QStringLiteral("Eskala:");
    m_translations[KEY_SHEET][LANG_NAME_EUSKARA] = QStringLiteral("Orria:");
    m_translations[KEY_TITLE][LANG_NAME_EUSKARA] = QStringLiteral("Tituluak, titulu osagarria:");
    m_translations[KEY_DATE][LANG_NAME_EUSKARA] = QStringLiteral("Jaulkipen-data:");
    m_translations[KEY_CREATED_BY][LANG_NAME_EUSKARA] = QStringLiteral("Egilea:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_EUSKARA] = QStringLiteral("Onartua:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_EUSKARA] = QStringLiteral("Marrazki-zenbakia:");
    m_translations[KEY_MATERIAL][LANG_NAME_EUSKARA] = QStringLiteral("Pieza-materiala:");
    m_translations[KEY_REVISION][LANG_NAME_EUSKARA] = QStringLiteral("Berrikuspena:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_EUSKARA] = QStringLiteral("Tolerantzia orokorrak:");
    m_translations[KEY_OWNER][LANG_NAME_EUSKARA] = QStringLiteral("Jabea:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_EUSKARA] = QStringLiteral("Dokumentu mota:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_EUSKARA] = QStringLiteral("Saila:");
    m_translations[KEY_STATUS][LANG_NAME_EUSKARA] = QStringLiteral("Dokumentuaren egoera:");
    m_translations[KEY_LANGUAGE][LANG_NAME_EUSKARA] = QStringLiteral("Hizkuntza:");
    
    // --- Filipino ---
    const QString LANG_NAME_FILIPINO = QStringLiteral("Filipino");
    m_translations[KEY_SCALE][LANG_NAME_FILIPINO] = QStringLiteral("Iskala:");
    m_translations[KEY_SHEET][LANG_NAME_FILIPINO] = QStringLiteral("Dahon:");
    m_translations[KEY_TITLE][LANG_NAME_FILIPINO] = QStringLiteral("Mga Pamagat, karagdagang pamagat:");
    m_translations[KEY_DATE][LANG_NAME_FILIPINO] = QStringLiteral("Petsa ng Pag-isyu:");
    m_translations[KEY_CREATED_BY][LANG_NAME_FILIPINO] = QStringLiteral("Nilikha ni:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_FILIPINO] = QStringLiteral("Inaprubahan ni:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_FILIPINO] = QStringLiteral("Numero ng Drawing:");
    m_translations[KEY_MATERIAL][LANG_NAME_FILIPINO] = QStringLiteral("Materyal ng Parte:");
    m_translations[KEY_REVISION][LANG_NAME_FILIPINO] = QStringLiteral("Rebisyon:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_FILIPINO] = QStringLiteral("Pangkalahatang Toleransiya:");
    m_translations[KEY_OWNER][LANG_NAME_FILIPINO] = QStringLiteral("May-ari:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_FILIPINO] = QStringLiteral("Uri ng Dokumento:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_FILIPINO] = QStringLiteral("Departamento:");
    m_translations[KEY_STATUS][LANG_NAME_FILIPINO] = QStringLiteral("Katayuan ng Dokumento:");
    m_translations[KEY_LANGUAGE][LANG_NAME_FILIPINO] = QStringLiteral("Wika:");

    // --- French (fr) ---
    const QString LANG_NAME_FRENCH = QStringLiteral("Français");
    m_translations[KEY_SCALE][LANG_NAME_FRENCH] = QStringLiteral("Échelle:");
    m_translations[KEY_SHEET][LANG_NAME_FRENCH] = QStringLiteral("Feuille:");
    m_translations[KEY_TITLE][LANG_NAME_FRENCH] = QStringLiteral("Titres, titre supplémentaire:");
    m_translations[KEY_DATE][LANG_NAME_FRENCH] = QStringLiteral("Date d'émission:");
    m_translations[KEY_CREATED_BY][LANG_NAME_FRENCH] = QStringLiteral("Créé par:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_FRENCH] = QStringLiteral("Approuvé par:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_FRENCH] = QStringLiteral("Numéro de plan:");
    m_translations[KEY_MATERIAL][LANG_NAME_FRENCH] = QStringLiteral("Matériau de la pièce:");
    m_translations[KEY_REVISION][LANG_NAME_FRENCH] = QStringLiteral("Révision:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_FRENCH] = QStringLiteral("Tolérances générales:");
    m_translations[KEY_OWNER][LANG_NAME_FRENCH] = QStringLiteral("Propriétaire:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_FRENCH] = QStringLiteral("Type de document:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_FRENCH] = QStringLiteral("Département:");
    m_translations[KEY_STATUS][LANG_NAME_FRENCH] = QStringLiteral("Statut du document:");
    m_translations[KEY_LANGUAGE][LANG_NAME_FRENCH] = QStringLiteral("Langue:");
    
    // --- Galego ---
    const QString LANG_NAME_GALEGO = QStringLiteral("Galego");
    m_translations[KEY_SCALE][LANG_NAME_GALEGO] = QStringLiteral("Escala:");
    m_translations[KEY_SHEET][LANG_NAME_GALEGO] = QStringLiteral("Folla:");
    m_translations[KEY_TITLE][LANG_NAME_GALEGO] = QStringLiteral("Títulos, título suplementario:");
    m_translations[KEY_DATE][LANG_NAME_GALEGO] = QStringLiteral("Data de emisión:");
    m_translations[KEY_CREATED_BY][LANG_NAME_GALEGO] = QStringLiteral("Creado por:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_GALEGO] = QStringLiteral("Aprobado por:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_GALEGO] = QStringLiteral("Número de plano:");
    m_translations[KEY_MATERIAL][LANG_NAME_GALEGO] = QStringLiteral("Material da peza:");
    m_translations[KEY_REVISION][LANG_NAME_GALEGO] = QStringLiteral("Revisión:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_GALEGO] = QStringLiteral("Tolerancias xerais:");
    m_translations[KEY_OWNER][LANG_NAME_GALEGO] = QStringLiteral("Propietario:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_GALEGO] = QStringLiteral("Tipo de documento:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_GALEGO] = QStringLiteral("Departamento:");
    m_translations[KEY_STATUS][LANG_NAME_GALEGO] = QStringLiteral("Estado do documento:");
    m_translations[KEY_LANGUAGE][LANG_NAME_GALEGO] = QStringLiteral("Idioma:");

    // --- Hrvatski ---
    const QString LANG_NAME_HRVATSKI = QStringLiteral("Hrvatski");
    m_translations[KEY_SCALE][LANG_NAME_HRVATSKI] = QStringLiteral("Mjerilo:");
    m_translations[KEY_SHEET][LANG_NAME_HRVATSKI] = QStringLiteral("List:");
    m_translations[KEY_TITLE][LANG_NAME_HRVATSKI] = QStringLiteral("Naslovi, dodatni naslov:");
    m_translations[KEY_DATE][LANG_NAME_HRVATSKI] = QStringLiteral("Datum izdavanja:");
    m_translations[KEY_CREATED_BY][LANG_NAME_HRVATSKI] = QStringLiteral("Izradio:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_HRVATSKI] = QStringLiteral("Odobrio:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_HRVATSKI] = QStringLiteral("Broj crteža:");
    m_translations[KEY_MATERIAL][LANG_NAME_HRVATSKI] = QStringLiteral("Materijal dijela:");
    m_translations[KEY_REVISION][LANG_NAME_HRVATSKI] = QStringLiteral("Revizija:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_HRVATSKI] = QStringLiteral("Opće tolerancije:");
    m_translations[KEY_OWNER][LANG_NAME_HRVATSKI] = QStringLiteral("Vlasnik:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_HRVATSKI] = QStringLiteral("Vrsta dokumenta:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_HRVATSKI] = QStringLiteral("Odjel:");
    m_translations[KEY_STATUS][LANG_NAME_HRVATSKI] = QStringLiteral("Status dokumenta:");
    m_translations[KEY_LANGUAGE][LANG_NAME_HRVATSKI] = QStringLiteral("Jezik:");

    // --- Indonesia --- (Bahasa Indonesia)
    const QString LANG_NAME_INDONESIA = QStringLiteral("Indonesia");
    m_translations[KEY_SCALE][LANG_NAME_INDONESIA] = QStringLiteral("Skala:");
    m_translations[KEY_SHEET][LANG_NAME_INDONESIA] = QStringLiteral("Lembar:");
    m_translations[KEY_TITLE][LANG_NAME_INDONESIA] = QStringLiteral("Judul, judul tambahan:");
    m_translations[KEY_DATE][LANG_NAME_INDONESIA] = QStringLiteral("Tanggal penerbitan:");
    m_translations[KEY_CREATED_BY][LANG_NAME_INDONESIA] = QStringLiteral("Dibuat oleh:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_INDONESIA] = QStringLiteral("Disetujui oleh:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_INDONESIA] = QStringLiteral("Nomor gambar:");
    m_translations[KEY_MATERIAL][LANG_NAME_INDONESIA] = QStringLiteral("Material komponen:");
    m_translations[KEY_REVISION][LANG_NAME_INDONESIA] = QStringLiteral("Revisi:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_INDONESIA] = QStringLiteral("Toleransi umum:");
    m_translations[KEY_OWNER][LANG_NAME_INDONESIA] = QStringLiteral("Pemilik:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_INDONESIA] = QStringLiteral("Jenis dokumen:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_INDONESIA] = QStringLiteral("Departemen:");
    m_translations[KEY_STATUS][LANG_NAME_INDONESIA] = QStringLiteral("Status dokumen:");
    m_translations[KEY_LANGUAGE][LANG_NAME_INDONESIA] = QStringLiteral("Bahasa:");

    // --- Italian (it) ---
    const QString LANG_NAME_ITALIAN = QStringLiteral("Italiano");
    m_translations[KEY_SCALE][LANG_NAME_ITALIAN] = QStringLiteral("Scala:");
    m_translations[KEY_SHEET][LANG_NAME_ITALIAN] = QStringLiteral("Foglio:");
    m_translations[KEY_TITLE][LANG_NAME_ITALIAN] = QStringLiteral("Titoli, titolo supplementare:");
    m_translations[KEY_DATE][LANG_NAME_ITALIAN] = QStringLiteral("Data di emissione:");
    m_translations[KEY_CREATED_BY][LANG_NAME_ITALIAN] = QStringLiteral("Creato da:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_ITALIAN] = QStringLiteral("Approvato da:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_ITALIAN] = QStringLiteral("Numero di disegno:");
    m_translations[KEY_MATERIAL][LANG_NAME_ITALIAN] = QStringLiteral("Materiale della parte:");
    m_translations[KEY_REVISION][LANG_NAME_ITALIAN] = QStringLiteral("Revisione:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_ITALIAN] = QStringLiteral("Tolleranze generali:");
    m_translations[KEY_OWNER][LANG_NAME_ITALIAN] = QStringLiteral("Proprietario:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_ITALIAN] = QStringLiteral("Tipo di documento:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_ITALIAN] = QStringLiteral("Dipartimento:");
    m_translations[KEY_STATUS][LANG_NAME_ITALIAN] = QStringLiteral("Stato del documento:");
    m_translations[KEY_LANGUAGE][LANG_NAME_ITALIAN] = QStringLiteral("Lingua:");

    
    // --- Lietuvių ---
    const QString LANG_NAME_LIETUVIU = QStringLiteral("Lietuvių");
    m_translations[KEY_SCALE][LANG_NAME_LIETUVIU] = QStringLiteral("Mastelis:");
    m_translations[KEY_SHEET][LANG_NAME_LIETUVIU] = QStringLiteral("Lapas:");
    m_translations[KEY_TITLE][LANG_NAME_LIETUVIU] = QStringLiteral("Pavadinimai, papildomas pavadinimas:");
    m_translations[KEY_DATE][LANG_NAME_LIETUVIU] = QStringLiteral("Išleidimo data:");
    m_translations[KEY_CREATED_BY][LANG_NAME_LIETUVIU] = QStringLiteral("Sukūrė:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_LIETUVIU] = QStringLiteral("Patvirtino:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_LIETUVIU] = QStringLiteral("Brėžinio numeris:");
    m_translations[KEY_MATERIAL][LANG_NAME_LIETUVIU] = QStringLiteral("Dalies medžiaga:");
    m_translations[KEY_REVISION][LANG_NAME_LIETUVIU] = QStringLiteral("Revizija:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_LIETUVIU] = QStringLiteral("Bendrosios tolerancijos:");
    m_translations[KEY_OWNER][LANG_NAME_LIETUVIU] = QStringLiteral("Savىininkas:"); // Savininkas
    m_translations[KEY_DOC_TYPE][LANG_NAME_LIETUVIU] = QStringLiteral("Dokumento tipas:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_LIETUVIU] = QStringLiteral("Departamentas:");
    m_translations[KEY_STATUS][LANG_NAME_LIETUVIU] = QStringLiteral("Dokumento būsena:");
    m_translations[KEY_LANGUAGE][LANG_NAME_LIETUVIU] = QStringLiteral("Kalba:");

    // --- Magyar ---
    const QString LANG_NAME_MAGYAR = QStringLiteral("Magyar");
    m_translations[KEY_SCALE][LANG_NAME_MAGYAR] = QStringLiteral("Méretarány:");
    m_translations[KEY_SHEET][LANG_NAME_MAGYAR] = QStringLiteral("Lap:");
    m_translations[KEY_TITLE][LANG_NAME_MAGYAR] = QStringLiteral("Címek, kiegészítő cím:");
    m_translations[KEY_DATE][LANG_NAME_MAGYAR] = QStringLiteral("Kiadás dátuma:");
    m_translations[KEY_CREATED_BY][LANG_NAME_MAGYAR] = QStringLiteral("Készítette:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_MAGYAR] = QStringLiteral("Jóváhagyta:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_MAGYAR] = QStringLiteral("Rajzszám:");
    m_translations[KEY_MATERIAL][LANG_NAME_MAGYAR] = QStringLiteral("Alkatrész anyaga:");
    m_translations[KEY_REVISION][LANG_NAME_MAGYAR] = QStringLiteral("Revízió:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_MAGYAR] = QStringLiteral("Általános tűrések:");
    m_translations[KEY_OWNER][LANG_NAME_MAGYAR] = QStringLiteral("Tulajdonos:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_MAGYAR] = QStringLiteral("Dokumentumtípus:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_MAGYAR] = QStringLiteral("Osztály:");
    m_translations[KEY_STATUS][LANG_NAME_MAGYAR] = QStringLiteral("Dokumentum állapota:");
    m_translations[KEY_LANGUAGE][LANG_NAME_MAGYAR] = QStringLiteral("Nyelv:");

    // --- Nederlands ---
    const QString LANG_NAME_NEDERLANDS = QStringLiteral("Nederlands");
    m_translations[KEY_SCALE][LANG_NAME_NEDERLANDS] = QStringLiteral("Schaal:");
    m_translations[KEY_SHEET][LANG_NAME_NEDERLANDS] = QStringLiteral("Blad:");
    m_translations[KEY_TITLE][LANG_NAME_NEDERLANDS] = QStringLiteral("Titels, aanvullende titel:");
    m_translations[KEY_DATE][LANG_NAME_NEDERLANDS] = QStringLiteral("Uitgiftedatum:");
    m_translations[KEY_CREATED_BY][LANG_NAME_NEDERLANDS] = QStringLiteral("Gemaakt door:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_NEDERLANDS] = QStringLiteral("Goedgekeurd door:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_NEDERLANDS] = QStringLiteral("Tekeningnummer:");
    m_translations[KEY_MATERIAL][LANG_NAME_NEDERLANDS] = QStringLiteral("Materiaal onderdeel:");
    m_translations[KEY_REVISION][LANG_NAME_NEDERLANDS] = QStringLiteral("Revisie:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_NEDERLANDS] = QStringLiteral("Algemene toleranties:");
    m_translations[KEY_OWNER][LANG_NAME_NEDERLANDS] = QStringLiteral("Eigenaar:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_NEDERLANDS] = QStringLiteral("Documenttype:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_NEDERLANDS] = QStringLiteral("Afdeling:");
    m_translations[KEY_STATUS][LANG_NAME_NEDERLANDS] = QStringLiteral("Documentstatus:");
    m_translations[KEY_LANGUAGE][LANG_NAME_NEDERLANDS] = QStringLiteral("Taal:");

    // --- Norsk bokmål ---
    const QString LANG_NAME_NORSK_BOKMAL = QStringLiteral("Norsk bokmål");
    m_translations[KEY_SCALE][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Målestokk:");
    m_translations[KEY_SHEET][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Ark:");
    m_translations[KEY_TITLE][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Titler, supplerende tittel:");
    m_translations[KEY_DATE][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Utgivelsesdato:");
    m_translations[KEY_CREATED_BY][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Opprettet av:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Godkjent av:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Tegningsnummer:");
    m_translations[KEY_MATERIAL][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Materiale for del:");
    m_translations[KEY_REVISION][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Revisjon:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Generelle toleranser:");
    m_translations[KEY_OWNER][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Eier:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Dokumenttype:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Avdeling:");
    m_translations[KEY_STATUS][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Dokumentstatus:");
    m_translations[KEY_LANGUAGE][LANG_NAME_NORSK_BOKMAL] = QStringLiteral("Språk:");

    
    // --- Polski ---
    const QString LANG_NAME_POLSKI = QStringLiteral("Polski");
    m_translations[KEY_SCALE][LANG_NAME_POLSKI] = QStringLiteral("Skala:");
    m_translations[KEY_SHEET][LANG_NAME_POLSKI] = QStringLiteral("Arkusz:");
    m_translations[KEY_TITLE][LANG_NAME_POLSKI] = QStringLiteral("Tytuły, tytuł dodatkowy:");
    m_translations[KEY_DATE][LANG_NAME_POLSKI] = QStringLiteral("Data wydania:");
    m_translations[KEY_CREATED_BY][LANG_NAME_POLSKI] = QStringLiteral("Utworzony przez:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_POLSKI] = QStringLiteral("Zatwierdzony przez:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_POLSKI] = QStringLiteral("Numer rysunku:");
    m_translations[KEY_MATERIAL][LANG_NAME_POLSKI] = QStringLiteral("Materiał części:");
    m_translations[KEY_REVISION][LANG_NAME_POLSKI] = QStringLiteral("Rewizja:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_POLSKI] = QStringLiteral("Tolerancje ogólne:");
    m_translations[KEY_OWNER][LANG_NAME_POLSKI] = QStringLiteral("Właściciel:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_POLSKI] = QStringLiteral("Typ dokumentu:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_POLSKI] = QStringLiteral("Dział:");
    m_translations[KEY_STATUS][LANG_NAME_POLSKI] = QStringLiteral("Status dokumentu:");
    m_translations[KEY_LANGUAGE][LANG_NAME_POLSKI] = QStringLiteral("Język:");

    // --- Português ---
    const QString LANG_NAME_PORTUGUES = QStringLiteral("Português");
    m_translations[KEY_SCALE][LANG_NAME_PORTUGUES] = QStringLiteral("Escala:");
    m_translations[KEY_SHEET][LANG_NAME_PORTUGUES] = QStringLiteral("Folha:");
    m_translations[KEY_TITLE][LANG_NAME_PORTUGUES] = QStringLiteral("Títulos, título suplementar:");
    m_translations[KEY_DATE][LANG_NAME_PORTUGUES] = QStringLiteral("Data de emissão:");
    m_translations[KEY_CREATED_BY][LANG_NAME_PORTUGUES] = QStringLiteral("Criado por:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_PORTUGUES] = QStringLiteral("Aprovado por:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_PORTUGUES] = QStringLiteral("Número do desenho:");
    m_translations[KEY_MATERIAL][LANG_NAME_PORTUGUES] = QStringLiteral("Material da peça:");
    m_translations[KEY_REVISION][LANG_NAME_PORTUGUES] = QStringLiteral("Revisão:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_PORTUGUES] = QStringLiteral("Tolerâncias gerais:");
    m_translations[KEY_OWNER][LANG_NAME_PORTUGUES] = QStringLiteral("Proprietário:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_PORTUGUES] = QStringLiteral("Tipo de documento:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_PORTUGUES] = QStringLiteral("Departamento:");
    m_translations[KEY_STATUS][LANG_NAME_PORTUGUES] = QStringLiteral("Status do documento:");
    m_translations[KEY_LANGUAGE][LANG_NAME_PORTUGUES] = QStringLiteral("Idioma:");

    // --- Română ---
    const QString LANG_NAME_ROMANA = QStringLiteral("Română");
    m_translations[KEY_SCALE][LANG_NAME_ROMANA] = QStringLiteral("Scară:");
    m_translations[KEY_SHEET][LANG_NAME_ROMANA] = QStringLiteral("Foaie:");
    m_translations[KEY_TITLE][LANG_NAME_ROMANA] = QStringLiteral("Titluri, titlu suplimentar:");
    m_translations[KEY_DATE][LANG_NAME_ROMANA] = QStringLiteral("Data emiterii:");
    m_translations[KEY_CREATED_BY][LANG_NAME_ROMANA] = QStringLiteral("Creat de:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_ROMANA] = QStringLiteral("Aprobat de:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_ROMANA] = QStringLiteral("Număr desen:");
    m_translations[KEY_MATERIAL][LANG_NAME_ROMANA] = QStringLiteral("Material piesă:");
    m_translations[KEY_REVISION][LANG_NAME_ROMANA] = QStringLiteral("Revizie:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_ROMANA] = QStringLiteral("Toleranțe generale:");
    m_translations[KEY_OWNER][LANG_NAME_ROMANA] = QStringLiteral("Proprietar:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_ROMANA] = QStringLiteral("Tip document:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_ROMANA] = QStringLiteral("Departament:");
    m_translations[KEY_STATUS][LANG_NAME_ROMANA] = QStringLiteral("Stare document:");
    m_translations[KEY_LANGUAGE][LANG_NAME_ROMANA] = QStringLiteral("Limbă:");

    // --- Slovenčina --- (Slovak)
    const QString LANG_NAME_SLOVENCINA_SK = QStringLiteral("Slovenčina");
    m_translations[KEY_SCALE][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Mierka:");
    m_translations[KEY_SHEET][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("List:");
    m_translations[KEY_TITLE][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Názvy, doplňujúci názov:");
    m_translations[KEY_DATE][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Dátum vydania:");
    m_translations[KEY_CREATED_BY][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Vytvoril:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Schválil:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Číslo výkresu:");
    m_translations[KEY_MATERIAL][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Materiál dielu:");
    m_translations[KEY_REVISION][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Revízia:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Všeobecné tolerancie:");
    m_translations[KEY_OWNER][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Vlastník:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Typ dokumentu:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Oddelenie:");
    m_translations[KEY_STATUS][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Stav dokumentu:");
    m_translations[KEY_LANGUAGE][LANG_NAME_SLOVENCINA_SK] = QStringLiteral("Jazyk:");
    
    // --- Slovenščina --- (Slovenian)
    const QString LANG_NAME_SLOVENSCINA_SL = QStringLiteral("Slovenščina");
    m_translations[KEY_SCALE][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Merilo:");
    m_translations[KEY_SHEET][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("List:");
    m_translations[KEY_TITLE][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Naslovi, dodatni naslov:");
    m_translations[KEY_DATE][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Datum izdaje:");
    m_translations[KEY_CREATED_BY][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Izdelal:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Odobril:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Številka risbe:");
    m_translations[KEY_MATERIAL][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Material dela:");
    m_translations[KEY_REVISION][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Revizija:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Splošne tolerance:");
    m_translations[KEY_OWNER][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Lastnik:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Vrsta dokumenta:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Oddelek:");
    m_translations[KEY_STATUS][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Status dokumenta:");
    m_translations[KEY_LANGUAGE][LANG_NAME_SLOVENSCINA_SL] = QStringLiteral("Jezik:");

    // --- Srpski --- (Serbian Latin)
    const QString LANG_NAME_SRPSKI_LATIN = QStringLiteral("Srpski"); // Assuming "Srpski" is Latin entry
    m_translations[KEY_SCALE][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Razmera:");
    m_translations[KEY_SHEET][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("List:");
    m_translations[KEY_TITLE][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Naslovi, dodatni naslov:");
    m_translations[KEY_DATE][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Datum izdavanja:");
    m_translations[KEY_CREATED_BY][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Izradio:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Odobrio:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Broj crteža:");
    m_translations[KEY_MATERIAL][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Materijal dela:");
    m_translations[KEY_REVISION][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Revizija:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Opšte tolerancije:");
    m_translations[KEY_OWNER][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Vlasnik:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Tip dokumenta:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Odeljenje:");
    m_translations[KEY_STATUS][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Status dokumenta:");
    m_translations[KEY_LANGUAGE][LANG_NAME_SRPSKI_LATIN] = QStringLiteral("Jezik:");

    // --- Suomi ---
    const QString LANG_NAME_SUOMI = QStringLiteral("Suomi");
    m_translations[KEY_SCALE][LANG_NAME_SUOMI] = QStringLiteral("Mittakaava:");
    m_translations[KEY_SHEET][LANG_NAME_SUOMI] = QStringLiteral("Lehti:");
    m_translations[KEY_TITLE][LANG_NAME_SUOMI] = QStringLiteral("Otsikot, lisäotsikko:");
    m_translations[KEY_DATE][LANG_NAME_SUOMI] = QStringLiteral("Julkaisupäivä:");
    m_translations[KEY_CREATED_BY][LANG_NAME_SUOMI] = QStringLiteral("Luonut:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_SUOMI] = QStringLiteral("Hyväksynyt:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_SUOMI] = QStringLiteral("Piirustusnumero:");
    m_translations[KEY_MATERIAL][LANG_NAME_SUOMI] = QStringLiteral("Osan materiaali:");
    m_translations[KEY_REVISION][LANG_NAME_SUOMI] = QStringLiteral("Versio:"); // Or "Tarkistus:"
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_SUOMI] = QStringLiteral("Yleiset toleranssit:");
    m_translations[KEY_OWNER][LANG_NAME_SUOMI] = QStringLiteral("Omistaja:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_SUOMI] = QStringLiteral("Asiakirjatyyppi:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_SUOMI] = QStringLiteral("Osasto:");
    m_translations[KEY_STATUS][LANG_NAME_SUOMI] = QStringLiteral("Asiakirjan tila:");
    m_translations[KEY_LANGUAGE][LANG_NAME_SUOMI] = QStringLiteral("Kieli:");

    // --- Svenska ---
    const QString LANG_NAME_SVENSKA = QStringLiteral("Svenska");
    m_translations[KEY_SCALE][LANG_NAME_SVENSKA] = QStringLiteral("Skala:");
    m_translations[KEY_SHEET][LANG_NAME_SVENSKA] = QStringLiteral("Blad:");
    m_translations[KEY_TITLE][LANG_NAME_SVENSKA] = QStringLiteral("Titlar, tilläggstitel:");
    m_translations[KEY_DATE][LANG_NAME_SVENSKA] = QStringLiteral("Utgivningsdatum:");
    m_translations[KEY_CREATED_BY][LANG_NAME_SVENSKA] = QStringLiteral("Skapad av:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_SVENSKA] = QStringLiteral("Godkänd av:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_SVENSKA] = QStringLiteral("Ritningsnummer:");
    m_translations[KEY_MATERIAL][LANG_NAME_SVENSKA] = QStringLiteral("Material, detalj:");
    m_translations[KEY_REVISION][LANG_NAME_SVENSKA] = QStringLiteral("Revision:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_SVENSKA] = QStringLiteral("Allmänna toleranser:");
    m_translations[KEY_OWNER][LANG_NAME_SVENSKA] = QStringLiteral("Ägare:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_SVENSKA] = QStringLiteral("Dokumenttyp:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_SVENSKA] = QStringLiteral("Avdelning:");
    m_translations[KEY_STATUS][LANG_NAME_SVENSKA] = QStringLiteral("Dokumentstatus:");
    m_translations[KEY_LANGUAGE][LANG_NAME_SVENSKA] = QStringLiteral("Språk:");

    // --- Taqbaylit --- (Kabyle)
    const QString LANG_NAME_TAQBAYLIT = QStringLiteral("Taqbaylit");
    m_translations[KEY_SCALE][LANG_NAME_TAQBAYLIT] = QStringLiteral("Taktilt:");
    m_translations[KEY_SHEET][LANG_NAME_TAQBAYLIT] = QStringLiteral("Tazrawt:");
    m_translations[KEY_TITLE][LANG_NAME_TAQBAYLIT] = QStringLiteral("Izwilen, azwel umarniḍ:");
    m_translations[KEY_DATE][LANG_NAME_TAQBAYLIT] = QStringLiteral("Azemz n tuffɣa:");
    m_translations[KEY_CREATED_BY][LANG_NAME_TAQBAYLIT] = QStringLiteral("Yexdem-it:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_TAQBAYLIT] = QStringLiteral("Yeɣɣar-it:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_TAQBAYLIT] = QStringLiteral("Uṭṭun n tuqqna:");
    m_translations[KEY_MATERIAL][LANG_NAME_TAQBAYLIT] = QStringLiteral("Agbur n teftart:");
    m_translations[KEY_REVISION][LANG_NAME_TAQBAYLIT] = QStringLiteral("As revisión:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_TAQBAYLIT] = QStringLiteral("Timerna timuta:");
    m_translations[KEY_OWNER][LANG_NAME_TAQBAYLIT] = QStringLiteral("Bab-is:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_TAQBAYLIT] = QStringLiteral("Anaw n warat:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_TAQBAYLIT] = QStringLiteral("Aḥric:");
    m_translations[KEY_STATUS][LANG_NAME_TAQBAYLIT] = QStringLiteral("Addad n warat:");
    m_translations[KEY_LANGUAGE][LANG_NAME_TAQBAYLIT] = QStringLiteral("Tutlayt:");

    // --- Tiếng Việt ---
    const QString LANG_NAME_TIENG_VIET = QStringLiteral("Tiếng Việt");
    m_translations[KEY_SCALE][LANG_NAME_TIENG_VIET] = QStringLiteral("Tỷ lệ:");
    m_translations[KEY_SHEET][LANG_NAME_TIENG_VIET] = QStringLiteral("Trang:");
    m_translations[KEY_TITLE][LANG_NAME_TIENG_VIET] = QStringLiteral("Tiêu đề, tiêu đề bổ sung:");
    m_translations[KEY_DATE][LANG_NAME_TIENG_VIET] = QStringLiteral("Ngày phát hành:");
    m_translations[KEY_CREATED_BY][LANG_NAME_TIENG_VIET] = QStringLiteral("Tạo bởi:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_TIENG_VIET] = QStringLiteral("Phê duyệt bởi:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_TIENG_VIET] = QStringLiteral("Số bản vẽ:");
    m_translations[KEY_MATERIAL][LANG_NAME_TIENG_VIET] = QStringLiteral("Vật liệu chi tiết:");
    m_translations[KEY_REVISION][LANG_NAME_TIENG_VIET] = QStringLiteral("Phiên bản:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_TIENG_VIET] = QStringLiteral("Dung sai chung:");
    m_translations[KEY_OWNER][LANG_NAME_TIENG_VIET] = QStringLiteral("Chủ sở hữu:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_TIENG_VIET] = QStringLiteral("Loại tài liệu:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_TIENG_VIET] = QStringLiteral("Bộ phận:");
    m_translations[KEY_STATUS][LANG_NAME_TIENG_VIET] = QStringLiteral("Trạng thái tài liệu:");
    m_translations[KEY_LANGUAGE][LANG_NAME_TIENG_VIET] = QStringLiteral("Ngôn ngữ:");

    // --- Türkçe ---
    const QString LANG_NAME_TURKCE = QStringLiteral("Türkçe");
    m_translations[KEY_SCALE][LANG_NAME_TURKCE] = QStringLiteral("Ölçek:");
    m_translations[KEY_SHEET][LANG_NAME_TURKCE] = QStringLiteral("Sayfa:");
    m_translations[KEY_TITLE][LANG_NAME_TURKCE] = QStringLiteral("Başlıklar, ek başlık:");
    m_translations[KEY_DATE][LANG_NAME_TURKCE] = QStringLiteral("Yayın tarihi:");
    m_translations[KEY_CREATED_BY][LANG_NAME_TURKCE] = QStringLiteral("Oluşturan:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_TURKCE] = QStringLiteral("Onaylayan:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_TURKCE] = QStringLiteral("Çizim numarası:");
    m_translations[KEY_MATERIAL][LANG_NAME_TURKCE] = QStringLiteral("Parça malzemesi:");
    m_translations[KEY_REVISION][LANG_NAME_TURKCE] = QStringLiteral("Revizyon:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_TURKCE] = QStringLiteral("Genel toleranslar:");
    m_translations[KEY_OWNER][LANG_NAME_TURKCE] = QStringLiteral("Sahip:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_TURKCE] = QStringLiteral("Belge türü:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_TURKCE] = QStringLiteral("Departman:");
    m_translations[KEY_STATUS][LANG_NAME_TURKCE] = QStringLiteral("Belge durumu:");
    m_translations[KEY_LANGUAGE][LANG_NAME_TURKCE] = QStringLiteral("Dil:");

    // --- Valencian --- (Often very similar to Català)
    const QString LANG_NAME_VALENCIAN = QStringLiteral("Valencian");
    m_translations[KEY_SCALE][LANG_NAME_VALENCIAN] = QStringLiteral("Escala:");
    m_translations[KEY_SHEET][LANG_NAME_VALENCIAN] = QStringLiteral("Full:");
    m_translations[KEY_TITLE][LANG_NAME_VALENCIAN] = QStringLiteral("Títols, títol suplementari:");
    m_translations[KEY_DATE][LANG_NAME_VALENCIAN] = QStringLiteral("Data d'emissió:");
    m_translations[KEY_CREATED_BY][LANG_NAME_VALENCIAN] = QStringLiteral("Creat per:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_VALENCIAN] = QStringLiteral("Aprovat per:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_VALENCIAN] = QStringLiteral("Número de dibuix:");
    m_translations[KEY_MATERIAL][LANG_NAME_VALENCIAN] = QStringLiteral("Material de la peça:");
    m_translations[KEY_REVISION][LANG_NAME_VALENCIAN] = QStringLiteral("Revisió:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_VALENCIAN] = QStringLiteral("Toleràncies generals:");
    m_translations[KEY_OWNER][LANG_NAME_VALENCIAN] = QStringLiteral("Propietari:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_VALENCIAN] = QStringLiteral("Tipus de document:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_VALENCIAN] = QStringLiteral("Departament:");
    m_translations[KEY_STATUS][LANG_NAME_VALENCIAN] = QStringLiteral("Estat del document:");
    m_translations[KEY_LANGUAGE][LANG_NAME_VALENCIAN] = QStringLiteral("Idioma:");

    // --- Čeština ---
    const QString LANG_NAME_CESTINA = QStringLiteral("Čeština");
    m_translations[KEY_SCALE][LANG_NAME_CESTINA] = QStringLiteral("Měřítko:");
    m_translations[KEY_SHEET][LANG_NAME_CESTINA] = QStringLiteral("List:");
    m_translations[KEY_TITLE][LANG_NAME_CESTINA] = QStringLiteral("Názvy, doplňkový název:");
    m_translations[KEY_DATE][LANG_NAME_CESTINA] = QStringLiteral("Datum vydání:");
    m_translations[KEY_CREATED_BY][LANG_NAME_CESTINA] = QStringLiteral("Vytvořil:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_CESTINA] = QStringLiteral("Schválil:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_CESTINA] = QStringLiteral("Číslo výkresu:");
    m_translations[KEY_MATERIAL][LANG_NAME_CESTINA] = QStringLiteral("Materiál dílu:");
    m_translations[KEY_REVISION][LANG_NAME_CESTINA] = QStringLiteral("Revize:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_CESTINA] = QStringLiteral("Obecné tolerance:");
    m_translations[KEY_OWNER][LANG_NAME_CESTINA] = QStringLiteral("Vlastník:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_CESTINA] = QStringLiteral("Typ dokumentu:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_CESTINA] = QStringLiteral("Oddělení:");
    m_translations[KEY_STATUS][LANG_NAME_CESTINA] = QStringLiteral("Stav dokumentu:");
    m_translations[KEY_LANGUAGE][LANG_NAME_CESTINA] = QStringLiteral("Jazyk:");

    // --- Ελληνικά --- (Greek)
    const QString LANG_NAME_ELLINIKA = QStringLiteral("Ελληνικά");
    m_translations[KEY_SCALE][LANG_NAME_ELLINIKA] = QStringLiteral("Κλίμακα:");
    m_translations[KEY_SHEET][LANG_NAME_ELLINIKA] = QStringLiteral("Φύλλο:");
    m_translations[KEY_TITLE][LANG_NAME_ELLINIKA] = QStringLiteral("Τίτλοι, συμπληρωματικός τίτλος:");
    m_translations[KEY_DATE][LANG_NAME_ELLINIKA] = QStringLiteral("Ημερομηνία έκδοσης:");
    m_translations[KEY_CREATED_BY][LANG_NAME_ELLINIKA] = QStringLiteral("Δημιουργήθηκε από:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_ELLINIKA] = QStringLiteral("Εγκρίθηκε από:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_ELLINIKA] = QStringLiteral("Αριθμός σχεδίου:");
    m_translations[KEY_MATERIAL][LANG_NAME_ELLINIKA] = QStringLiteral("Υλικό εξαρτήματος:");
    m_translations[KEY_REVISION][LANG_NAME_ELLINIKA] = QStringLiteral("Αναθεώρηση:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_ELLINIKA] = QStringLiteral("Γενικές ανοχές:");
    m_translations[KEY_OWNER][LANG_NAME_ELLINIKA] = QStringLiteral("Κάτοχος:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_ELLINIKA] = QStringLiteral("Τύπος εγγράφου:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_ELLINIKA] = QStringLiteral("Τμήμα:");
    m_translations[KEY_STATUS][LANG_NAME_ELLINIKA] = QStringLiteral("Κατάσταση εγγράφου:");
    m_translations[KEY_LANGUAGE][LANG_NAME_ELLINIKA] = QStringLiteral("Γλώσσα:");

    // --- Беларуская --- (Belarusian)
    const QString LANG_NAME_BELARUSKAYA = QStringLiteral("Беларуская");
    m_translations[KEY_SCALE][LANG_NAME_BELARUSKAYA] = QStringLiteral("Маштаб:");
    m_translations[KEY_SHEET][LANG_NAME_BELARUSKAYA] = QStringLiteral("Ліст:");
    m_translations[KEY_TITLE][LANG_NAME_BELARUSKAYA] = QStringLiteral("Назвы, дадатковая назва:");
    m_translations[KEY_DATE][LANG_NAME_BELARUSKAYA] = QStringLiteral("Дата выдачы:");
    m_translations[KEY_CREATED_BY][LANG_NAME_BELARUSKAYA] = QStringLiteral("Створана:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_BELARUSKAYA] = QStringLiteral("Зацверджана:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_BELARUSKAYA] = QStringLiteral("Нумар чарцяжа:");
    m_translations[KEY_MATERIAL][LANG_NAME_BELARUSKAYA] = QStringLiteral("Матэрыял дэталі:");
    m_translations[KEY_REVISION][LANG_NAME_BELARUSKAYA] = QStringLiteral("Рэвізія:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_BELARUSKAYA] = QStringLiteral("Агульныя допускі:");
    m_translations[KEY_OWNER][LANG_NAME_BELARUSKAYA] = QStringLiteral("Уладальнік:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_BELARUSKAYA] = QStringLiteral("Тып дакумента:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_BELARUSKAYA] = QStringLiteral("Аддзел:");
    m_translations[KEY_STATUS][LANG_NAME_BELARUSKAYA] = QStringLiteral("Стан дакумента:");
    m_translations[KEY_LANGUAGE][LANG_NAME_BELARUSKAYA] = QStringLiteral("Мова:");

    // --- Български --- (Bulgarian)
    const QString LANG_NAME_BULGARSKI = QStringLiteral("Български");
    m_translations[KEY_SCALE][LANG_NAME_BULGARSKI] = QStringLiteral("Мащаб:");
    m_translations[KEY_SHEET][LANG_NAME_BULGARSKI] = QStringLiteral("Лист:");
    m_translations[KEY_TITLE][LANG_NAME_BULGARSKI] = QStringLiteral("Заглавия, допълнително заглавие:");
    m_translations[KEY_DATE][LANG_NAME_BULGARSKI] = QStringLiteral("Дата на издаване:");
    m_translations[KEY_CREATED_BY][LANG_NAME_BULGARSKI] = QStringLiteral("Създадено от:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_BULGARSKI] = QStringLiteral("Одобрено от:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_BULGARSKI] = QStringLiteral("Номер на чертежа:");
    m_translations[KEY_MATERIAL][LANG_NAME_BULGARSKI] = QStringLiteral("Материал на частта:");
    m_translations[KEY_REVISION][LANG_NAME_BULGARSKI] = QStringLiteral("Ревизия:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_BULGARSKI] = QStringLiteral("Общи допустими отклонения:");
    m_translations[KEY_OWNER][LANG_NAME_BULGARSKI] = QStringLiteral("Собственик:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_BULGARSKI] = QStringLiteral("Тип на документа:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_BULGARSKI] = QStringLiteral("Отдел:");
    m_translations[KEY_STATUS][LANG_NAME_BULGARSKI] = QStringLiteral("Статус на документа:");
    m_translations[KEY_LANGUAGE][LANG_NAME_BULGARSKI] = QStringLiteral("Език:");

    // --- Русский --- (Russian)
    const QString LANG_NAME_RUSSKIY = QStringLiteral("Русский");
    m_translations[KEY_SCALE][LANG_NAME_RUSSKIY] = QStringLiteral("Масштаб:");
    m_translations[KEY_SHEET][LANG_NAME_RUSSKIY] = QStringLiteral("Лист:");
    m_translations[KEY_TITLE][LANG_NAME_RUSSKIY] = QStringLiteral("Наименования, дополнительное наименование:");
    m_translations[KEY_DATE][LANG_NAME_RUSSKIY] = QStringLiteral("Дата выпуска:");
    m_translations[KEY_CREATED_BY][LANG_NAME_RUSSKIY] = QStringLiteral("Разработал:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_RUSSKIY] = QStringLiteral("Утвердил:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_RUSSKIY] = QStringLiteral("Номер чертежа:");
    m_translations[KEY_MATERIAL][LANG_NAME_RUSSKIY] = QStringLiteral("Материал детали:");
    m_translations[KEY_REVISION][LANG_NAME_RUSSKIY] = QStringLiteral("Ревизия:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_RUSSKIY] = QStringLiteral("Общие допуски:");
    m_translations[KEY_OWNER][LANG_NAME_RUSSKIY] = QStringLiteral("Владелец:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_RUSSKIY] = QStringLiteral("Тип документа:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_RUSSKIY] = QStringLiteral("Отдел:");
    m_translations[KEY_STATUS][LANG_NAME_RUSSKIY] = QStringLiteral("Статус документа:");
    m_translations[KEY_LANGUAGE][LANG_NAME_RUSSKIY] = QStringLiteral("Язык:");

    // --- Српски --- (Serbian Cyrillic)
    const QString LANG_NAME_SRPSKI_CYRILLIC = QStringLiteral("Српски"); // The Cyrillic one
    m_translations[KEY_SCALE][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Размера:");
    m_translations[KEY_SHEET][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Лист:");
    m_translations[KEY_TITLE][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Наслови, додатни наслов:");
    m_translations[KEY_DATE][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Датум издавања:");
    m_translations[KEY_CREATED_BY][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Израдио:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Одобрио:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Број цртежа:");
    m_translations[KEY_MATERIAL][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Материјал дела:");
    m_translations[KEY_REVISION][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Ревизија:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Опште толеранције:");
    m_translations[KEY_OWNER][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Власник:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Тип документа:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Одељење:");
    m_translations[KEY_STATUS][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Статус документа:");
    m_translations[KEY_LANGUAGE][LANG_NAME_SRPSKI_CYRILLIC] = QStringLiteral("Језик:");

    // --- Українська --- (Ukrainian)
    const QString LANG_NAME_UKRAINSKA = QStringLiteral("Українська");
    m_translations[KEY_SCALE][LANG_NAME_UKRAINSKA] = QStringLiteral("Масштаб:");
    m_translations[KEY_SHEET][LANG_NAME_UKRAINSKA] = QStringLiteral("Аркуш:");
    m_translations[KEY_TITLE][LANG_NAME_UKRAINSKA] = QStringLiteral("Назви, додаткова назва:");
    m_translations[KEY_DATE][LANG_NAME_UKRAINSKA] = QStringLiteral("Дата видання:");
    m_translations[KEY_CREATED_BY][LANG_NAME_UKRAINSKA] = QStringLiteral("Створено:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_UKRAINSKA] = QStringLiteral("Затверджено:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_UKRAINSKA] = QStringLiteral("Номер креслення:");
    m_translations[KEY_MATERIAL][LANG_NAME_UKRAINSKA] = QStringLiteral("Матеріал деталі:");
    m_translations[KEY_REVISION][LANG_NAME_UKRAINSKA] = QStringLiteral("Ревізія:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_UKRAINSKA] = QStringLiteral("Загальні допуски:");
    m_translations[KEY_OWNER][LANG_NAME_UKRAINSKA] = QStringLiteral("Власник:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_UKRAINSKA] = QStringLiteral("Тип документа:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_UKRAINSKA] = QStringLiteral("Відділ:");
    m_translations[KEY_STATUS][LANG_NAME_UKRAINSKA] = QStringLiteral("Стан документа:");
    m_translations[KEY_LANGUAGE][LANG_NAME_UKRAINSKA] = QStringLiteral("Мова:");
    
    // --- العربية --- (Arabic)
    // Note on RTL: The colon placement might need adjustment based on overall title block layout for RTL.
    // These translations keep the colon at the end of the Arabic phrase.
    const QString LANG_NAME_ARABIC = QStringLiteral("العربية");
    m_translations[KEY_SCALE][LANG_NAME_ARABIC] = QStringLiteral("مقياس الرسم:");
    m_translations[KEY_SHEET][LANG_NAME_ARABIC] = QStringLiteral("الورقة:");
    m_translations[KEY_TITLE][LANG_NAME_ARABIC] = QStringLiteral("العناوين، عنوان إضافي:");
    m_translations[KEY_DATE][LANG_NAME_ARABIC] = QStringLiteral("تاريخ الإصدار:");
    m_translations[KEY_CREATED_BY][LANG_NAME_ARABIC] = QStringLiteral("أنشأه:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_ARABIC] = QStringLiteral("وافق عليه:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_ARABIC] = QStringLiteral("رقم الرسم:");
    m_translations[KEY_MATERIAL][LANG_NAME_ARABIC] = QStringLiteral("مادة الجزء:");
    m_translations[KEY_REVISION][LANG_NAME_ARABIC] = QStringLiteral("مراجعة:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_ARABIC] = QStringLiteral("التفاوتات العامة:");
    m_translations[KEY_OWNER][LANG_NAME_ARABIC] = QStringLiteral("المالك:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_ARABIC] = QStringLiteral("نوع المستند:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_ARABIC] = QStringLiteral("القسم:");
    m_translations[KEY_STATUS][LANG_NAME_ARABIC] = QStringLiteral("حالة المستند:");
    m_translations[KEY_LANGUAGE][LANG_NAME_ARABIC] = QStringLiteral("اللغة:");

    // --- ქართული --- (Georgian)
    const QString LANG_NAME_KARTULI = QStringLiteral("ქართული");
    m_translations[KEY_SCALE][LANG_NAME_KARTULI] = QStringLiteral("მასშტაბი:");
    m_translations[KEY_SHEET][LANG_NAME_KARTULI] = QStringLiteral("ფურცელი:");
    m_translations[KEY_TITLE][LANG_NAME_KARTULI] = QStringLiteral("სათაურები, დამატებითი სათაური:");
    m_translations[KEY_DATE][LANG_NAME_KARTULI] = QStringLiteral("გამოცემის თარიღი:");
    m_translations[KEY_CREATED_BY][LANG_NAME_KARTULI] = QStringLiteral("შექმნილია:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_KARTULI] = QStringLiteral("დამტკიცებულია:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_KARTULI] = QStringLiteral("ნახაზის ნომერი:");
    m_translations[KEY_MATERIAL][LANG_NAME_KARTULI] = QStringLiteral("ნაწილის მასალა:");
    m_translations[KEY_REVISION][LANG_NAME_KARTULI] = QStringLiteral("რევიზია:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_KARTULI] = QStringLiteral("ზოგადი ტოლერანცები:");
    m_translations[KEY_OWNER][LANG_NAME_KARTULI] = QStringLiteral("მფლობელი:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_KARTULI] = QStringLiteral("დოკუმენტის ტიპი:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_KARTULI] = QStringLiteral("დეპარტამენტი:");
    m_translations[KEY_STATUS][LANG_NAME_KARTULI] = QStringLiteral("დოკუმენტის სტატუსი:");
    m_translations[KEY_LANGUAGE][LANG_NAME_KARTULI] = QStringLiteral("ენა:");

    // --- 日本語 --- (Japanese)
    const QString LANG_NAME_NIHONGO = QStringLiteral("日本語");
    m_translations[KEY_SCALE][LANG_NAME_NIHONGO] = QStringLiteral("縮尺:");
    m_translations[KEY_SHEET][LANG_NAME_NIHONGO] = QStringLiteral("図番:");
    m_translations[KEY_TITLE][LANG_NAME_NIHONGO] = QStringLiteral("表題、副題:");
    m_translations[KEY_DATE][LANG_NAME_NIHONGO] = QStringLiteral("発行日:");
    m_translations[KEY_CREATED_BY][LANG_NAME_NIHONGO] = QStringLiteral("作成者:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_NIHONGO] = QStringLiteral("承認者:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_NIHONGO] = QStringLiteral("図面番号:");
    m_translations[KEY_MATERIAL][LANG_NAME_NIHONGO] = QStringLiteral("部品材料:");
    m_translations[KEY_REVISION][LANG_NAME_NIHONGO] = QStringLiteral("改訂:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_NIHONGO] = QStringLiteral("一般公差:");
    m_translations[KEY_OWNER][LANG_NAME_NIHONGO] = QStringLiteral("所有者:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_NIHONGO] = QStringLiteral("文書タイプ:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_NIHONGO] = QStringLiteral("部門:");
    m_translations[KEY_STATUS][LANG_NAME_NIHONGO] = QStringLiteral("文書ステータス:");
    m_translations[KEY_LANGUAGE][LANG_NAME_NIHONGO] = QStringLiteral("言語:");

    // --- 简体中文 --- (Chinese Simplified)
    const QString LANG_NAME_CHINESE_SIMPLIFIED = QStringLiteral("简体中文");
    m_translations[KEY_SCALE][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("比例:");
    m_translations[KEY_SHEET][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("张:");
    m_translations[KEY_TITLE][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("标题, 副标题:");
    m_translations[KEY_DATE][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("发行日期:");
    m_translations[KEY_CREATED_BY][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("创建者:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("批准者:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("图号:");
    m_translations[KEY_MATERIAL][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("零件材料:");
    m_translations[KEY_REVISION][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("修订版本:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("通用公差:");
    m_translations[KEY_OWNER][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("所有者:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("文档类型:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("部门:");
    m_translations[KEY_STATUS][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("文档状态:");
    m_translations[KEY_LANGUAGE][LANG_NAME_CHINESE_SIMPLIFIED] = QStringLiteral("语言:");

    // --- 繁體中文 --- (Chinese Traditional)
    const QString LANG_NAME_CHINESE_TRADITIONAL = QStringLiteral("繁體中文");
    m_translations[KEY_SCALE][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("比例:");
    m_translations[KEY_SHEET][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("圖紙:");
    m_translations[KEY_TITLE][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("標題, 副標題:");
    m_translations[KEY_DATE][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("發行日期:");
    m_translations[KEY_CREATED_BY][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("創建者:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("批准者:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("圖號:");
    m_translations[KEY_MATERIAL][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("零件材料:");
    m_translations[KEY_REVISION][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("修訂版本:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("通用公差:");
    m_translations[KEY_OWNER][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("所有者:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("文檔類型:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("部門:");
    m_translations[KEY_STATUS][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("文檔狀態:");
    m_translations[KEY_LANGUAGE][LANG_NAME_CHINESE_TRADITIONAL] = QStringLiteral("語言:");

    // --- 한국어 --- (Korean)
    const QString LANG_NAME_HANGUKEO = QStringLiteral("한국어");
    m_translations[KEY_SCALE][LANG_NAME_HANGUKEO] = QStringLiteral("축척:");
    m_translations[KEY_SHEET][LANG_NAME_HANGUKEO] = QStringLiteral("시트:");
    m_translations[KEY_TITLE][LANG_NAME_HANGUKEO] = QStringLiteral("제목, 부제목:");
    m_translations[KEY_DATE][LANG_NAME_HANGUKEO] = QStringLiteral("발행일:");
    m_translations[KEY_CREATED_BY][LANG_NAME_HANGUKEO] = QStringLiteral("작성자:");
    m_translations[KEY_APPROVED_BY][LANG_NAME_HANGUKEO] = QStringLiteral("승인자:");
    m_translations[KEY_DRAWING_NUMBER][LANG_NAME_HANGUKEO] = QStringLiteral("도면 번호:");
    m_translations[KEY_MATERIAL][LANG_NAME_HANGUKEO] = QStringLiteral("부품 재질:");
    m_translations[KEY_REVISION][LANG_NAME_HANGUKEO] = QStringLiteral("개정:");
    m_translations[KEY_GENERAL_TOLERANCES][LANG_NAME_HANGUKEO] = QStringLiteral("일반 공차:");
    m_translations[KEY_OWNER][LANG_NAME_HANGUKEO] = QStringLiteral("소유자:");
    m_translations[KEY_DOC_TYPE][LANG_NAME_HANGUKEO] = QStringLiteral("문서 유형:");
    m_translations[KEY_DEPARTEMENT][LANG_NAME_HANGUKEO] = QStringLiteral("부서:");
    m_translations[KEY_STATUS][LANG_NAME_HANGUKEO] = QStringLiteral("문서 상태:");
    m_translations[KEY_LANGUAGE][LANG_NAME_HANGUKEO] = QStringLiteral("언어:");
}

QString TemplateTranslator::translate(const QString& key, const QString& languageCode) const
{
    if (m_translations.contains(key)) {
        const auto& langMap = m_translations.value(key);
        if (langMap.contains(languageCode)) {
            return langMap.value(languageCode);
        }
        // Fallback to English if specific language translation is missing for this key
        if (langMap.contains(QStringLiteral("English"))) {
            Base::Console().log(
                "TemplateTranslator: Key '%s' translated to English (fallback for lang '%s').\n",
                key.toStdString().c_str(),
                languageCode.toStdString().c_str());
            return langMap.value(QStringLiteral("English"));
        }
    }
    Base::Console().warning(
        "TemplateTranslator: No translation found for key '%s' (lang '%s'). Returning key.\n",
        key.toStdString().c_str(),
        languageCode.toStdString().c_str());

    return key;  // Return the key itself if no translation found at all
}

QStringList TemplateTranslator::getAllKeys() const
{
    return m_translations.keys();
}

QStringList TemplateTranslator::getSupportedLanguageNames() const
{
    // Collect all unique language names for which at least one translation exists
    QSet<QString> names;
    for (const auto& langMap : m_translations.values()) {
        for (const QString& name : langMap.keys()) {
            names.insert(name);
        }
    }
    QStringList list = names.values();
    std::sort(list.begin(), list.end());
    return list;
}

}  // namespace TechDraw