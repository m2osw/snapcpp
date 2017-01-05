" Vim syntax file
" Language:	AS2JS
" Maintainer:	Alexis Wilke <alexis@m2osw.com>
" Last change:	2014 Nov 12
"
" Installation:
"
" To use this file, add something as follow in your .vimrc file:
"
"   if !exists("my_autocommands_loaded")
"     let my_autocommands_loaded=1
"     au BufNewFile,BufReadPost *.js    so $HOME/vim/as2js.vim
"   endif
"
" Obviously, you will need to put the correct path to the as2js.vim
" file before it works, and you may want to use an extension other
" than .js.
"
"
" Copyright (c) 2005-2017 Made to Order Software, Corp.
"
" Permission is hereby granted, free of charge, to any
" person obtaining a copy of this software and
" associated documentation files (the "Software"), to
" deal in the Software without restriction, including
" without limitation the rights to use, copy, modify,
" merge, publish, distribute, sublicense, and/or sell
" copies of the Software, and to permit persons to whom
" the Software is furnished to do so, subject to the
" following conditions:
"
" The above copyright notice and this permission notice
" shall be included in all copies or substantial
" portions of the Software.
"
" THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
" ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
" LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
" FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
" EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
" LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
" WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
" ARISING FROM, OUT OF OR IN CONNECTION WITH THE
" SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
" SOFTWARE.
"


" Remove any other syntax
syn clear

set formatoptions-=tc
set formatoptions+=or

" minimum number of lines for synchronization
" /* ... */ comments can be long
syn sync minlines=150


" Operators
syn match	as2jsOperator		"+"
syn match	as2jsOperator		"&"
syn match	as2jsOperator		"\~"
syn match	as2jsOperator		"|"
syn match	as2jsOperator		"\^"
syn match	as2jsOperator		","
syn match	as2jsOperator		"/"
syn match	as2jsOperator		">"
syn match	as2jsOperator		"<"
syn match	as2jsOperator		"!"
syn match	as2jsOperator		"%"
syn match	as2jsOperator		"\*"
syn match	as2jsOperator		"\."
syn match	as2jsOperator		";"
syn match	as2jsOperator		"-"
syn match	as2jsOperator		"="
syn match	as2jsOperator		"+="
syn match	as2jsOperator		"&="
syn match	as2jsOperator		"|="
syn match	as2jsOperator		"\^="
syn match	as2jsOperator		"/="
syn match	as2jsOperator		"&&="
syn match	as2jsOperator		"||="
syn match	as2jsOperator		"\^\^="
syn match	as2jsOperator		"%="
syn match	as2jsOperator		"\*="
syn match	as2jsOperator		"\*\*="
syn match	as2jsOperator		"<<="
syn match	as2jsOperator		">>="
syn match	as2jsOperator		">>>="
syn match	as2jsOperator		"!<="
syn match	as2jsOperator		"!>="
syn match	as2jsOperator		"-="
syn match	as2jsOperator		"--"
syn match	as2jsOperator		"=="
syn match	as2jsOperator		">="
syn match	as2jsOperator		"++"
syn match	as2jsOperator		"<="
syn match	as2jsOperator		"&&"
syn match	as2jsOperator		"||"
syn match	as2jsOperator		"\^\^"
syn match	as2jsOperator		"\~="
syn match	as2jsOperator		"?>"
syn match	as2jsOperator		"?<"
syn match	as2jsOperator		"!="
syn match	as2jsOperator		"\*\*"
syn match	as2jsOperator		"\.\."
syn match	as2jsOperator		"\.\.\."
syn match	as2jsOperator		"!<"
syn match	as2jsOperator		"!>"
syn match	as2jsOperator		"::"
syn match	as2jsOperator		"<<"
syn match	as2jsOperator		">>"
syn match	as2jsOperator		">>>"
syn match	as2jsOperator		"==="
syn match	as2jsOperator		"!==="


" Complex keywords
syn match	as2jsKeyword		"\<function\>\([ \t\n\r]\+\<[sg]et\>\)\="
syn match	as2jsKeyword		"\<for\>\([ \t\n\r]\+\<each\>\)\="

" Keywords
syn keyword	as2jsKeyword		as break case catch class const
syn keyword	as2jsKeyword		continue default delete do else
syn keyword	as2jsKeyword		enum extends finally friend
syn keyword	as2jsKeyword		goto if implements import in
syn keyword	as2jsKeyword		inline instanceof interface
syn keyword	as2jsKeyword		intrinsic is namespace native new
syn keyword	as2jsKeyword		package private public return
syn keyword	as2jsKeyword		static super switch
syn keyword	as2jsKeyword		this throw try typeof use var
syn keyword	as2jsKeyword		virtual with while

" Known Types (internal)
syn keyword	as2jsType		Array Boolean Date Double Function Global
syn keyword	as2jsType		Integer Math Native Number Object
syn keyword	as2jsType		RegularExpression String System Void

" Constants
syn keyword	as2jsConstant		true false null undefined Infinity NaN
syn match	as2jsConstant		"\<0x[0-9A-F]\+\>"
syn match	as2jsConstant		"\<0[0-7]*\>"
syn match	as2jsConstant		"\<[1-9][0-9]*\.\=[0-9]*\([eE][+-]\=[0-9]\+\)\=\>"
syn match	as2jsConstant		"\<0\=\.[0-9]\+\([eE][+-]\=[0-9]\+\)\=\>"
syn region	as2jsConstant		start=+"+ skip=+\\.+ end=+"+
syn region	as2jsConstant		start=+'+ skip=+\\.+ end=+'+
syn region	as2jsRegularExpression	start=+`+ skip=+\\.+ end=+`+


" Labels
syn match	as2jsLabel		"^[a-zA-Z_$][a-zA-Z_$0-9]*[ \t\r\n]*:[^=:]\="

" prevent labels in `?:' expressions
syn region	as2jsNothing		start="?" end=":" contains=as2jsConstant,as2jsLComment,as2jsMComment
syn match	as2jsOperator		"?"
syn match	as2jsOperator		":"


" Comments
syn keyword	as2jsTodo		contained TODO FIXME XXX
syn match	as2jsTodo		contained "WATCH\(\s\=OUT\)\="
syn region	as2jsMComment		start="/\*" end="\*/" contains=as2jsTodo
syn region	as2jsLComment		start="//" end="$" contains=as2jsTodo


let b:current_syntax = "as2js"

if !exists("did_as2js_syntax_inits")
  let did_as2js_syntax_inits = 1
  hi link as2jsKeyword			Keyword
  hi link as2jsMComment			Comment
  hi link as2jsLComment			Comment
  hi link as2jsLabel			Typedef
  hi link as2jsTodo			Todo
  hi link as2jsType			Type
  hi link as2jsOperator			Operator
  hi link as2jsConstant			Constant
  hi link as2jsRegularExpression	Constant
endif
