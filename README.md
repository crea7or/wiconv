# wiconv
Windows iconv for git to view diffs of utf-16 encoded files. The tool is used to convert utf-16 to utf-8 and vice versa with only 9kb executable! 

Converts Windows unicode UTF-16 files to git-friendly UTF-8 ( project.rc and resource.h ). The original file is untouched, this conversion is made for git diif tool only. Conversion is made only if file in UTF-16 due to wiconv tool ( detected by marker at the start of the file ). If file is not in utf-16 - no conversion is made and file returned unmodified ( for older .rc and resource.h files ).
 
 How to use:

 1) requires wiconv:
 
		http://github.com/crea7or/wiconv
		download/build and place to "program files/git/cmd"

 2) A script must be placed in: "program files/git/cmd" with name "utf16"
 		you can find it in this repo too.

 3) modify global /user/.gitconfig
 
 		[diff "utf16"]		
			textconv = utf16

 4) modify global /user/.gitattributes or local .gitattributes in project:
 
 		*.rc diff=utf16
		resource.h diff=utf16

