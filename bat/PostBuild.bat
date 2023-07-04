echo //////////// COPY DLL ////////////
copy %cd%\Release\Final\PB2E.dll %PB2EX_DIR%\pb2_re34_alt_p.app\META-INF\AIR\extensions\com.pb2works.PB2E\META-INF\ANE\Windows-x86\PB2E.dll
copy %cd%\Release\Final\PB2E.dll %cd%\ANE\platforms\Windows-x86\PB2E.dll
echo //////////// MAKE ANE ////////////
cd ANE
make