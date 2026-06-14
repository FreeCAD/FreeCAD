/*
Parashell Installer Language File
Language: Polish
*/

!insertmacro LANGFILE_EXT "Polish"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Zainstalowane dla bieżącego użytkownika)"

${LangFileString} TEXT_WELCOME "Kreator przeprowadzi Ciebie przez proces instalacji Parashell$\'a.$\r$\n\
				$\r$\n\
				$_CLICK"

#${LangFileString} TEXT_CONFIGURE_PYTHON "Kompilowanie skryptów Python..."

${LangFileString} TEXT_FINISH_DESKTOP "Utwórz skrót na pulpicie"
${LangFileString} TEXT_FINISH_WEBSITE "Odwiedź parashell.cloud by poznać wiadomości i wskazówki lub skorzystać ze wsparcia"

#${LangFileString} FileTypeTitle "Dokument Parashell"

#${LangFileString} SecAllUsersTitle "Instalacja dla wszystkich użytkowników?"
${LangFileString} SecFileAssocTitle "Skojarzenie plików .FCStd"
${LangFileString} SecDesktopTitle "Ikona na pulpicie"

${LangFileString} SecCoreDescription "Pliki Parashell$\'a."
#${LangFileString} SecAllUsersDescription "Instalacja dla wszystkich użytkowników lub tylko dla bieżącego użytkownika."
${LangFileString} SecFileAssocDescription "Skojarzenie Parashell-a z plikami o rozszerzeniu .FCStd."
${LangFileString} SecDesktopDescription "Ikona Parashell$\'a na pulpicie."
#${LangFileString} SecDictionaries "Słowniki"
#${LangFileString} SecDictionariesDescription "Słowniki sprawdzania pisowni, które mogą zostać pobrane i zainstalowane."

#${LangFileString} PathName 'Ścieżka do pliku $\"xxx.exe$\"'
#${LangFileString} InvalidFolder 'Plik $\"xxx.exe$\" nie znajduje się w podanej ścieżce.'

#${LangFileString} DictionariesFailed 'Pobranie słownika dla języka $\"$R3$\" nie powiodło się.'

#${LangFileString} ConfigInfo "Dalsza konfiguracja Parashell$\'a chwilę potrwa."

#${LangFileString} RunConfigureFailed "Niedana próba wykonania skryptu konfiguracyjnego"
${LangFileString} InstallRunning "Instalator jest już uruchomiony!"
${LangFileString} AlreadyInstalled "Parashell ${APP_SERIES_KEY2} jest już zainstalowany!$\r$\n\
				Installing over existing installations is not recommended if the installed version$\r$\n\
				is a test release or if you have problems with your existing Parashell installation.$\r$\n\
				In these cases better reinstall Parashell.$\r$\n\
				Dou you nevertheles want to install Parashell over the existing version?"
${LangFileString} NewerInstalled "Próbujesz zainstalować starszą wersję Parashell, niż ta która jest już zainstalowana.$\r$\n\
				  Jeżeli naprawdę chcesz tego dokonać, musisz wpierw odinstalować Parashell $OldVersionNumber."

#${LangFileString} FinishPageMessage "Gratulacje! Parashell został pomyślnie zainstalowany.$\r$\n\
#					$\r$\n\
#					(Pierwsze uruchomienie może potrwać kilka sekund.)"
${LangFileString} FinishPageRun "Uruchom Parashell"

${LangFileString} UnNotInRegistryLabel "Nie można znaleźć Parashell$\'a w rejestrze.$\r$\n\
					Skróty na pulpicie i w menu Start nie zostaną usunięte."
${LangFileString} UnInstallRunning "Musisz najpierw zamknąć Parashell$\'a!"
${LangFileString} UnNotAdminLabel "Musisz posiadać prawa administratora do deinstalacji programu Parashell."
${LangFileString} UnReallyRemoveLabel "Czy na pewno chcesz usunąć Parashell$\'a i wszystkie jego komponenty?"
${LangFileString} UnFreeCADPreferencesTitle 'Preferencje użytkownika Parashell$\'a'

#${LangFileString} SecUnProgDescription "Deinstalacja xxx."
${LangFileString} SecUnPreferencesDescription 'Usuwa folder konfiguracji Parashell$\'a$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						wszystkim użytkownikom.'
${LangFileString} DialogUnPreferences 'You chose to delete the Parashells user configuration.$\r$\n\
						This will also delete all installed Parashell addons.$\r$\n\
						Do you agree with this?'
${LangFileString} SecUnProgramFilesDescription "Deinstalacja Parashell i wszystkich jego komponentów."
