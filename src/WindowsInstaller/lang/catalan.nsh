/*
FreeCAD Installer Language File
Language: Catalan
*/

!insertmacro LANGFILE_EXT "Catalan"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Installed for Current User)"

${LangFileString} TEXT_WELCOME "Aquest assistent us guiarà en la instal·lació del FreeCAD.$\r$\n\
				$\r$\n\
				$_CLICK"

#${LangFileString} TEXT_CONFIGURE_PYTHON "Compiling Python scripts..."

${LangFileString} TEXT_FINISH_DESKTOP "Create desktop shortcut"
${LangFileString} TEXT_FINISH_WEBSITE "Visit freecadweb.org for the latest news, support and tips"

#${LangFileString} FileTypeTitle "Document FreeCAD"

#${LangFileString} SecAllUsersTitle "Voleu instal·lar-ho per a tots els usuaris?"
${LangFileString} SecFileAssocTitle "Associació de fitxers"
${LangFileString} SecDesktopTitle "Icona a l'escriptori"

${LangFileString} SecCoreDescription "Els fitxers del FreeCAD."
#${LangFileString} SecAllUsersDescription "Instal·la el FreeCAD per a tots els usuaris o només per a l'usuari actual."
${LangFileString} SecFileAssocDescription "Els fitxers amb extensió .FCStd s'obriran automàticament amb el FreeCAD."
${LangFileString} SecDesktopDescription "Una icona del FreeCAD a l'escriptori."
#${LangFileString} SecDictionaries "Diccionaris"
#${LangFileString} SecDictionariesDescription "Spell-checker dictionaries that can be downloaded and installed."

#${LangFileString} PathName 'Camí al fitxer $\"xxx.exe$\"'
#${LangFileString} InvalidFolder 'El fitxer $\"xxx.exe$\" no es troba al camí indicat.'

#${LangFileString} DictionariesFailed 'Download of dictionary for language $\"$R3$\" failed.'

#${LangFileString} ConfigInfo "La configuració següent del FreeCAD pot trigar una mica."

#${LangFileString} RunConfigureFailed "No es pot executar el programa de configuració"
${LangFileString} InstallRunning "L'instal·lador ja s'està executant!"
${LangFileString} AlreadyInstalled "El FreeCAD ${APP_SERIES_KEY2} ja es troba instal·lat!$\r$\n\
				Installing over existing installations is not recommended if the installed version$\r$\n\
				is a test release or if you have problems with your existing FreeCAD installation.$\r$\n\
				In these cases better reinstall FreeCAD.$\r$\n\
				Dou you nevertheles want to install FreeCAD over the existing version?"
${LangFileString} NewerInstalled "You are trying to install an older version of FreeCAD than what you have installed.$\r$\n\
				  If you really want this, you must uninstall the existing FreeCAD $OldVersionNumber before."

#${LangFileString} FinishPageMessage "Felicitats! Heu instal·lat correctament el FreeCAD.$\r$\n\
#					$\r$\n\
#					(La primera execució del FreeCAD pot trigar alguns segons.)"
${LangFileString} FinishPageRun "Executa el FreeCAD"

${LangFileString} UnNotInRegistryLabel "No es possible trobar el FreeCAD al registre.$\r$\n\
					No se suprimiran les dreceres de l'escriptori i del menú inici."
${LangFileString} UnInstallRunning "Primer heu de tancar el FreeCAD!"
${LangFileString} UnNotAdminLabel "Necessiteu drets d'administrador per desinstal·lar el FreeCAD!"
${LangFileString} UnReallyRemoveLabel "Esteu segur de voler suprimir completament el FreeCAD i tots els seus components?"
${LangFileString} UnFreeCADPreferencesTitle 'Preferències d$\'usuari del FreeCAD'

#${LangFileString} SecUnProgDescription "Desinstal·xxx."
${LangFileString} SecUnPreferencesDescription 'Suprimeix les carptes de configuració del FreeCAD$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						de tots els usuaris.'
${LangFileString} DialogUnPreferences 'You chose to delete the FreeCADs user configuration.$\r$\n\
						This will also delete all installed FreeCAD addons.$\r$\n\
						Do you agree with this?'
${LangFileString} SecUnProgramFilesDescription "Desinstal·la el FreeCAD i tots els seus components."
