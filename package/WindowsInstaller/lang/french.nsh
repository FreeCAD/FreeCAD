/*
FreeCAD Installer Language File
Language: French
*/

!insertmacro LANGFILE_EXT "French"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Installation pour l'utilisateur courant)"

${LangFileString} TEXT_WELCOME "Cet assistant va vous guider tout au long de l'installation de $(^NameDA).$\r$\n\
				$\r$\n\
				$_CLICK"

${LangFileString} TEXT_FINISH_WEBSITE "Consulter les dernières nouvelles, trucs et astuces sur le site freecad.org"

${LangFileString} SecFileAssocTitle "Associations de fichiers"
${LangFileString} SecDesktopTitle "Icône du bureau"

${LangFileString} SecCoreDescription "Les fichiers FreeCAD"
${LangFileString} SecFileAssocDescription "Les fichiers de suffixe .FCStd seront automatiquement ouverts dans FreeCAD."
${LangFileString} SecDesktopDescription "Une icône FreeCAD sur le bureau."

${LangFileString} AlreadyInstalled "FreeCAD ${APP_SERIES_KEY2} est déjà installé !$\r$\n\
				Voulez-vous néanmoins installer FreeCAD par dessus la version existante ?"
${LangFileString} NewerInstalled "Vous essayez d$\'installer une version de FreeCAD plus ancienne que celle qui est déjà installée.$\r$\n\
				  Si c$\'est ce qu vous voulez, vous devez d$\'abord désinstaller FreeCAD $OldVersionNumber."

${LangFileString} FinishPageRun "Démarrer FreeCAD"

${LangFileString} UnNotInRegistryLabel "FreeCAD introuvable dans la base des registres.$\r$\n\
					Les raccourcis sur le bureau et dans le menu de démarrage ne seront pas supprimés."
${LangFileString} UnInstallRunning "Vous devez fermer FreeCAD d$\'abord !"
${LangFileString} UnNotAdminLabel "Vous devez avoir les droits d$\'administration pour désinstaller FreeCAD !"
${LangFileString} UnReallyRemoveLabel "Êtes vous sûr(e) de vouloir supprimer complètement FreeCAD et tous ses composants ?"
${LangFileString} UnFreeCADPreferencesTitle 'Préférences utilisateurs de FreeCAD'

${LangFileString} SecUnPreferencesDescription 'Supprime le répertoire de configuration de FreeCAD$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						pour tous les utilisateurs.'
${LangFileString} DialogUnPreferences 'Vous avez choisi de supprimer le répertoire de configuration de FreeCADs.$\r$\n\
						Cela supprimera également tous les addons FreeCAD installés.$\r$\n\
						Êtes-vous d$\'accord avec cela ?'
${LangFileString} SecUnProgramFilesDescription "Désinstaller FreeCAD et tous ses composants."
