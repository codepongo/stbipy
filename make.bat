cl /MT /D"WIN" /LD /I C:\python27\include stbi.c  C:\python27\libs\python27.lib
del stbi.obj stbi.lib
copy stbi.dll stbi.pyd
