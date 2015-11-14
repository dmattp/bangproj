mp-addpath-msbuild
dir *.vcxproj | %{ msbuild /p:Configuration=release $_ }
copy release/*.dll,release/*.exe \bang
