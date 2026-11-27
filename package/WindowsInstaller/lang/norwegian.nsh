/*
FreeCAD Installer Language File
Language: Norwegian
*/

!insertmacro LANGFILE_EXT "Norwegian"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Installer for denne brukeren)"

${LangFileString} TEXT_WELCOME "Denne veiviseren installerer $(^NameDA) på datamaskinen din.$\r$\n\
				$\r$\n\
				$_CLICK"

${LangFileString} TEXT_FINISH_WEBSITE "Besøk freecad.org for de seneste nyhetene, hjelp og støtte"

${LangFileString} SecFileAssocTitle "Fil-assosiasjoner"
${LangFileString} SecDesktopTitle "Skrivebordsikon"

${LangFileString} SecCoreDescription "FreeCAD-filene."
${LangFileString} SecFileAssocDescription "Filer med endelsen .FCStd åpnes automatisk i FreeCAD."
${LangFileString} SecDesktopDescription "Et FreeCAD-ikon på skrivebordet."

${LangFileString} AlreadyInstalled "FreeCAD ${APP_SERIES_KEY2} er allerede installert!$\r$\n\
				Vil du likevel installere FreeCAD over den eksisterende versjonen?"
${LangFileString} NewerInstalled "Du prøver å installere en eldre versjon av FreeCAD enn den du har installert fra før.$\r$\n\
				  Dersom du ønsker dette må du avinstallere FreeCAD $OldVersionNumber først."

${LangFileString} FinishPageRun "Start FreeCAD"

${LangFileString} UnNotInRegistryLabel "Fant ikke FreeCAD i registeret.$\r$\n\
					Snarveier på skrivebordet og i startmenyen fjernes ikke."
${LangFileString} UnInstallRunning "Du må avslutte FreeCAD først!"
${LangFileString} UnNotAdminLabel "Du må ha administratorrettigheter for å fjerne FreeCAD!"
${LangFileString} UnReallyRemoveLabel "Er du sikker på at du vil fjerne FreeCAD og alle tilhørende komponenter?"
${LangFileString} UnFreeCADPreferencesTitle 'FreeCAD sine bruker innstillinger'

${LangFileString} SecUnPreferencesDescription 'Sletter FreeCAD sine konfigurasjonsmapper$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						for alle brukere.'
${LangFileString} SecUnProgramFilesDescription "Avinstallerer FreeCAD og alle delkomponenter."
