" Vim syntax file
" Language:	Haard
" Maintainer:	Hadley Siqueira <hadley.siqueira@gmail.com>
" Last Change:	2023 Jan 16
" Credits:	Hadley Siqueira <hadley.siqueira@gmail.com>
"
"		This version is based on Python by Zvezdan Petkovic.
"
" Optional highlighting can be controlled using these variables.
"
"   let HD_no_builtin_highlight = 1
"   let HD_no_doctest_code_highlight = 1
"   let HD_no_doctest_highlight = 1
"   let HD_no_exception_highlight = 1
"   let HD_no_number_highlight = 1
"   let HD_space_error_highlight = 1
"
" All the options above can be switched on together.
"
"   let HD_highlight_all = 1
"

" For version 5.x: Clear all syntax items.
" For version 6.x: Quit when a syntax file was already loaded.
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" We need nocompatible mode in order to continue lines with backslashes.
" Original setting will be restored.
let s:cpo_save = &cpo
set cpo&vim

" Keep HD keywords in alphabetical order inside groups for easy
" comparison with the table in the 'HD Language Reference'
" https://docs.HD.org/2/reference/lexical_analysis.html#keywords,
" https://docs.HD.org/3/reference/lexical_analysis.html#keywords.
" Groups are in the order presented in NAMING CONVENTIONS in syntax.txt.
" Exceptions come last at the end of each group (class and def below).
"
" Keywords 'with' and 'as' are new in HD 2.6
" (use 'from __future__ import with_statement' in HD 2.5).
"
" Some compromises had to be made to support both HD 3 and 2.
" We include HD 3 features, but when a definition is duplicated,
" the last definition takes precedence.
"
" - 'False', 'None', and 'True' are keywords in HD 3 but they are
"   built-ins in 2 and will be highlighted as built-ins below.
" - 'exec' is a built-in in HD 3 and will be highlighted as
"   built-in below.
" - 'nonlocal' is a keyword in HD 3 and will be highlighted.
" - 'print' is a built-in in HD 3 and will be highlighted as
"   built-in below (use 'from __future__ import print_function' in 2)
" - async and await were added in HD 3.5 and are soft keywords.
"
syn keyword HDStatement	true, null, false
syn keyword HDStatement	as assert break continue del exec global
syn keyword HDStatement	lambda nonlocal pass return with yield new delete
syn keyword HDStatement	interface class this def function var const enum union struct switch default data alias nextgroup=HDFunction skipwhite
syn keyword HDConditional	elif else if
syn keyword HDRepeat	for while case others 
syn keyword HDOperator	and in is not or sizeof
syn keyword HDException	except finally raise try
syn keyword HDInclude	from import
syn keyword HDAsync		async await

" Decorators (new in HD 2.4)
syn match   HDDecorator	"@" display nextgroup=HDFunction skipwhite
syn match   HDDecorator	"$" display nextgroup=HDFunction skipwhite
" The zero-length non-grouping match before the function name is
" extremely important in HDFunction.  Without it, everything is
" interpreted as a function inside the contained environment of
" doctests.
" A dot must be allowed because of @MyClass.myfunc decorators.
syn match   HDFunction
      \ "\%(\%(enum\s\|union\s\|function\s\|struct\s\|@\)\s*\)\@<=\h\%(\w\|\.\)*" contained

syn match   HDComment	"#.*$" contains=HDTodo,@Spell
syn keyword HDTodo		FIXME NOTE NOTES TODO contained

syn match  rubySymbol		"[]})\"':]\@<!:\%(\^\|\~\|<<\|<=>\|<=\|<\|===\|[=!]=\|[=!]\~\|!\|>>\|>=\|>\||\|-@\|-\|/\|\[]=\|\[]\|\*\*\|\*\|&\|%\|+@\|+\|`\)"
syn match  rubySymbol		"[]})\"':]\@<!:\$\%(-.\|[`~<=>_,;:!?/.'"@$*\&+0]\)"
syn match  rubySymbol		"[]})\"':]\@<!:\%(\$\|@@\=\)\=\%(\h\|[^\x00-\x7F]\)\%(\w\|[^\x00-\x7F]\)*"
syn match  rubySymbol		"[]})\"':]\@<!:\%(\h\|[^\x00-\x7F]\)\%(\w\|[^\x00-\x7F]\)*\%([?!=]>\@!\)\="
syn match  rubySymbol		"\%([{(,]\_s*\)\@<=\l\w*[!?]\=::\@!"he=e-1
syn match  rubySymbol		"[]})\"':]\@<!\%(\h\|[^\x00-\x7F]\)\%(\w\|[^\x00-\x7F]\)*[!?]\=:\s\@="he=e-1
syn match  rubySymbol		"\%([{(,]\_s*\)\@<=[[:space:],{]\l\w*[!?]\=::\@!"hs=s+1,he=e-1
syn match  rubySymbol		"[[:space:],{]\%(\h\|[^\x00-\x7F]\)\%(\w\|[^\x00-\x7F]\)*[!?]\=:\s\@="hs=s+1,he=e-1
syn region rubySymbol		start="[]})\"':]\@<!:'"  end="'"  skip="\\\\\|\\'"  contains=rubyQuoteEscape fold
syn region rubySymbol		start="[]})\"':]\@<!:\"" end="\"" skip="\\\\\|\\\"" contains=@rubyStringSpecial fold

syn region rubySymbol matchgroup=rubySymbolDelimiter start="%s\z([~`!@#$%^&*_\-+=|\:;"',.? /]\)"   end="\z1" skip="\\\\\|\\\z1" fold
syn region rubySymbol matchgroup=rubySymbolDelimiter start="%s{"				   end="}"   skip="\\\\\|\\}"	fold contains=rubyNestedCurlyBraces,rubyDelimEscape
syn region rubySymbol matchgroup=rubySymbolDelimiter start="%s<"				   end=">"   skip="\\\\\|\\>"	fold contains=rubyNestedAngleBrackets,rubyDelimEscape
syn region rubySymbol matchgroup=rubySymbolDelimiter start="%s\["				   end="\]"  skip="\\\\\|\\\]"	fold contains=rubyNestedSquareBrackets,rubyDelimEscape
syn region rubySymbol matchgroup=rubySymbolDelimiter start="%s("				   end=")"   skip="\\\\\|\\)"	fold contains=rubyNestedParentheses,rubyDelimEscape


hi def link rubySymbol			Constant


" Triple-quoted strings can contain doctests.
syn region  HDString matchgroup=HDQuotes
      \ start=+[uU]\=\z(['"]\)+ end="\z1" skip="\\\\\|\\\z1"
      \ contains=HDEscape,@Spell
syn region  HDString matchgroup=HDTripleQuotes
      \ start=+[uU]\=\z('''\|"""\)+ end="\z1" keepend
      \ contains=HDEscape,HDSpaceError,HDDoctest,@Spell
syn region  HDRawString matchgroup=HDQuotes
      \ start=+[uU]\=[rR]\z(['"]\)+ end="\z1" skip="\\\\\|\\\z1"
      \ contains=@Spell
syn region  HDRawString matchgroup=HDTripleQuotes
      \ start=+[uU]\=[rR]\z('''\|"""\)+ end="\z1" keepend
      \ contains=HDSpaceError,HDDoctest,@Spell

syn match   HDEscape	+\\[abfnrtv'"\\]+ contained
syn match   HDEscape	"\\\o\{1,3}" contained
syn match   HDEscape	"\\x\x\{2}" contained
syn match   HDEscape	"\%(\\u\x\{4}\|\\U\x\{8}\)" contained
" HD allows case-insensitive Unicode IDs: http://www.unicode.org/charts/
syn match   HDEscape	"\\N{\a\+\%(\s\a\+\)*}" contained
syn match   HDEscape	"\\$"

if exists("HD_highlight_all")
  if exists("HD_no_builtin_highlight")
    unlet HD_no_builtin_highlight
  endif
  if exists("HD_no_doctest_code_highlight")
    unlet HD_no_doctest_code_highlight
  endif
  if exists("HD_no_doctest_highlight")
    unlet HD_no_doctest_highlight
  endif
  if exists("HD_no_exception_highlight")
    unlet HD_no_exception_highlight
  endif
  if exists("HD_no_number_highlight")
    unlet HD_no_number_highlight
  endif
  let HD_space_error_highlight = 1
endif

" It is very important to understand all details before changing the
" regular expressions below or their order.
" The word boundaries are *not* the floating-point number boundaries
" because of a possible leading or trailing decimal point.
" The expressions below ensure that all valid number literals are
" highlighted, and invalid number literals are not.  For example,
"
" - a decimal point in '4.' at the end of a line is highlighted,
" - a second dot in 1.0.0 is not highlighted,
" - 08 is not highlighted,
" - 08e0 or 08j are highlighted,
"
" and so on, as specified in the 'HD Language Reference'.
" https://docs.HD.org/2/reference/lexical_analysis.html#numeric-literals
" https://docs.HD.org/3/reference/lexical_analysis.html#numeric-literals
if !exists("HD_no_number_highlight")
  " numbers (including longs and complex)
  syn match   HDNumber	"\<0[oO]\=\o\+[Ll]\=\>"
  syn match   HDNumber	"\<0[xX]\x\+[Ll]\=\>"
  syn match   HDNumber	"\<0[bB][01]\+[Ll]\=\>"
  syn match   HDNumber	"\<\%([1-9]\d*\|0\)[Ll]\=\>"
  syn match   HDNumber	"\<\d\+[jJ]\>"
  syn match   HDNumber	"\<\d\+[eE][+-]\=\d\+[jJ]\=\>"
  syn match   HDNumber
	\ "\<\d\+\.\%([eE][+-]\=\d\+\)\=[jJ]\=\%(\W\|$\)\@="
  syn match   HDNumber
	\ "\%(^\|\W\)\zs\d*\.\d\+\%([eE][+-]\=\d\+\)\=[jJ]\=\>"
endif

" Group the built-ins in the order in the 'HD Library Reference' for
" easier comparison.
" https://docs.HD.org/2/library/constants.html
" https://docs.HD.org/3/library/constants.html
" http://docs.HD.org/2/library/functions.html
" http://docs.HD.org/3/library/functions.html
" http://docs.HD.org/2/library/functions.html#non-essential-built-in-functions
" http://docs.HD.org/3/library/functions.html#non-essential-built-in-functions
" HD built-in functions are in alphabetical order.
if !exists("HD_no_builtin_highlight")
  " built-in constants
  " 'False', 'True', and 'None' are also reserved words in HD 3
  syn keyword HDBuiltin	true false null
  syn keyword HDBuiltin	NotImplemented Ellipsis __debug__
  " built-in functions
  syn keyword HDBuiltin i8 i16 i32 i64 u8 u16 u32 u64 f32 f64
  syn keyword HDBuiltin void bool char uchar short ushort int uint long ulong float double sym str
  " HD 2 only
  " avoid highlighting attributes as builtins
  syn match   HDAttribute	/\.\h\w*/hs=s+1 contains=ALLBUT,HDBuiltin transparent
endif

" From the 'HD Library Reference' class hierarchy at the bottom.
" http://docs.HD.org/2/library/exceptions.html
" http://docs.HD.org/3/library/exceptions.html
if !exists("HD_no_exception_highlight")
  " builtin base exceptions (used mostly as base classes for other exceptions)
  syn keyword HDExceptions	BaseException Exception
  syn keyword HDExceptions	ArithmeticError BufferError
  syn keyword HDExceptions	LookupError
  " builtin base exceptions removed in HD 3
  syn keyword HDExceptions	EnvironmentError StandardError
  " builtin exceptions (actually raised)
  syn keyword HDExceptions	AssertionError AttributeError
  syn keyword HDExceptions	EOFError FloatingPointError GeneratorExit
  syn keyword HDExceptions	ImportError IndentationError
  syn keyword HDExceptions	IndexError KeyError KeyboardInterrupt
  syn keyword HDExceptions	MemoryError NameError NotImplementedError
  syn keyword HDExceptions	OSError OverflowError ReferenceError
  syn keyword HDExceptions	RuntimeError StopIteration SyntaxError
  syn keyword HDExceptions	SystemError SystemExit TabError TypeError
  syn keyword HDExceptions	UnboundLocalError UnicodeError
  syn keyword HDExceptions	UnicodeDecodeError UnicodeEncodeError
  syn keyword HDExceptions	UnicodeTranslateError ValueError
  syn keyword HDExceptions	ZeroDivisionError
  " builtin OS exceptions in HD 3
  syn keyword HDExceptions	BlockingIOError BrokenPipeError
  syn keyword HDExceptions	ChildProcessError ConnectionAbortedError
  syn keyword HDExceptions	ConnectionError ConnectionRefusedError
  syn keyword HDExceptions	ConnectionResetError FileExistsError
  syn keyword HDExceptions	FileNotFoundError InterruptedError
  syn keyword HDExceptions	IsADirectoryError NotADirectoryError
  syn keyword HDExceptions	PermissionError ProcessLookupError
  syn keyword HDExceptions	RecursionError StopAsyncIteration
  syn keyword HDExceptions	TimeoutError
  " builtin exceptions deprecated/removed in HD 3
  syn keyword HDExceptions	IOError VMSError WindowsError
  " builtin warnings
  syn keyword HDExceptions	BytesWarning DeprecationWarning FutureWarning
  syn keyword HDExceptions	ImportWarning PendingDeprecationWarning
  syn keyword HDExceptions	RuntimeWarning SyntaxWarning UnicodeWarning
  syn keyword HDExceptions	UserWarning Warning
  " builtin warnings in HD 3
  syn keyword HDExceptions	ResourceWarning
endif

if exists("HD_space_error_highlight")
  " trailing whitespace
  syn match   HDSpaceError	display excludenl "\s\+$"
  " mixed tabs and spaces
  syn match   HDSpaceError	display " \+\t"
  syn match   HDSpaceError	display "\t\+ "
endif

" Do not spell doctests inside strings.
" Notice that the end of a string, either ''', or """, will end the contained
" doctest too.  Thus, we do *not* need to have it as an end pattern.
if !exists("HD_no_doctest_highlight")
  if !exists("HD_no_doctest_code_highlight")
    syn region HDDoctest
	  \ start="^\s*>>>\s" end="^\s*$"
	  \ contained contains=ALLBUT,HDDoctest,@Spell
    syn region HDDoctestValue
	  \ start=+^\s*\%(>>>\s\|\.\.\.\s\|"""\|'''\)\@!\S\++ end="$"
	  \ contained
  else
    syn region HDDoctest
	  \ start="^\s*>>>" end="^\s*$"
	  \ contained contains=@NoSpell
  endif
endif

" Sync at the beginning of class, function, or method definition.
syn sync match HDSync grouphere NONE "^\s*\%(def\|class\)\s\+\h\w*\s*("

if version >= 508 || !exists("did_HD_syn_inits")
  if version <= 508
    let did_HD_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  " The default highlight links.  Can be overridden later.
  HiLink HDStatement	Statement
  HiLink HDConditional	Conditional
  HiLink HDRepeat		Repeat
  HiLink HDOperator		Operator
  HiLink HDException	Exception
  HiLink HDInclude		Include
  HiLink HDAsync		Statement
  HiLink HDDecorator	Define
  HiLink HDFunction		Function
  HiLink HDComment		Comment
  HiLink HDTodo		Todo
  HiLink HDString		String
  HiLink HDRawString	String
  HiLink HDQuotes		String
  HiLink HDTripleQuotes	HDQuotes
  HiLink HDEscape		Special
  if !exists("HD_no_number_highlight")
    HiLink HDNumber		Number
  endif
  if !exists("HD_no_builtin_highlight")
    HiLink HDBuiltin	Function
  endif
  if !exists("HD_no_exception_highlight")
    HiLink HDExceptions	Structure
  endif
  if exists("HD_space_error_highlight")
    HiLink HDSpaceError	Error
  endif
  if !exists("HD_no_doctest_highlight")
    HiLink HDDoctest	Special
    HiLink HDDoctestValue	Define
  endif

  delcommand HiLink
endif

let b:current_syntax = "HD"

let &cpo = s:cpo_save
unlet s:cpo_save

" vim:set sw=2 sts=2 ts=8 noet:
