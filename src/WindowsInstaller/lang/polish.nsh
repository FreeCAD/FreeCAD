/*
FreeCAD Installer Language File
Language: Polish
*/

!insertmacro LANGFILE_EXT "Polish"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Zainstalowane dla bieżącego użytkownika)"

${LangFileString} TEXT_WELCOME "Kreator przeprowadzi Ciebie przez proces instalacji FreeCAD$\'a.$\r$\n\
				$\r$\n\
				$_CLICK"

#${LangFileString} TEXT_CONFIGURE_PYTHON "Kompilowanie skryptów Python..."

${LangFileString} TEXT_FINISH_DESKTOP "Utwórz skrót na pulpicie"
${LangFileString} TEXT_FINISH_WEBSITE "Odwiedź freecadweb.org by poznać wiadomości i wskazówki lub skorzystać ze wsparcia"

#${LangFileString} FileTypeTitle "Dokument FreeCAD"

#${LangFileString} SecAllUsersTitle "Instalacja dla wszystkich użytkowników?"
${LangFileString} SecFileAssocTitle "Skojarzenie plików .FCStd"
${LangFileString} SecDesktopTitle "Ikona na pulpicie"

${LangFileString} SecCoreDescription "Pliki FreeCAD$\'a."
#${LangFileString} SecAllUsersDescription "Instalacja dla wszystkich użytkowników lub tylko dla bieżącego użytkownika."
${LangFileString} SecFileAssocDescription "Skojarzenie FreeCAD-a z plikami o rozszerzeniu .FCStd."
${LangFileString} SecDesktopDescription "Ikona FreeCAD$\'a na pulpicie."
#${LangFileString} SecDictionaries "Słowniki"
#${LangFileString} SecDictionariesDescription "Słowniki sprawdzania pisowni, które mogą zostać pobrane i zainstalowane."

#${LangFileString} PathName 'Ścieżka do pliku $\"xxx.exe$\"'
#${LangFileString} InvalidFolder 'Plik $\"xxx.exe$\" nie znajduje się w podanej ścieżce.'

#${LangFileString} DictionariesFailed 'Pobranie słownika dla języka $\"$R3$\" nie powiodło się.'

#${LangFileString} ConfigInfo "Dalsza konfiguracja FreeCAD$\'a chwilę potrwa."

#${LangFileString} RunConfigureFailed "Niedana próba wykonania skryptu konfiguracyjnego"
${LangFileString} InstallRunning "Instalator jest już uruchomiony!"
${LangFileString} AlreadyInstalled "FreeCAD ${APP_SERIES_KEY2} jest już zainstalowany!$\r$\n\
				Installing over existing installations is not recommended if the installed version$\r$\n\
				is a test release or if you have problems with your existing FreeCAD installation.$\r$\n\
				In these cases better reinstall FreeCAD.$\r$\n\
				Dou you nevertheles want to install FreeCAD over the existing version?"
${LangFileString} NewerInstalled "Próbujesz zainstalować starszą wersję FreeCAD, niż ta która jest już zainstalowana.$\r$\n\
				  Jeżeli naprawdę chcesz tego dokonać, musisz wpierw odinstalować FreeCAD $OldVersionNumber."

#${LangFileString} FinishPageMessage "Gratulacje! FreeCAD został pomyślnie zainstalowany.$\r$\n\
#					$\r$\n\
#					(Pierwsze uruchomienie może potrwać kilka sekund.)"
${LangFileString} FinishPageRun "Uruchom FreeCAD"

${LangFileString} UnNotInRegistryLabel "Nie można znaleźć FreeCAD$\'a w rejestrze.$\r$\n\
					Skróty na pulpicie i w menu Start nie zostaną usunięte."
${LangFileString} UnInstallRunning "Musisz najpierw zamknąć FreeCAD$\'a!"
${LangFileString} UnNotAdminLabel "Musisz posiadać prawa administratora do deinstalacji programu FreeCAD."
${LangFileString} UnReallyRemoveLabel "Czy na pewno chcesz usunąć FreeCAD$\'a i wszystkie jego komponenty?"
${LangFileString} UnFreeCADPreferencesTitle 'Preferencje użytkownika FreeCAD$\'a'

#${LangFileString} SecUnProgDescription "Deinstalacja xxx."
${LangFileString} SecUnPreferencesDescription 'Usuwa folder konfiguracji FreeCAD$\'a$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						wszystkim użytkownikom.'
${LangFileString} DialogUnPreferences 'You chose to delete the FreeCADs user configuration.$\r$\n\
						This will also delete all installed FreeCAD addons.$\r$\n\
						Do you agree with this?'
${LangFileString} SecUnProgramFilesDescription "Deinstalacja FreeCAD i wszystkich jego komponentów."
