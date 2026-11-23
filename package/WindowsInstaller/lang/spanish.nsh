/*
FreeCAD Installer Language File
Language: Spanish
*/

!insertmacro LANGFILE_EXT "Spanish"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Instalado para el actual usuario)"

${LangFileString} TEXT_WELCOME "Este programa instalará $(^NameDA) en su ordenador.$\r$\n\
				$\r$\n\
				$_CLICK"

${LangFileString} TEXT_FINISH_WEBSITE "Visite freecad.org para últimas noticias, ayuda y consejos"

${LangFileString} SecFileAssocTitle "Asociar ficheros"
${LangFileString} SecDesktopTitle "Icono de escritorio"

${LangFileString} SecCoreDescription "Los ficheros de FreeCAD."
${LangFileString} SecFileAssocDescription "Asociar la extensión .FCStd con FreeCAD."
${LangFileString} SecDesktopDescription "Crear un icono de FreeCAD en el escritorio."

${LangFileString} AlreadyInstalled "¡FreeCAD ${APP_SERIES_KEY2} ya está instalado!$\r$\n\
				Aún así, ¿quiere instalar FreeCAD sobre la versión existente?"
${LangFileString} NewerInstalled "Está tratando de instalar una versión de FreeCAD más antigua que la que tiene instalada.$\r$\n\
				  Si realmente lo desea, debe desinstalar antes la versión de FreeCAD instalada $OldVersionNumber."

${LangFileString} FinishPageRun "Ejecutar FreeCAD"

${LangFileString} UnNotInRegistryLabel "Imposible encontrar FreeCAD en el registro.$\r$\n\
					Los accesos rápidos del escritorio y del Menú de Inicio no serán eliminados."
${LangFileString} UnInstallRunning "Antes cierre FreeCAD!"
${LangFileString} UnNotAdminLabel "Necesita privilegios de administrador para desinstalar FreeCAD!"
${LangFileString} UnReallyRemoveLabel "¿Está seguro de que desea eliminar completamente FreeCAD y todos sus componentes?"
${LangFileString} UnFreeCADPreferencesTitle 'Preferencias de usuario de FreeCAD'

${LangFileString} SecUnPreferencesDescription 'Elimina las carpetas de configuración de FreeCAD$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						de todos los usuarios.'
${LangFileString} DialogUnPreferences 'Eligió eliminar la configuración de usuario de FreeCAD.$\r$\n\
						Esto también eliminará todos los addons de FreeCAD instalados.$\r$\n\
						¿Está de acuerdo con esto?'
${LangFileString} SecUnProgramFilesDescription "Desinstala FreeCAD y todos sus componentes."

${LangFileString} DirNotEmptyWarning "La carpeta seleccionada '$INSTDIR' no está vacia. Elija una carpeta vacía."
