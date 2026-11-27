/*
FreeCAD Installer Language File
Language: Brazilian Portuguese
*/

!insertmacro LANGFILE_EXT "PortugueseBR"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(Instalado para o Usuário Atual)"

${LangFileString} TEXT_WELCOME "Este assistente guiará você durante a instalação do $(^NameDA), $\r$\n\
				$\r$\n\
				$_CLICK"

${LangFileString} TEXT_FINISH_WEBSITE "Visite freecad.org para ver as últimas novidades do FreeCAD!"

${LangFileString} SecFileAssocTitle "Associações de arquivos"
${LangFileString} SecDesktopTitle "Ícone de área de trabalho"

${LangFileString} SecCoreDescription "Os arquivos do FreeCAD."
${LangFileString} SecFileAssocDescription "Arquivos com a extensão .FCStd serão abertos automaticamente no FreeCAD."
${LangFileString} SecDesktopDescription "Um ícone do FreeCAD na área de trabalho."

${LangFileString} AlreadyInstalled "O FreeCAD ${APP_SERIES_KEY2} já está instalado!$\r$\n\
				Deseja instalar sobre a versão existente mesmo assim?"
${LangFileString} NewerInstalled "A versão que você está tentando instalar é mais antiga que aquela que já está instalada.$\r$\n\
				  Se isso for realmente o que deseja, primeiro desinstale o FreeCAD $OldVersionNumber."

${LangFileString} FinishPageRun "Executar o FreeCAD"

${LangFileString} UnNotInRegistryLabel "Não foi possível encontrar o FreeCAD no Registro.$\r$\n\
					Os atalhos na área de trabalho e no Menu Iniciar não serão removidos."
${LangFileString} UnInstallRunning "É necessário fechar o FreeCAD primeiro!"
${LangFileString} UnNotAdminLabel "Para desinstalar o FreeCAD é necessário ter privilégios de administrador!"
${LangFileString} UnReallyRemoveLabel "Tem certeza que deseja remover completamente o FreeCAD e todos os seus componentes?"
${LangFileString} UnFreeCADPreferencesTitle 'Preferências de usuário do FreeCAD'

${LangFileString} SecUnPreferencesDescription 'Exclui a configuração do FreeCAD$\r$\n\
						(pasta $\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						para você ou para todos os usuários (se você for um administrador)).'
${LangFileString} SecUnProgramFilesDescription "Desinstalar o FreeCAD e todos os seus componentes."
