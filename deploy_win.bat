@echo
C:\qt\5.6\msvc2013\bin\windeployqt.exe .\debug\acquisition.exe --release --no-compiler-runtime --dir .\release\
copy .\debug\acquisition.exe .\release\
copy .\redist\* .\release\
del /S /Q .\release\bearer
rmdir .\release\bearer
copy .\debug\acquisition.pdb .\release\
echo "[Paths]\nLibraries=./plugins" > qt.conf