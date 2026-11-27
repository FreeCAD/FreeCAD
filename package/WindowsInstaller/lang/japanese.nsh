/*
FreeCAD Installer Language File
Language: Japanese
*/

!insertmacro LANGFILE_EXT "Japanese"

${LangFileString} TEXT_INSTALL_CURRENTUSER "(現ユーザー用に導入を行う)"

${LangFileString} TEXT_WELCOME "このウィザードが、あなたのFreeCAD導入作業中のご案内をします。$\r$\n\
				$\r$\n\
				$_CLICK"

${LangFileString} TEXT_FINISH_WEBSITE "freecad.orgを開いて最新ニュースやサポート、ヒントなどを入手する"

${LangFileString} SecFileAssocTitle "ファイル関連付け"
${LangFileString} SecDesktopTitle "デスクトップ・アイコン"

${LangFileString} SecCoreDescription "FreeCADのファイル。"
${LangFileString} SecFileAssocDescription "拡張子が.FCStdのファイルは自動的にFreeCADで開かれる。"
${LangFileString} SecDesktopDescription "デスクトップ上のFreeCADアイコン"

${LangFileString} AlreadyInstalled "FreeCAD${APP_SERIES_KEY2}は既に導入済みです！$\r$\n\
				これらを承知の上で、既存のFreeCADを上書きしますか？"
${LangFileString} NewerInstalled "あなたは、既に導入済みのFreeCADよりも古い版を導入しようとしています。$\r$\n\
				  本当にそうしたいのであれば、既存の FreeCAD $OldVersionNumber をまず導入解除してください。"

${LangFileString} FinishPageRun "FreeCADを起動する"

${LangFileString} UnNotInRegistryLabel "レジストリにFreeCADが見当たりません。$\r$\n\
					デスクトップとスタートメニューのショートカットは削除されません。"
${LangFileString} UnInstallRunning "まずFreeCADを閉じてください！"
${LangFileString} UnNotAdminLabel "FreeCADの導入解除を行うには、管理者権限を持っていなくてはなりません！"
${LangFileString} UnReallyRemoveLabel "本当に、FreeCADとすべての附属コンポーネントを削除してしまう積もりですか？"
${LangFileString} UnFreeCADPreferencesTitle 'FreeCADのユーザー設定'

${LangFileString} SecUnPreferencesDescription 'ユーザー共通のFreeCADの設定フォルダ$\r$\n\
						$\"$AppPre\username\$\r$\n\
						$AppSuff\$\r$\n\
						${APP_DIR_USERDATA}$\")$\r$\n\
						を削除します。'
${LangFileString} SecUnProgramFilesDescription "FreeCADとすべての附属コンポーネントの導入解除を行います。"
