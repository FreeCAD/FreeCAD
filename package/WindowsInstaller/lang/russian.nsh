/*
FreeCAD Installer Language File
Language: Russian
*/

!insertmacro LANGFILE_EXT "Russian"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Установлено для текущего пользователя)"

${LangFileString} TEXT_WELCOME "Этот мастер проведет вас через процесс установки $(^NameDA). $\r$\n\
				$\r$\n\
				$_CLICK"

${LangFileString} TEXT_FINISH_WEBSITE "Перейти на freecad.org за новостями, поддержкой и советами"

${LangFileString} SecFileAssocTitle "Ассоциации файлов"
${LangFileString} SecDesktopTitle "Значок на рабочем столе"

${LangFileString} SecCoreDescription "Файлы FreeCAD."
${LangFileString} SecFileAssocDescription "Файлы с расширением .FCStd будут автоматически открываться в FreeCAD."
${LangFileString} SecDesktopDescription "Значок FreeCAD на рабочем столе."

${LangFileString} AlreadyInstalled "FreeCAD ${APP_SERIES_KEY2} уже установлен!$\r$\n\
				Вы все равно хотите установить FreeCAD поверх существующей версии?"
${LangFileString} NewerInstalled "Вы пытаетесь установить более старую версию FreeCAD, чем уже установленная.$\r$\n\
				  Если вы действительно хотите этого, то сначала необходимо удалить существующий FreeCAD $OldVersionNumber."

${LangFileString} FinishPageRun "Запустить FreeCAD"

${LangFileString} UnNotInRegistryLabel "Не удалось найти FreeCAD в реестре.$\r$\n\
					Ярлыки на рабочем столе и в меню Пуск не будут удалены."
${LangFileString} UnInstallRunning "Вы должны сначала закрыть FreeCAD!"
${LangFileString} UnNotAdminLabel "Необходимо иметь права администратора для удаления FreeCAD!"
${LangFileString} UnReallyRemoveLabel "Вы действительно хотите полностью удалить FreeCAD и все его компоненты?"
${LangFileString} UnFreeCADPreferencesTitle 'Пользовательские настройки FreeCAD'

${LangFileString} SecUnPreferencesDescription 'Удалить настройки FreeCAD$\r$\n\
						(каталог $\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						для вас или для всех пользователей (если вы администратор).'
${LangFileString} SecUnProgramFilesDescription "Удалить FreeCAD и все его компоненты."
