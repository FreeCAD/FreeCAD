/*
FreeCAD Installer Language File
Language: German
Author: Uwe Stöhr
*/

!insertmacro LANGFILE_EXT "German"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Installiert für den aktuellen Benutzer)"

${LangFileString} TEXT_WELCOME "Dieser Assistent wird Sie durch die Installation von $(^NameDA) begleiten.$\r$\n\
				$\r$\n\
				$_CLICK"

${LangFileString} TEXT_FINISH_WEBSITE "Besuchen Sie freecad.org für aktuelle Neuigkeiten"

${LangFileString} SecFileAssocTitle "Dateizuordnungen"
${LangFileString} SecDesktopTitle "Desktopsymbol"

${LangFileString} SecCoreDescription "Das Programm FreeCAD."
${LangFileString} SecFileAssocDescription "Vernüpfung zwischen FreeCAD und der .FCStd Dateiendung."
${LangFileString} SecDesktopDescription "Verknüpfung zu FreeCAD auf dem Desktop."

${LangFileString} AlreadyInstalled "FreeCAD ${APP_SERIES_KEY2} ist bereits installiert!$\r$\n\
				Wollen Sie FreeCAD dennoch über die bestehende Version installieren?"
${LangFileString} NewerInstalled "Sie versuchen eine Vesion von FreeCAD zu installieren, die älter als die derzeit installierte ist.$\r$\n\
				  Wenn Sie das wirklich wollen, müssen Sie erst das existierende FreeCAD $OldVersionNumber deinstallieren."

${LangFileString} FinishPageRun "FreeCAD starten"

${LangFileString} UnNotInRegistryLabel "Kann FreeCAD nicht in der Registry finden.$\r$\n\
					Desktopsymbole und Einträge im Startmenü können nicht entfernt werden."
${LangFileString} UnInstallRunning "Sie müssen FreeCAD zuerst beenden!"
${LangFileString} UnNotAdminLabel "Sie benötigen Administratorrechte um FreeCAD zu deinstallieren!"
${LangFileString} UnReallyRemoveLabel "Sind Sie sicher, dass sie FreeCAD und all seine Komponenten deinstallieren möchten?"
${LangFileString} UnFreeCADPreferencesTitle 'FreeCADs Benutzereinstellungen'

${LangFileString} SecUnPreferencesDescription 'Löscht FreeCADs Benutzereinstellungen$\r$\n\
						(Ordner $\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						für Sie oder für alle Benutzer (wenn Sie Admin sind).'
${LangFileString} DialogUnPreferences 'Sie haben ausgewählt, die FreeCAD-Benutzereinstellungen zu löschen.$\r$\n\
						Dies wird auch alle installierten FreeCAD-Addons löschen.$\r$\n\
						Sind Sie damit einverstanden?'
${LangFileString} SecUnProgramFilesDescription "Deinstalliert FreeCAD und all seine Komponenten."
