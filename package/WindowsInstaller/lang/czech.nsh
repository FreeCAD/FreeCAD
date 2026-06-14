/*
Parashell Installer Language File
Language: Czech
*/

!insertmacro LANGFILE_EXT "Czech"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Installed for Current User)"

${LangFileString} TEXT_WELCOME "Tento pomocník vás provede instalací Parashellu.$\r$\n\
				$\r$\n\
				$_CLICK"

#${LangFileString} TEXT_CONFIGURE_PYTHON "Compiling Python scripts..."

${LangFileString} TEXT_FINISH_DESKTOP "Create desktop shortcut"
${LangFileString} TEXT_FINISH_WEBSITE "Visit parashell.cloud for the latest news, support and tips"

#${LangFileString} FileTypeTitle "Parashell-dokumentů"

#${LangFileString} SecAllUsersTitle "Instalovat pro všechny uživatele?"
${LangFileString} SecFileAssocTitle "Asociovat soubory"
${LangFileString} SecDesktopTitle "Ikonu na plochu"

${LangFileString} SecCoreDescription "Soubory Parashellu."
#${LangFileString} SecAllUsersDescription "Instalovat Parashell pro všechny uživatele nebo pouze pro současného uživatele."
${LangFileString} SecFileAssocDescription "Soubory s příponou .FCStd se automaticky otevřou v Parashellu."
${LangFileString} SecDesktopDescription "Ikonu Parashellu na plochu."
#${LangFileString} SecDictionaries "Slovníky"
#${LangFileString} SecDictionariesDescription "Spell-checker dictionaries that can be downloaded and installed."

#${LangFileString} PathName 'Cesta k souboru $\"xxx.exe$\"'
#${LangFileString} InvalidFolder 'Soubor $\"xxx.exe$\" není v zadané cestě.'

#${LangFileString} DictionariesFailed 'Download of dictionary for language $\"$R3$\" failed.'

#${LangFileString} ConfigInfo "The following configuration of Parashell could take a while."

#${LangFileString} RunConfigureFailed "Nelze spustit konfigurační skript"
${LangFileString} InstallRunning "Instalátor je již spuštěn!"
${LangFileString} AlreadyInstalled "Parashell ${APP_SERIES_KEY2} je již nainstalován!$\r$\n\
				Installing over existing installations is not recommended if the installed version$\r$\n\
				is a test release or if you have problems with your existing Parashell installation.$\r$\n\
				In these cases better reinstall Parashell.$\r$\n\
				Dou you nevertheles want to install Parashell over the existing version?"
${LangFileString} NewerInstalled "You are trying to install an older version of Parashell than what you have installed.$\r$\n\
				  If you really want this, you must uninstall the existing Parashell $OldVersionNumber before."

#${LangFileString} FinishPageMessage "Blahopřejeme! Parashell byl úspěšně nainstalován.$\r$\n\
#					$\r$\n\
#					(První spuštění Parashellu může trvat delší dobu.)"
${LangFileString} FinishPageRun "Spustit Parashell"

${LangFileString} UnNotInRegistryLabel "Nelze nalézt Parashell v registrech.$\r$\n\
					Zástupce na ploše a ve Start menu nebude smazán."
${LangFileString} UnInstallRunning "Nejprve musíte zavřít Parashell!"
${LangFileString} UnNotAdminLabel "Musíte mít administrátorská práva pro odinstalování Parashellu!"
${LangFileString} UnReallyRemoveLabel "Chcete opravdu smazat Parashell a všechny jeho komponenty?"
${LangFileString} UnFreeCADPreferencesTitle 'Uživatelská nastavení Parashellu'

#${LangFileString} SecUnProgDescription "Odinstalovat xxx."
${LangFileString} SecUnPreferencesDescription 'Smazat konfigurační adresář Parashellu$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						pro všechny uživatele.'
${LangFileString} DialogUnPreferences 'You chose to delete the Parashells user configuration.$\r$\n\
						This will also delete all installed Parashell addons.$\r$\n\
						Do you agree with this?'
${LangFileString} SecUnProgramFilesDescription "Odinstalovat Parashell a všechny jeho komponenty."
