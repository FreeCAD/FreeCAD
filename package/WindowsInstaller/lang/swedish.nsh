/*
FreeCAD Installer Language File
Language: Swedish
*/

!insertmacro LANGFILE_EXT "Swedish"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Installerad för aktuell användare)"

${LangFileString} TEXT_WELCOME "Denna guide tar dig igenom installationen av $(^NameDA), $\r$\n\
				$\r$\n\
				$_CLICK"

${LangFileString} TEXT_FINISH_WEBSITE "Besök freecad.org för de senaste nyheterna, support och tips"

${LangFileString} SecFileAssocTitle "Filassociationer"
${LangFileString} SecDesktopTitle "Skrivbordsikon"

${LangFileString} SecCoreDescription "FreeCAD-filerna."
${LangFileString} SecFileAssocDescription "Filer med ändelsen .FCStd kommer att automatiskt öppnas i FreeCAD."
${LangFileString} SecDesktopDescription "En FreeCAD-ikon på skrivbordet."

${LangFileString} AlreadyInstalled "FreeCAD ${APP_SERIES_KEY2} är redan installerad!$\r$\n\
				Vill du ändå installera FreeCAD över den nuvarande versionen?"
${LangFileString} NewerInstalled "Du försöker att installera en äldre version av FreeCAD än vad du har installerad.$\r$\n\
				  Om du verkligen vill detta måste du avinstallera den befintliga FreeCAD $OldVersionNumber innan."

${LangFileString} FinishPageRun "Kör FreeCAD"

${LangFileString} UnNotInRegistryLabel "Kan inte hitta FreeCAD i registret.$\r$\n\
					Genvägar på skrivbordet och i startmenyn kommer inte att tas bort."
${LangFileString} UnInstallRunning "Du måste stänga FreeCAD först!"
${LangFileString} UnNotAdminLabel "Du måste ha administratörsbehörighet för att avinstallera FreeCAD!"
${LangFileString} UnReallyRemoveLabel "Är du säker på att du verkligen vill fullständigt ta bort FreeCAD och alla dess komponenter?"
${LangFileString} UnFreeCADPreferencesTitle 'FreeCAD-användarinställningar'

${LangFileString} SecUnPreferencesDescription 'Raderar FreeCAD-konfiguration$\r$\n\
						(katalog $\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						för dig eller för alla användare (om du är admin).'
${LangFileString} SecUnProgramFilesDescription "Avinstallera FreeCAD och alla dess komponenter."
