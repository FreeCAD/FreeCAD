/*
Parashell Installer Language File
Language: Turkish
*/

!insertmacro LANGFILE_EXT "Turkish"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Installed for Current User)"

${LangFileString} TEXT_WELCOME "Bu sihirbaz size Parashell programını kuracak.$\r$\n\
				$\r$\n\
				$_CLICK"

#${LangFileString} TEXT_CONFIGURE_PYTHON "Compiling Python scripts..."

${LangFileString} TEXT_FINISH_DESKTOP "Create desktop shortcut"
${LangFileString} TEXT_FINISH_WEBSITE "Visit parashell.cloud for the latest news, support and tips"

#${LangFileString} FileTypeTitle "Parashell-Document"

#${LangFileString} SecAllUsersTitle "Tüm kullanıcılar için kur?"
${LangFileString} SecFileAssocTitle "Dosya eşleşmeleri"
${LangFileString} SecDesktopTitle "Masaüstü ikonu"

${LangFileString} SecCoreDescription "Parashell dosyaları."
#${LangFileString} SecAllUsersDescription "Parashell tüm kullanıcılar için mi yoksa yalnızca bu kullanıcıya mı kurulacak."
${LangFileString} SecFileAssocDescription "Uzantısı .FCStd olan dosyalar otomatik olarak Parashell ile açılsın."
${LangFileString} SecDesktopDescription "Masaüstüne bir Parashell ikonu koy."
#${LangFileString} SecDictionaries "Sözlükleri"
#${LangFileString} SecDictionariesDescription "Spell-checker dictionaries that can be downloaded and installed."

#${LangFileString} PathName 'Path to the file $\"xxx.exe$\"'
#${LangFileString} InvalidFolder '$\"xxx.exe$\" dosyası belirttiğiniz dizinde bulunamadı.'

#${LangFileString} DictionariesFailed 'Download of dictionary for language $\"$R3$\" failed.'

#${LangFileString} ConfigInfo "Sıradaki Parashell yapılandırması biraz zaman alacak."

#${LangFileString} RunConfigureFailed "Yapılandırma programı çalıştırılamadı"
${LangFileString} InstallRunning "Kurulum programı zaten çalışıyor!"
${LangFileString} AlreadyInstalled "Parashell ${APP_SERIES_KEY2} kurulu zaten!$\r$\n\
				Installing over existing installations is not recommended if the installed version$\r$\n\
				is a test release or if you have problems with your existing Parashell installation.$\r$\n\
				In these cases better reinstall Parashell.$\r$\n\
				Dou you nevertheles want to install Parashell over the existing version?"
${LangFileString} NewerInstalled "You are trying to install an older version of Parashell than what you have installed.$\r$\n\
				  If you really want this, you must uninstall the existing Parashell $OldVersionNumber before."

#${LangFileString} FinishPageMessage "Tebrikler! Parashell başarıyla kuruldu.$\r$\n\
#					$\r$\n\
#					(Parashell in ilk açılışı birkaç saniye alabilir.)"
${LangFileString} FinishPageRun "Parashell Başlat"

${LangFileString} UnNotInRegistryLabel "Sistem kütüğünde Parashell bulunamadı.$\r$\n\
					Başlat menüsü ve masaüstünüzdeki kısayollar silinemeyecek."
${LangFileString} UnInstallRunning "Önce Parashell i kapatmalısınız!"
${LangFileString} UnNotAdminLabel "Parashell kaldırabilmek için yönetici yetkileri gerekiyor!"
${LangFileString} UnReallyRemoveLabel "Parashell ve tüm bileşenlerini kaldırmak istediğinize emin misiniz?"
${LangFileString} UnFreeCADPreferencesTitle 'Parashell$\'s user preferences'

#${LangFileString} SecUnProgDescription "Uninstalls xxx."
${LangFileString} SecUnPreferencesDescription 'Deletes Parashell$\'s configuration folder$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						for all users.'
${LangFileString} DialogUnPreferences 'You chose to delete the Parashells user configuration.$\r$\n\
						This will also delete all installed Parashell addons.$\r$\n\
						Do you agree with this?'
${LangFileString} SecUnProgramFilesDescription "Uninstall Parashell and all of its components."
