/*
FreeCAD Installer Language File
Language: Ukrainian
*/

!insertmacro LANGFILE_EXT "Ukrainian"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Встановлено для поточного користувача)"

${LangFileString} TEXT_WELCOME "За допомогою цього майстра ви зможете встановити $(^NameDA) у вашу систему.$\r$\n\
				$\r$\n\
				$_CLICK"

${LangFileString} TEXT_FINISH_WEBSITE "Відвідати freecad.org, щоб ознайомитися з новинами, довідковими матеріалами та підказками"

${LangFileString} SecFileAssocTitle "Прив’язка файлів"
${LangFileString} SecDesktopTitle "Піктограма стільниці"

${LangFileString} SecCoreDescription "Файли FreeCAD."
${LangFileString} SecFileAssocDescription "Файли з суфіксом .FCStd автоматично відкриватимуться за допомогою FreeCAD."
${LangFileString} SecDesktopDescription "Піктограма FreeCAD на стільниці."

${LangFileString} AlreadyInstalled "FreeCAD ${APP_SERIES_KEY2} вже встановлено!$\r$\n\
				Чи хочете ви попри ці зауваження встановити FreeCAD на місце наявної версії?"
${LangFileString} NewerInstalled "Ви намагаєтеся встановити версію FreeCAD, яка є застарілою порівняно з вже встановленою.$\r$\n\
				  Якщо ви хочете встановити застарілу версію, вам слід спочатку вилучити вже встановлений FreeCAD $OldVersionNumber."

${LangFileString} FinishPageRun "Запустити FreeCAD"

${LangFileString} UnNotInRegistryLabel "Не вдалося знайти записи FreeCAD у регістрі.$\r$\n\
					Записи на стільниці і у меню запуску вилучено не буде."
${LangFileString} UnInstallRunning "Спочатку слід завершити роботу програми FreeCAD!"
${LangFileString} UnNotAdminLabel "Для вилучення FreeCAD вам слід мати привілеї адміністратора!"
${LangFileString} UnReallyRemoveLabel "Ви справді бажаєте повністю вилучити FreeCAD і всі його компоненти?"
${LangFileString} UnFreeCADPreferencesTitle 'Параметри FreeCAD, встановлені користувачем'

${LangFileString} SecUnPreferencesDescription 'Вилучає теку з налаштуваннями FreeCAD$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						для всіх користувачів.'
${LangFileString} SecUnProgramFilesDescription "Вилучити FreeCAD і всі його компоненти."
