/*
FreeCAD Installer Language File
Language: Spanish
*/

!insertmacro LANGFILE_EXT "Spanish"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Instalado para el actual usuario)"

${LangFileString} TEXT_WELCOME "Este programa instalará FreeCAD en su ordenador.$\r$\n\
				$\r$\n\
				$_CLICK"

#${LangFileString} TEXT_CONFIGURE_PYTHON "Compilando guiones Python..."

${LangFileString} TEXT_FINISH_DESKTOP "Crear acceso directo en el escritorio"
${LangFileString} TEXT_FINISH_WEBSITE "Visite freecadweb.org para últimas noticias, ayuda y consejos"

#${LangFileString} FileTypeTitle "Documento FreeCAD"

#${LangFileString} SecAllUsersTitle "Instalar para todos los usuarios"
${LangFileString} SecFileAssocTitle "Asociar ficheros"
${LangFileString} SecDesktopTitle "Icono de escritorio"

${LangFileString} SecCoreDescription "Los ficheros de FreeCAD."
#${LangFileString} SecAllUsersDescription "Instalar FreeCAD para todos los usuarios o sólo para el usuario actual."
${LangFileString} SecFileAssocDescription "Asociar la extensión .FCStd con FreeCAD."
${LangFileString} SecDesktopDescription "Crear un icono de FreeCAD en el escritorio."
#${LangFileString} SecDictionaries "Diccionarios"
#${LangFileString} SecDictionariesDescription "Diccionarios de revisión ortográfica que se pueden descargar e instalar."

#${LangFileString} PathName 'Ruta al fichero $\"xxx.exe$\"'
#${LangFileString} InvalidFolder 'Imposible encontrar $\"xxx.exe$\".'

#${LangFileString} DictionariesFailed 'La descarga del diccionario para el idioma $\"$R3$\" ha fallado.'

#${LangFileString} ConfigInfo "La siguiente configuración de FreeCAD va a tardar un poco."

#${LangFileString} RunConfigureFailed "Error al intentar ejecutar el programa de configuración"
${LangFileString} InstallRunning "El instalador ya está siendo ejecutado!"
${LangFileString} AlreadyInstalled "¡FreeCAD ${APP_SERIES_KEY2} ya está instalado!$\r$\n\
				Se recomienda no instalar sobre una instalación existente$\r$\n\
				si la versión instalada es de prueba o da problemas.$\r$\n\
				En estos casos es mejor reinstalar FreeCAD.$\r$\n\
				Aún así, ¿quiere instalar FreeCAD sobre la versión existente?"
${LangFileString} NewerInstalled "Está tratando de instalar una versión de FreeCAD más antigua que la que tiene instalada.$\r$\n\
				  Si realmente lo desea, debe desinstalar antes la versión de FreeCAD instalada $OldVersionNumber."

#${LangFileString} FinishPageMessage "¡Enhorabuena! FreeCAD ha sido instalado con éxito.$\r$\n\
#					$\r$\n\
#					(El primer arranque de FreeCAD puede tardar algunos segundos.)"
${LangFileString} FinishPageRun "Ejecutar FreeCAD"

${LangFileString} UnNotInRegistryLabel "Imposible encontrar FreeCAD en el registro.$\r$\n\
					Los accesos rápidos del escritorio y del Menú de Inicio no serán eliminados."
${LangFileString} UnInstallRunning "Antes cierre FreeCAD!"
${LangFileString} UnNotAdminLabel "Necesita privilegios de administrador para desinstalar FreeCAD!"
${LangFileString} UnReallyRemoveLabel "¿Está seguro de que desea eliminar completamente FreeCAD y todos sus componentes?"
${LangFileString} UnFreeCADPreferencesTitle 'Preferencias de usuario de FreeCAD'

#${LangFileString} SecUnProgDescription "Desinstala xxx."
${LangFileString} SecUnPreferencesDescription 'Elimina las carpetas de configuración de FreeCAD$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						de todos los usuarios.'
${LangFileString} DialogUnPreferences 'Eligió eliminar la configuración de usuario de FreeCAD.$\r$\n\
						Esto también eliminará todos los addons de FreeCAD instalados.$\r$\n\
						¿Está de acuerdo con esto?'
${LangFileString} SecUnProgramFilesDescription "Desinstala FreeCAD y todos sus componentes."
