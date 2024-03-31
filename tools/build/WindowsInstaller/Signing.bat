signtool.exe sign /sha1 "1df70f0b16e019698bf6b38c8e66434fa96e6cf4" /tr http://time.certum.pl /td sha256 /fd sha256 /v %1
certutil -hashfile %1 SHA256 > %1-SHA256.txt
