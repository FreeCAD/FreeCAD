/*
FreeCAD Installer Language File
Language: Arabic
*/

!insertmacro LANGFILE_EXT "Arabic"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(تركيب للمستخدم الحالي)"

${LangFileString} TEXT_WELCOME "هذا المساعد سوف يرشدك خلال تركيب $(^NameDA), $\r$\n\
				$\r$\n\
				$_CLICK"

#${LangFileString} TEXT_CONFIGURE_PYTHON "بناء سكربتات بايثون..."

${LangFileString} TEXT_FINISH_DESKTOP "إنشاء اختصار سطح المكتب"
${LangFileString} TEXT_FINISH_WEBSITE "زيارة freecadweb.org لمشاهدة آخر الاخبار, الدعم والأفكار"

#${LangFileString} FileTypeTitle "مستند - ليك"

#${LangFileString} SecAllUsersTitle "تركيب لكل المستخدمين؟"
${LangFileString} SecFileAssocTitle "اقتران الملف"
${LangFileString} SecDesktopTitle "رمز سطح المكتب"

${LangFileString} SecCoreDescription "ملفات ليك."
#${LangFileString} SecAllUsersDescription "تركيب ليك لهذا المستخدم أم لجميع المستخدمين."
${LangFileString} SecFileAssocDescription "الملفات بلاحقة .FCStd سوف تفتح تلفائيا ببرنامج ليك."
${LangFileString} SecDesktopDescription "رمز ليم على سطح المكتب."
#${LangFileString} SecDictionaries "قواميس"
#${LangFileString} SecDictionariesDescription "قواميس المدقق الإملائي التي يمكن تنزيلها وتركيبها."

#${LangFileString} PathName 'مسار الملف $\"xxx.exe$\"'
#${LangFileString} InvalidFolder 'الملف $\"xxx.exe$\" ليس في المسار المحدد.'

#${LangFileString} DictionariesFailed 'فشل تنزيل قاموس اللغة $\"$R3$\" .'

#${LangFileString} ConfigInfo "ضبط ليك سيستغرق وفت."

#${LangFileString} RunConfigureFailed "لم ينفذ سكريبت الضبط"
${LangFileString} InstallRunning "المركب يعمل حاليا!"
${LangFileString} AlreadyInstalled "ليك ${APP_SERIES_KEY2} تم تركيبه بالفعل!$\r$\n\
				التركيب على النسخة الحالية غير مفضل إذا كانت النسخة الحالية$\r$\n\
				تجريبية أو بها مشاكل.$\r$\n\
				في هذه الحالة من الأفضل إعادة التركيب.$\r$\n\
				هل تريد بالرغم من ذلك تركيب ليك على النسخة الحالية؟"
${LangFileString} NewerInstalled "تحاول تركيب نسخة ليك أقدم من الموجودة حاليا.$\r$\n\
				  إذا كنت تريدها بالتأكيد, عليك حذف النسخة الحالية $OldVersionNumber أولا."

#${LangFileString} FinishPageMessage "مبروك! تم تركيب ليك بنجاح.$\r$\n\
#					$\r$\n\
#					(البدء الأول لليك ربما يستغرق ثوان.)"
${LangFileString} FinishPageRun "بدء ليك"

${LangFileString} UnNotInRegistryLabel "لم يتم العثور على ليك في سجل النظام.$\r$\n\
					إختصارات سطح المكتب وقائمة البدء لم يتم حذفها."
${LangFileString} UnInstallRunning "يجب إغلاق ليك أولا!"
${LangFileString} UnNotAdminLabel "يجب أن يكون لديك صلاحيات المدير لكي تحذف ليك!"
${LangFileString} UnReallyRemoveLabel "هل ترغب بإزالة ليك مع كل مكوناته؟"
${LangFileString} UnFreeCADPreferencesTitle 'تفضيلات مستخدم ليك'

#${LangFileString} SecUnProgDescription "إزالة مدير ثبت المراجع xxx."
${LangFileString} SecUnPreferencesDescription 'حذف FreeCAD$\'s ضبط$\r$\n\
						(مجلد $\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						لك او لكل المستخدمين (إذا كنت المدير).'
${LangFileString} DialogUnPreferences 'You chose to delete the FreeCADs user configuration.$\r$\n\
						This will also delete all installed FreeCAD addons.$\r$\n\
						Do you agree with this?'
${LangFileString} SecUnProgramFilesDescription "إزالة ليك مع كل مكوناته."
