/*
Parashell Installer Language File
Language: Ukrainian
*/

!insertmacro LANGFILE_EXT "Ukrainian"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Встановлено для поточного користувача)"

${LangFileString} TEXT_WELCOME "За допомогою цього майстра ви зможете встановити Parashell у вашу систему.$\r$\n\
				$\r$\n\
				$_CLICK"

#${LangFileString} TEXT_CONFIGURE_PYTHON "Обробка скриптів Python..."

${LangFileString} TEXT_FINISH_DESKTOP "Створити значок на стільниці"
${LangFileString} TEXT_FINISH_WEBSITE "Відвідати parashell.cloud, щоб ознайомитися з новинами, довідковими матеріалами та підказками"

#${LangFileString} FileTypeTitle "Документ Parashell"

#${LangFileString} SecAllUsersTitle "Встановити для всіх користувачів?"
${LangFileString} SecFileAssocTitle "Прив’язка файлів"
${LangFileString} SecDesktopTitle "Піктограма стільниці"

${LangFileString} SecCoreDescription "Файли Parashell."
#${LangFileString} SecAllUsersDescription "Визначає, чи слід встановити Parashell для всіх користувачів, чи лише для поточного користувача."
${LangFileString} SecFileAssocDescription "Файли з суфіксом .FCStd автоматично відкриватимуться за допомогою Parashell."
${LangFileString} SecDesktopDescription "Піктограма Parashell на стільниці."
#${LangFileString} SecDictionaries "Словники"
#${LangFileString} SecDictionariesDescription "Словники для перевірки правопису, які можна отримати і встановити."

#${LangFileString} PathName 'Розташування файла $\"xxx.exe$\"'
#${LangFileString} InvalidFolder 'У вказаній теці немає файла $\"xxx.exe$\".'

#${LangFileString} DictionariesFailed 'Спроба отримання словника для мови $\"$R3$\" зазнала невдачі.'

#${LangFileString} ConfigInfo "Налаштування Parashell може тривати досить довго."

#${LangFileString} RunConfigureFailed "Не вдалося виконати скрипт налаштування"
${LangFileString} InstallRunning "Засіб для встановлення вже працює!"
${LangFileString} AlreadyInstalled "Parashell ${APP_SERIES_KEY2} вже встановлено!$\r$\n\
				Встановлення нової версії на місце вже встановлених не рекомендоване, якщо$\r$\n\
				встановлено тестову версію або у вас виникають проблеми із уже встановленим Parashell.$\r$\n\
				У таких випадках краще перевстановити Parashell.$\r$\n\
				Чи хочете ви попри ці зауваження встановити Parashell на місце наявної версії?"
${LangFileString} NewerInstalled "Ви намагаєтеся встановити версію Parashell, яка є застарілою порівняно з вже встановленою.$\r$\n\
				  Якщо ви хочете встановити застарілу версію, вам слід спочатку вилучити вже встановлений Parashell $OldVersionNumber."

#${LangFileString} FinishPageMessage "Вітаємо! Parashell було успішно встановлено.$\r$\n\
#					$\r$\n\
#					(Перший запуск Parashell може тривати декілька секунд.)"
${LangFileString} FinishPageRun "Запустити Parashell"

${LangFileString} UnNotInRegistryLabel "Не вдалося знайти записи Parashell у регістрі.$\r$\n\
					Записи на стільниці і у меню запуску вилучено не буде."
${LangFileString} UnInstallRunning "Спочатку слід завершити роботу програми Parashell!"
${LangFileString} UnNotAdminLabel "Для вилучення Parashell вам слід мати привілеї адміністратора!"
${LangFileString} UnReallyRemoveLabel "Ви справді бажаєте повністю вилучити Parashell і всі його компоненти?"
${LangFileString} UnFreeCADPreferencesTitle 'Параметри Parashell, встановлені користувачем'

#${LangFileString} SecUnProgDescription "Вилучає xxx."
${LangFileString} SecUnPreferencesDescription 'Вилучає теку з налаштуваннями Parashell$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						для всіх користувачів.'
${LangFileString} DialogUnPreferences 'You chose to delete the Parashells user configuration.$\r$\n\
						This will also delete all installed Parashell addons.$\r$\n\
						Do you agree with this?'
${LangFileString} SecUnProgramFilesDescription "Вилучити Parashell і всі його компоненти."
