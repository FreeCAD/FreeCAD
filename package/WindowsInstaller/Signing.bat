signtool.exe sign /f FCweborg.pfx /p FreeCADIsCool /fd sha512 /tr http://timestamp.digicert.com /td sha512 /v %1
certutil -hashfile %1 SHA256 > %1-SHA256.txt
