/*
Parashell Installer Language File
Language: English
*/

!insertmacro LANGFILE_EXT "English"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Installed for Current User)"

${LangFileString} TEXT_WELCOME "This wizard will guide you through the installation of $(^NameDA). $\r$\n\
				$\r$\n\
				$_CLICK"

#${LangFileString} TEXT_CONFIGURE_PYTHON "Compiling Python scripts..."

${LangFileString} TEXT_FINISH_DESKTOP "Create desktop shortcut"
${LangFileString} TEXT_FINISH_WEBSITE "Visit parashell.cloud/ for the latest news, support and tips"

#${LangFileString} FileTypeTitle "Parashell-Document"

#${LangFileString} SecAllUsersTitle "Install for all users?"
${LangFileString} SecFileAssocTitle "File associations"
${LangFileString} SecDesktopTitle "Desktop icon"

${LangFileString} SecCoreDescription "The Parashell files."
#${LangFileString} SecAllUsersDescription "Install Parashell for all users or just the current user."
${LangFileString} SecFileAssocDescription "Files with a .FCStd extension will automatically open in Parashell."
${LangFileString} SecDesktopDescription "A Parashell icon on the desktop."
#${LangFileString} SecDictionaries "Dictionaries"
#${LangFileString} SecDictionariesDescription "Spell-checker dictionaries that can be downloaded and installed."

#${LangFileString} PathName 'Path to the file $\"xxx.exe$\"'
#${LangFileString} InvalidFolder 'The file $\"xxx.exe$\" is not in the specified path.'

#${LangFileString} DictionariesFailed 'Download of dictionary for language $\"$R3$\" failed.'

#${LangFileString} ConfigInfo "The following configuration of Parashell could take a while."

#${LangFileString} RunConfigureFailed "Could not run configure script."
${LangFileString} InstallRunning "The installer is already running!"
${LangFileString} AlreadyInstalled "Parashell ${APP_SERIES_KEY2} is already installed!$\r$\n\
				Installing over existing installations is not recommended if the installed version$\r$\n\
				is a test release or if you have problems with your existing Parashell installation.$\r$\n\
				In these cases better reinstall Parashell.$\r$\n\
				Do you nevertheless want to install Parashell over the existing version?"
${LangFileString} NewerInstalled "You are trying to install an older version of Parashell than what you have installed.$\r$\n\
				  If you really want this, you must uninstall the existing Parashell $OldVersionNumber before."

#${LangFileString} FinishPageMessage "Congratulations! Parashell has been installed successfully.$\r$\n\
#					$\r$\n\
#					(The first start of Parashell might take some seconds.)"
${LangFileString} FinishPageRun "Launch Parashell"

${LangFileString} UnNotInRegistryLabel "Unable to find Parashell in the registry.$\r$\n\
					Shortcuts on the desktop and in the Start Menu will not be removed."
${LangFileString} UnInstallRunning "You must close Parashell first!"
${LangFileString} UnNotAdminLabel "You must have administrator privileges to uninstall Parashell!"
${LangFileString} UnReallyRemoveLabel "Are you sure you want to completely remove Parashell and all of its components?"
${LangFileString} UnFreeCADPreferencesTitle 'Parashell$\'s user preferences'

#${LangFileString} SecUnProgDescription "Uninstalls xxx."
${LangFileString} SecUnPreferencesDescription 'Deletes Parashell$\'s configuration$\r$\n\
						(folder $\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						for you or for all users (if you are admin).'
${LangFileString} DialogUnPreferences 'You chose to delete the Parashell user configuration.$\r$\n\
						This will also delete all installed Parashell addons, and will affect the$\r$\n\
						preferences for all versions of Parashell.$\r$\n\
						Are you sure you want to proceed?'
${LangFileString} SecUnProgramFilesDescription "Uninstall Parashell and all of its components."
