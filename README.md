Coding guide lines for working on TeXnicCenter
==============================================

Unicode
-------

String literals have to be enclosed by the _T-macro, e.g. _T("String"), to make
sure that the Unicode version of TeXnicCenter compiles correctly. When declaring
variables use the TCHAR-typedefs, such as LPTSTR, LPCTSTR instead of char* and
const char* and TCHAR instead of char.


Macros
------

Always use inline funtions and const/enum instead of #defines. Constants can be
declared as follows:

    const double PI = 3.14159265358979;
    const int ArraySize = 1024;
    LPCTSTR const FileName = _T("temp.out");


Mostly Private: Encapsulation
-----------------------------

Implementation details should always be hidden using the private/protected
access modifier. Prefer private to protected. When defining object properties,
provide a public Get and/or Set method instead of making the data member public.
This makes your and our life easier to debug the application.


Global Variables
----------------

Don't use global variables, use the Singleton pattern instead. Refer to
http://en.wikipedia.org/wiki/Singleton_pattern for detailed description.  See
also TeXnicCenter/configuration.h for a possible implementation.


Indent Style
------------

TeXnicCenter uses two different styles of indent: ANSI and K&R. See
http://en.wikipedia.org/wiki/Indent_style for explanations.

The first rule is to *not* change the format of the source code. Changes to the
source code should be related to the content, not the format. This allows easier
diffs and merges.

The second rule is to stick to the indent style that is dominating in the source
file. In other words, the creator of a file dictates the indent style for that
file. And this shall not be changed nor mixed with other styles.

If format changes become necessary for some reason, such changes shall be
committed separately to the repository. Do not mix format changes with content
changes. This hinders diffs and merges.

The majority of files uses ANSI style. If you create a new file and prefer ANSI
or are in doubt what style to use, then use ANSI. If you create a new file and
prefer K&R, then use that.


Tabs and Spaces
---------------

We use tabs for indentation and a tab width of 4. Do not convert tabs to spaces.


Hungarian Notation
------------------

We do *not* use Hungarian notation (anymore). See
http://en.wikipedia.org/wiki/Hungarian_notation for explanations.

You may notice that Hungarian notation is used very often in the source code of
TXC. This is not encouraged anymore. However, we do not change the code just for
that reason.

In particular, new classes get a name without the preceding 'C'.


CamelCase
---------

It is desired to use CamelCase for variables, classes, and file names. See
http://en.wikipedia.org/wiki/CamelCase for explanations.

In particular, start the word with a capital letter if you use CamelCase for
that word, i.e., not camelCase.


Structured Programming
----------------------

Follow the structured programming paradigm when possible.  This means that
especially 'goto' is not allowed at all.  Not following this paradigm results in
spaghetti code which is hard to understand and debug.  Other statements such as
'break', 'continue', or multiple 'return's in a single function are allowed but
should be used with care.


const-Correctness
------------------

Use the const keyword for data and class members as much as possible, especially
for Get-methods or the like.  See http://en.wikipedia.org/wiki/Const-correctness
for further explanation.


Single line 'for' and 'if' statements
-------------------------------------

If the body of a 'for' or 'if' statement consists of a single line only, it is
desired to write it with braces.  If you omit the braces, please insert an empty
line after the statement.

    if (foo)
    {
        bar() //Correct.
    }

    if (foo)
        bar() //Not desired. Use only with care. Insert empty line after statement.

It is also not desired to write it into a single line:

    if (foo) bar() //Not desired. Use only with care. Insert empty line after statement.


Modifications
-------------

TeXnicCenter uses the Scintilla source code editing component by Neil Hodgson
and Scintilla MFC wrapper classes by PJ Naughter (http://naughter.com/scintilla.html).

Scintilla
^^^^^^^^^

Scintalla's original TeX lexer (LexTeX.cxx) has been modified to allow richer
syntax highlighting and a BibTeX lexer (LexBibTeX.cxx) has been added.

In order to be able to use a custom cursor for incremental search the command
SCI_SHOWCURSOR(bool) has been introduced which suppresses cursor changes.

The changes that have been made can be reviewed by comparing revisions 912 and 
916 of the following files:
  
  Scintilla/include/Scintilla.h
  Scintilla/src/Editor.cxx
  Scintilla/src/Editor.h


Scintilla MFC wrapper classes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The member function void CScintillaCtrl::ShowCursor(bool show, bool direct) has
been added to wrap the previously mentioned message.

static CScintillaFindReplaceDlg* CScintillaFindReplaceDlg::GetFindReplaceDlg()
has been added to TeXnicCenter/ScintillaDocView.cpp which returns
_scintillaEditState.pFindReplaceDlg.

To the same file #include "FontOccManager.h" has been added.

CScintillaFindReplaceDlg::Create has been modified to use a dialog template from
the memory by adding the FR_ENABLETEMPLATEHANDLE flag to m_fr.Flags and using
the DialogTemplate class. This change allows to replace dialog's resource font
by the default OS font especially useful for Windows Vista users.

CScintillaView::OnReplaceAll has been modified to use targets instead of
selections to supress unwated display updates, see Scintilla documentation.
This function calls another new function 
  
    virtual void CScintillaView::GetReplaceAllTarget(long& s, long& e).

To review these changes compare revisions 913 and 921 of the following files:
  
  TeXnicCenter/ScintillaDocView.cpp
  TeXnicCenter/ScintillaDocView.h

and also revisions 913 and 916 of

  TeXnicCenter/ScintillaCtrl.cpp
  TeXnicCenter/ScintillaCtrl.h
