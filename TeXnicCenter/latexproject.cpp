/********************************************************************
 *
 * This file is part of the TeXnicCenter-system
 *
 * Copyright (C) 1999-2000 Sven Wiegand
 * Copyright (C) 2000-$CurrentYear$ ToolsCenter
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * If you have further questions or if you want to support
 * further TeXnicCenter development, visit the TeXnicCenter-homepage
 *
 *    http://www.ToolsCenter.org
 *
 *********************************************************************/

/********************************************************************
 *
 * $Id$
 *
 ********************************************************************/

#include "stdafx.h"
#include "TeXnicCenter.h"

#include <sstream>

#include "MainFrm.h"
#include "LatexProject.h"
#include "Configuration.h"
#include "global.h"
#include "ProjectPropertyDialog.h"
#include "Process.h"
#include "TextSourceFile.h"
#include "ItemPropertyDialog.h"
#include "ChildFrm.h"
#include "ProjectNewDialog.h"
#include "Speller.h"
#include "SpellCheckDlg.h"
#include "LaTeXDocument.h"


IMPLEMENT_DYNCREATE(CLaTeXProject,CProject)

BEGIN_INTERFACE_MAP(CLaTeXProject,CProject)
INTERFACE_PART(CLaTeXProject,Interfaces::IID_IProject,Project)
END_INTERFACE_MAP()


CLaTeXProject::CLaTeXProject()
		: m_bCanParse(TRUE),
		m_bCanRunLatex(TRUE),
		m_pStructureParser(NULL),
		m_nCurrentStructureItem(-1),
		m_bUseBibTex(FALSE),
		m_bUseMakeIndex(FALSE),
		m_nInitialNavigatorTab(0)
		, tabbed_pane_(0)
{
	// initialization
	// Initialize the control bars and main frame pointer members
	m_pwndMainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());
	ASSERT(m_pwndMainFrame);

	m_pStructureParser = new CStructureParser(this,m_pwndMainFrame->GetOutputDoc() /*, this */);
	ASSERT(m_pStructureParser);

	if (!m_pStructureParser)
		AfxThrowMemoryException();
}

CLaTeXProject::~CLaTeXProject()
{
	delete m_pStructureParser;
}

BOOL CLaTeXProject::OnNewProject()
{
	if (!CProject::OnNewProject())
		return FALSE;

	//Add views to the docking bars in the frame wnd
	m_nInitialNavigatorTab = 0;
	CreateProjectViews();

	CProjectNewDialog dlg;

	dlg.AddTemplateFilter(_T("*.tex"),RUNTIME_CLASS(CFileBasedProjectTemplateItem));
	dlg.AddTemplateFilter(_T("*.dll"),RUNTIME_CLASS(CWizardBasedProjectTemplateItem));

	for (int i = 0; i < CConfiguration::GetInstance()->m_astrProjectTemplatePaths.GetSize(); i++)
		dlg.AddSearchDir(CConfiguration::GetInstance()->m_astrProjectTemplatePaths[i]);

	if (dlg.DoModal() == IDCANCEL)
		return FALSE;

	//Save and add to LRU
	DoFileSave();
	theApp.m_recentProjectList.Add(GetPathName());

	//Trigger analysis - parse project
	AfxGetMainWnd()->PostMessage(WM_COMMAND,ID_PROJECT_PARSE);

	return TRUE;
}

BOOL CLaTeXProject::OnNewProjectFromDoc(LPCTSTR lpszDocPathName)
{
	if (!CProject::OnNewProjectFromDoc(lpszDocPathName))
		return false;

	if (!CPathTool::Exists(lpszDocPathName))
	{
		AfxMessageBox(STE_PROJECT_MAINFILENOTFOUND,MB_ICONSTOP | MB_OK);
		return false;
	}

	//Set the path of the project file
	CString ProjectFileName = GetProjectFileNameFromDoc(lpszDocPathName);
	if (ProjectFileName.IsEmpty()) return false;

	//Does it already exist?
	if (CheckExistingProjectFile(ProjectFileName))
	{
		// save modified documents
		if (!theApp.SaveAllModified()) return false;

		// close latex-documents
		// skipping annoying question in this version --
		// just closing the documents.
		// lets see, what the users will say ...
		theApp.GetLatexDocTemplate()->CloseAllDocuments(theApp.m_bEndSession);

		//Open the project
		OnOpenProject(ProjectFileName);
		//... and (at least) the main file
		OnProjectOpenMainfile();

		return true;
	}

	//Set it
	SetPathName(ProjectFileName);

	//Set the main latex file
	SetMainPath(lpszDocPathName);

	//Open the properties dialog for the user to change some values.
	OnProjectProperties();

	//Save and add to LRU
	//DoFileSave(); ==> This brings up the Save-Dialog, if tcp is not there - not wanted
	OnSaveProject(GetPathName());
	theApp.m_recentProjectList.Add(GetPathName());

	//Add views to the docking bars in the frame wnd
	m_nInitialNavigatorTab = 0;
	CreateProjectViews();

	//Open the main file (just to be sure)
	OnProjectOpenMainfile();

	//Trigger analysis - parse project
	AfxGetMainWnd()->PostMessage(WM_COMMAND,ID_PROJECT_PARSE);

	return true;
}

CString CLaTeXProject::GetProjectFileNameFromDoc(LPCTSTR lpszDocPathName)
{
	CString ProjectFileName = (CPathTool::GetFileExtension(lpszDocPathName) == _T("tex"))
	                          ?
	                          CPathTool::GetBase(lpszDocPathName) + _T(".tcp")
	                          :
	                          _T("");
	return ProjectFileName;
};

bool CLaTeXProject::CheckExistingProjectFile(LPCTSTR lpszPathName)
{
	if (!CPathTool::Exists(lpszPathName)) return false;

	//Ask the user what to do
	CString strMsg;
	strMsg.Format(STE_PROJECT_EXISTS,lpszPathName);
	return (AfxMessageBox(strMsg,MB_ICONQUESTION | MB_YESNO) == IDYES);
};

LPCTSTR const TC_PROJECT_SESSION_EXT	= _T("tps");

BOOL CLaTeXProject::OnOpenProject(LPCTSTR lpszPathName)
{
	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// load project information
	CIniFile file(lpszPathName);

	if (!file.ReadFile())
		return FALSE;

	//Check if ini file contains any useful data.
	if (file.GetNumKeys() < 2)
	{
		//tcp-file not valid: inform the user about this error.
		CString strMsg;
		strMsg.Format(STE_FILE_INUSE,
		              AfxLoadString(IDS_OPEN),
		              lpszPathName,
		              AfxLoadString(STE_TCP_INVALID));

		AfxMessageBox(strMsg,MB_ICONEXCLAMATION | MB_OK);

		//and return without loading
		return false;
	}

	//Set the working dir of the project
	SetProjectDir(CPathTool::GetDirectory(lpszPathName));
	//Modify the LastOpenedFolder-Value to be the working dir
	AfxSetLastDirectory(CPathTool::GetDirectory(lpszPathName));

	//Load the Project Information
	Serialize(file,FALSE);

	SetModifiedFlag(FALSE);

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// load project session
	CIniFile session(GetSessionPathName(lpszPathName));

	if (session.ReadFile())
		SerializeSession(session,FALSE);

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// add views to the docking bars in the frame wnd
	CreateProjectViews();

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// Activate Editor window
	if (m_pwndMainFrame)
	{
		m_pwndMainFrame->SendMessage(WM_COMMAND,ID_WINDOW_EDITOR);
	}

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// parse project
	if (m_pwndMainFrame)
	{
		m_pwndMainFrame->PostMessage(WM_COMMAND,ID_PROJECT_PARSE);
	}

	return TRUE;
}

BOOL CLaTeXProject::OnSaveProject(LPCTSTR lpszPathName)
{
	//return CProject::OnSaveDocument(lpszPathName);
	CIniFile file(lpszPathName);
	file.Reset();
	if (!Serialize(file,TRUE))
		return FALSE;

	file.WriteFile();
	SetModifiedFlag(FALSE);
	return TRUE;
}

BOOL CLaTeXProject::SaveModified()
{
	if (!CProject::SaveModified())
		return FALSE;

	if (!theApp.GetLatexDocTemplate()->SaveAllModified())
		return FALSE;

	return TRUE;
}

void CLaTeXProject::OnCloseProject()
{
	// Save ignored words

	if (Speller* s = theApp.GetSpeller())
	{
		if (s->IsIgnoredModified())
		{
			const CString sIgnoredPath = GetIgnoredWordsFileName();
			s->SaveIgnoredWords(sIgnoredPath);
		}
	}

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// store project session
	CString SessionPathName = GetSessionPathName();

	if (!SessionPathName.IsEmpty())
	{
		POSITION pos = theApp.GetLatexDocTemplate()->GetFirstDocPosition();
		CDocument* doc;

		while (pos) {
			doc = theApp.GetLatexDocTemplate()->GetNextDoc(pos);
			ASSERT(doc->IsKindOf(RUNTIME_CLASS(CodeDocument)));
			SetFoldingPoints(doc->GetPathName(),static_cast<CodeDocument*>(doc)->GetFoldingPoints());
		}

		CIniFile session(SessionPathName);
		session.Reset();
		SerializeSession(session,TRUE);
		session.WriteFile();
	}

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// close latex-documents?

	// skipping annoying question in this version --
	// just closing the documents.
	// lets see, what the users will say ...
	theApp.GetLatexDocTemplate()->CloseAllDocuments(theApp.m_bEndSession);

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// remember last project
	if (theApp.m_bEndSession)
		CConfiguration::GetInstance()->m_strLastProject = GetPathName();
	else
		CConfiguration::GetInstance()->m_strLastProject.Empty();

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// Clean up

	//// add views to the docking bars in the frame wnd
	//CMainFrame *pwndMainFrame = (CMainFrame*)AfxGetMainWnd();

	//if (!pwndMainFrame)
	//    return;

	// cancel structure parsing
	if (m_pStructureParser)
	{
		m_pStructureParser->CancelParsing();
		WaitForSingleObject(m_pStructureParser->m_evtParsingDone,INFINITE);
	}

	// delete views
	DeleteProjectViews();
}

BEGIN_MESSAGE_MAP(CLaTeXProject,CProject)
	//{{AFX_MSG_MAP(CLaTeXProject)
	ON_COMMAND(ID_PROJECT_PROPERTIES,OnProjectProperties)
	ON_COMMAND(ID_PROJECT_OPEN_MAINFILE,OnProjectOpenMainfile)
	ON_COMMAND(ID_PROJECT_PARSED,OnProjectParsed)
	ON_COMMAND(ID_PROJECT_PARSE,OnProjectParse)
	ON_COMMAND(ID_ITEM_PROPERTIES,OnItemProperties)
	ON_UPDATE_COMMAND_UI(ID_ITEM_PROPERTIES,OnUpdateItemCmd)
	ON_COMMAND(ID_ITEM_GOTO,OnItemGoto)
	ON_COMMAND(ID_ITEM_INSERT_PAGEREF,OnItemInsertPageref)
	ON_COMMAND(ID_ITEM_INSERT_REF,OnItemInsertRef)
	ON_COMMAND(ID_ITEM_INSERT_LABEL,OnItemInsertLabel)
	ON_UPDATE_COMMAND_UI(ID_ITEM_GOTO,OnUpdateItemCmd)
	ON_UPDATE_COMMAND_UI(ID_ITEM_INSERT_PAGEREF,OnUpdateItemCmd)
	ON_UPDATE_COMMAND_UI(ID_ITEM_INSERT_REF,OnUpdateItemCmd)
	ON_UPDATE_COMMAND_UI(ID_ITEM_INSERT_LABEL,OnUpdateItemCmd)
	ON_COMMAND(ID_SPELL_PROJECT,OnSpellProject)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


#ifdef _DEBUG
void CLaTeXProject::AssertValid() const
{
	CProject::AssertValid();
}

void CLaTeXProject::Dump(CDumpContext& dc) const
{
	CProject::Dump(dc);
}
#endif //_DEBUG


#define KEY_FORMATINFO						_T("FormatInfo")
#define VAL_FORMATINFO_TYPE					_T("Type")
#define VAL_FORMATINFO_VERSION				_T("Version")

#define KEY_PROJECTINFO						_T("ProjectInfo")
#define VAL_PROJECTINFO_WORKINGDIR			_T("WorkingDir")
#define VAL_PROJECTINFO_MAINFILE			_T("MainFile")
#define VAL_PROJECTINFO_USEBIBTEX			_T("UseBibTeX")
#define VAL_PROJECTINFO_USEMAKEINDEX		_T("UseMakeIndex")
#define VAL_PROJECTINFO_ACTIVEPROFILE		_T("ActiveProfile")

#define VAL_PROJECTINFO_PLANGUAGE			_T("ProjectLanguage")
#define VAL_PROJECTINFO_PDIALECT			_T("ProjectDialect")

#define CURRENTFORMATVERSION				4
#define	FORMATTYPE							_T("TeXnicCenterProjectInformation")

BOOL CLaTeXProject::Serialize(CIniFile &ini, BOOL bWrite)
{
	if (bWrite)
	{
		// setting format information
		ini.SetValue(KEY_FORMATINFO,VAL_FORMATINFO_TYPE,FORMATTYPE);
		ini.SetValue(KEY_FORMATINFO,VAL_FORMATINFO_VERSION,CURRENTFORMATVERSION);

		// setting project information
		ini.SetValue(KEY_PROJECTINFO,VAL_PROJECTINFO_MAINFILE,CPathTool::GetRelativePath(GetProjectDir(),m_strMainPath));
		ini.SetValue(KEY_PROJECTINFO,VAL_PROJECTINFO_USEBIBTEX,(int)m_bUseBibTex);
		ini.SetValue(KEY_PROJECTINFO,VAL_PROJECTINFO_USEMAKEINDEX,(int)m_bUseMakeIndex);
		ini.SetValue(KEY_PROJECTINFO,VAL_PROJECTINFO_ACTIVEPROFILE,CProfileMap::GetInstance()->GetActiveProfileKey());

		ini.SetValue(KEY_PROJECTINFO,VAL_PROJECTINFO_PLANGUAGE,m_strProjectLanguage);
		ini.SetValue(KEY_PROJECTINFO,VAL_PROJECTINFO_PDIALECT,m_strProjectDialect);


		return TRUE;
	}
	else
	{
		// getting format information
		CString strType = ini.GetValue(KEY_FORMATINFO,VAL_FORMATINFO_TYPE,_T(""));
		int nVersion = ini.GetValue(KEY_FORMATINFO,VAL_FORMATINFO_VERSION,0);

		if (strType != FORMATTYPE)
			return FALSE;

		// setting project information
		if (!ini.VerifyValue(KEY_PROJECTINFO,VAL_PROJECTINFO_MAINFILE))
			return FALSE;

		if (nVersion == 1)
			m_strProjectDir = ini.GetValue(KEY_PROJECTINFO,VAL_PROJECTINFO_WORKINGDIR,_T(""));

		m_strMainPath = ini.GetValue(KEY_PROJECTINFO,VAL_PROJECTINFO_MAINFILE,_T(""));
		if (nVersion > 1)
			m_strMainPath = CPathTool::Cat(GetProjectDir(),m_strMainPath);

		m_bUseBibTex = ini.GetValue(KEY_PROJECTINFO,VAL_PROJECTINFO_USEBIBTEX,FALSE);
		m_bUseMakeIndex = ini.GetValue(KEY_PROJECTINFO,VAL_PROJECTINFO_USEMAKEINDEX,FALSE);

		m_strProjectLanguage = ini.GetValue(KEY_PROJECTINFO,VAL_PROJECTINFO_PLANGUAGE,CConfiguration::GetInstance()->m_strLanguageDefault);
		m_strProjectDialect = ini.GetValue(KEY_PROJECTINFO,VAL_PROJECTINFO_PDIALECT,CConfiguration::GetInstance()->m_strLanguageDialectDefault);


		if (nVersion > 2)
		{
			if (CProfileMap::GetInstance()->SetActiveProfile(ini.GetValue(KEY_PROJECTINFO,VAL_PROJECTINFO_ACTIVEPROFILE,_T("")),false))
			{
				//Successfull change of the active profile - update UI
				theApp.UpdateLaTeXProfileSel();
			}
			else
			{
				//Saved profile name not found - other profile used - set project to be modified
				SetModifiedFlag(true);
			}
		}

		return TRUE;
	}
}

#undef KEY_FORMATINFO
#undef VAL_FORMATINFO_TYPE
#undef VAL_FORMATINFO_VERSION
#undef KEY_PROJECTINFO
#undef VAL_PROJECTINFO_WORKINGDIR
#undef VAL_PROJECTINFO_MAINFILE
#undef CURRENTFORMATVERSION
#undef FORMATTYPE


#define KEY_FORMATINFO							_T("FormatInfo")
#define VAL_FORMATINFO_TYPE						_T("Type")
#define VAL_FORMATINFO_VERSION					_T("Version")

#define KEY_SESSIONINFO							_T("SessionInfo")
#define VAL_SESSIONINFO_ACTIVETAB				_T("ActiveTab")
#define VAL_SESSIONINFO_FRAMECOUNT				_T("FrameCount")
#define VAL_SESSIONINFO_ACTIVEFRAME				_T("ActiveFrame")

#define KEY_FRAMEINFO							_T("Frame%d")

#define CURRENTFORMATVERSION					2
#define	FORMATTYPE								_T("TeXnicCenterProjectSessionInformation")

void CLaTeXProject::SerializeSession(CIniFile &ini, BOOL bWrite)
{
	LPCTSTR const BookmarksKey = _T("Bookmarks");
	LPCTSTR const FoldingKey = _T("Folding");

	if (bWrite)
	{
		//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		//Format information
		ini.SetValue(KEY_FORMATINFO,VAL_FORMATINFO_TYPE,FORMATTYPE);
		ini.SetValue(KEY_FORMATINFO,VAL_FORMATINFO_VERSION,CURRENTFORMATVERSION);

		//Store frame information
		if (m_pwndMainFrame)
		{
			//Get the MDI Childs (sorted by Tabs)
			CArray<CChildFrame*,CChildFrame*> MDIChildArray;
			const int nActiveFrame = m_pwndMainFrame->GetMDIChilds(MDIChildArray,true);

			//Set Number of Frames
			ini.SetValue(KEY_SESSIONINFO,VAL_SESSIONINFO_FRAMECOUNT,MDIChildArray.GetSize());
			//Set active MDI Child
			ini.SetValue(KEY_SESSIONINFO,VAL_SESSIONINFO_ACTIVEFRAME,nActiveFrame);

			for (int i = 0; i < MDIChildArray.GetSize(); i++)
			{
				//Store information for this MDI-window
				CString strKey;
				strKey.Format(KEY_FRAMEINFO,i);

				MDIChildArray.GetAt(i)->Serialize(ini,strKey,bWrite);
			}
		}

#pragma region Bookmarks

		typedef FileBookmarksContainerType::iterator IB;
		std::basic_ostringstream<TCHAR> oss;

		for (IB it = bookmarks_.begin(); it != bookmarks_.end(); ++it) 
		{
			oss.str(_T(""));
			oss.clear();

			const BookmarkContainerType& bookmarks = it->second;
			std::copy(bookmarks.begin(),bookmarks.end(),std::ostream_iterator<CodeBookmark,TCHAR>(oss,_T(" ")));

			ini.SetValue(BookmarksKey,it->first,oss.str().c_str());
		}

#pragma endregion

#pragma region Folding points

		typedef FileFoldingPointsContainerType::iterator IF;

		for (IF it = folding_points_.begin(); it != folding_points_.end(); ++it) 
		{
			oss.str(_T(""));
			oss.clear();

			const FoldingPointContainerType& points = it->second;
			std::copy(points.begin(),points.end(),std::ostream_iterator<FoldingPoint,TCHAR>(oss,_T(" ")));

			ini.SetValue(FoldingKey,it->first,oss.str().c_str());
		}

#pragma endregion
	}
	else
	{
#pragma region Bookmarks

		// Restore the bookmarks before the views are created
		// otherwise the bookmarks will no be visible

		typedef std::multimap<CString,CString> ValueContainerType;
		ValueContainerType values;

		bookmarks_.clear();

		BookmarkContainerType bookmarks;

		if (ini.GetValues(BookmarksKey,values))
		{
			std::basic_istringstream<TCHAR> iss;
			typedef std::istream_iterator<CodeBookmark,TCHAR> iterator_type;
			typedef ValueContainerType::iterator I;

			for (I it = values.begin(); it != values.end(); ++it)
			{
				iss.clear();
				iss.str(static_cast<LPCTSTR>(it->second)); // Value

				bookmarks.clear();

				std::copy(iterator_type(iss),iterator_type(),std::back_inserter(bookmarks));
				bookmarks_.insert(std::make_pair(it->first,bookmarks)); 
			}
		}

#pragma endregion

#pragma region Folding points
		{
			// Restore the folding points before the views are created
			// otherwise the folding will no be set

			typedef std::multimap<CString,CString> ValueContainerType;
			ValueContainerType values;

			folding_points_.clear();

			FoldingPointContainerType points;

			if (ini.GetValues(FoldingKey,values))
			{
				std::basic_istringstream<TCHAR> iss;
				typedef std::istream_iterator<FoldingPoint,TCHAR> iterator_type;
				typedef ValueContainerType::iterator I;

				for (I it = values.begin(); it != values.end(); ++it)
				{
					iss.clear();
					iss.str(static_cast<LPCTSTR>(it->second)); // Value

					points.clear();

					std::copy(iterator_type(iss),iterator_type(),std::back_inserter(points));
					folding_points_.insert(std::make_pair(it->first,points));
				}
			}
		}

#pragma endregion
		
#pragma region Reading format information

		CString strKey;
		const int nFrameCount = ini.GetValue(KEY_SESSIONINFO,VAL_SESSIONINFO_FRAMECOUNT,0);
		const int nActiveFrame = ini.GetValue(KEY_SESSIONINFO,VAL_SESSIONINFO_ACTIVEFRAME,-1);

		//Open all stored frames
		bool bCouldOpenAllFrames(true);
		CChildFrame* pChildToBeActivated = NULL;

		for (int nFrame = 0; nFrame < nFrameCount; nFrame++)
		{
			CChildFrame* pChildFrame = new CChildFrame();

			ASSERT(pChildFrame);

			if (!pChildFrame) 
				continue;

			strKey.Format(KEY_FRAMEINFO,nFrame);
			
			if (!pChildFrame->Serialize(ini,strKey,bWrite))
			{
				bCouldOpenAllFrames = false;
				delete pChildFrame;
			}
			else
			{
				if (nActiveFrame == nFrame)
					pChildToBeActivated = pChildFrame;
			}
		}

#pragma endregion

		//Restore navigator tab selection
		m_nInitialNavigatorTab = ini.GetValue(KEY_SESSIONINFO,VAL_SESSIONINFO_ACTIVETAB,0);

		//Activate the frame that was active when closing the project
		if (pChildToBeActivated)
		{
			pChildToBeActivated->ActivateFrame(SW_SHOW);
			//NOTE: We give the focus later to this window
		}

		//Give message, if we could not open all files / frames
		if (!bCouldOpenAllFrames)
		{
			AfxMessageBox(STE_SESSION_RESTORE_NOTALLWINDOWSRESTORED,
			              MB_ICONINFORMATION | MB_OK);
		}
	}
}

/////////////////////////////////////////////////////////////////////
// paths, open, save, etc.

void CLaTeXProject::SetPathName(LPCTSTR lpszPathName)
{
	CProject::SetPathName(lpszPathName);
	SetProjectDir(CPathTool::GetDirectory(lpszPathName));

	// Open ignored words

	if (Speller* s = theApp.GetSpeller())
	{
		const CString sIgnoredPath = GetIgnoredWordsFileName();

		if (CPathTool::Exists(sIgnoredPath))
			s->LoadIgnoredWords(sIgnoredPath);
	}
}

void CLaTeXProject::DoProjectSave()
{
	OnSaveProject(GetPathName());
}

void CLaTeXProject::SetMainPath(LPCTSTR lpszMainPath)
{
	if (m_strMainPath != lpszMainPath)
		SetModifiedFlag();
	m_strMainPath = lpszMainPath;
}

const CString& CLaTeXProject::GetMainPath() const
{
	return m_strMainPath;
}

void CLaTeXProject::SetProjectDir(LPCTSTR lpszProjectDir)
{
	m_strProjectDir = lpszProjectDir;
}

const CString CLaTeXProject::GetProjectDir() const
{
	return m_strProjectDir;
}

const CString CLaTeXProject::GetFilePath(LPCTSTR lpszFile) const
{
	CString strPath;

	SetCurrentDirectory(CPathTool::GetDirectory(m_strMainPath));
	GetFullPathName(lpszFile,_MAX_PATH,strPath.GetBuffer(_MAX_PATH),0);
	strPath.ReleaseBuffer();

	return strPath;
}

//Returns the full path to the project session file

const CString CLaTeXProject::GetSessionPathName(LPCTSTR lpszPath /*= NULL*/) const
{
	if (!lpszPath && GetPathName().IsEmpty())
		return CString();

	return (CPathTool::GetBase((lpszPath != NULL) ? lpszPath : GetPathName())
	        + _T('.') + TC_PROJECT_SESSION_EXT);
}

void CLaTeXProject::SetRunBibTex(BOOL bRunBibTex)
{
	if (bRunBibTex != m_bUseBibTex)
		SetModifiedFlag();

	m_bUseBibTex = bRunBibTex;
}

void CLaTeXProject::SetRunMakeIndex(BOOL bRunMakeIndex)
{
	if (bRunMakeIndex != m_bUseMakeIndex)
		SetModifiedFlag();

	m_bUseMakeIndex = bRunMakeIndex;
}

BOOL CLaTeXProject::GetRunBibTex() const
{
	return m_bUseBibTex;
}

BOOL CLaTeXProject::GetRunMakeIndex() const
{
	return m_bUseMakeIndex;
}

CString CLaTeXProject::GetWorkingDir() const
{
	return CPathTool::GetDirectory(m_strMainPath);
}

void CLaTeXProject::OnProjectProperties()
{
	CProjectPropertyDialog dlg(theApp.m_pMainWnd);
	dlg.m_strProjectDir = GetProjectDir();
	dlg.m_strMainFile = m_strMainPath;
	dlg.m_bUseBibTex = m_bUseBibTex;
	dlg.m_bUseMakeIndex = m_bUseMakeIndex;
	dlg.m_strLanguageCurrent = m_strProjectLanguage;
	dlg.m_strDialectCurrent = m_strProjectDialect;

	if (dlg.DoModal() != IDOK)
		return;

	if (m_strMainPath != dlg.m_strMainFile ||
	        m_bUseBibTex != dlg.m_bUseBibTex ||
	        m_bUseMakeIndex != dlg.m_bUseMakeIndex)
	{
		SetModifiedFlag();
	}

	if (m_strProjectLanguage != dlg.m_strLanguageCurrent ||
	        m_strProjectDialect != dlg.m_strDialectCurrent)
	{
		SetModifiedFlag();
		// TO DO: restart the speller on the new language
	}


	m_strMainPath = dlg.m_strMainFile;
	m_bUseBibTex = dlg.m_bUseBibTex;
	m_bUseMakeIndex = dlg.m_bUseMakeIndex;
	m_strProjectLanguage = dlg.m_strLanguageCurrent;
	m_strProjectDialect = dlg.m_strDialectCurrent;
}

void CLaTeXProject::OnProjectOpenMainfile()
{
	theApp.OpenLatexDocument(m_strMainPath,FALSE,-1,FALSE,false);
}


/////////////////////////////////////////////////////////////////////
// implementation of CStructureParserHandler virtuals

void CLaTeXProject::OnParsingFinished(BOOL bSuccess)
{
	// synchronize with main thread
	if (bSuccess)
		theApp.m_pMainWnd->SendMessage(WM_COMMAND,(WPARAM)ID_PROJECT_PARSED);

}


/////////////////////////////////////////////////////////////////////
// manage project parsing

void CLaTeXProject::OnProjectParse()
{
	if (!m_bCanParse || IsClosing())
		return;

	// Update the background thread
	AfxGetMainWnd()->PostMessage(WM_COMMAND,ID_BG_UPDATE_PROJECT);

	// change to working dir
	SetCurrentDirectory(CPathTool::GetDirectory(m_strMainPath));

	// start parsing
	m_bCanParse = FALSE;
	m_pStructureParser->StartParsing(m_strMainPath,GetWorkingDir());
}

void CLaTeXProject::OnProjectParsed()
{
	m_pStructureParser->GetStructureItems(&m_aStructureItems);
	UpdateAllViews(NULL,COutputDoc::hintParsingFinished,(CObject*) & m_aStructureItems);
	m_bCanParse = TRUE;
}

void CLaTeXProject::SetCurrentStructureItem(int nIndex)
{
	// TODO: The assert fails sometimes due to nIndex being set to -1
	//ASSERT((nIndex == 0) || (nIndex < m_aStructureItems.size())); // invalid index
	if (nIndex < 0 || nIndex >= static_cast<int>(m_aStructureItems.size()))
		return;

	m_nCurrentStructureItem = nIndex;
}

void CLaTeXProject::OnUpdateItemCmd(CCmdUI* pCmdUI)
{
	ASSERT(m_nCurrentStructureItem >= -1 && m_nCurrentStructureItem < static_cast<int>(m_aStructureItems.size()));

	// disable command, if no valid item is selected
	if (m_nCurrentStructureItem == -1)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	// check commands
	const CStructureItem &si = m_aStructureItems[m_nCurrentStructureItem];

	switch (pCmdUI->m_nID)
	{
		case ID_ITEM_GOTO:
			switch (si.m_nType)
			{
				case CStructureParser::texFile :
				case CStructureParser::bibFile :
					pCmdUI->Enable(!si.m_strPath.IsEmpty());
					break;
				default:
					pCmdUI->Enable(si.m_nLine > 0 && !si.m_strPath.IsEmpty());
			}
			break;

		case ID_ITEM_INSERT_PAGEREF: // makes no sense for BibTeX entries
			if (si.m_nType != CStructureParser::bibItem)
			{
				pCmdUI->Enable(si.HasLabels() && theApp.GetActiveEditView());
			}
			else
			{
				pCmdUI->Enable(FALSE);
			}
			break;
		case ID_ITEM_INSERT_LABEL:
		case ID_ITEM_INSERT_REF:
			pCmdUI->Enable(si.HasLabels() && theApp.GetActiveEditView());
			break;
		default:
			pCmdUI->Enable();
	}
}

void CLaTeXProject::OnItemProperties()
{
	ASSERT(m_nCurrentStructureItem >= 0 && m_nCurrentStructureItem < static_cast<int>(m_aStructureItems.size()));
	CItemPropertyDialog dialog(m_aStructureItems[m_nCurrentStructureItem]);
	dialog.DoModal();
}

void CLaTeXProject::OnItemGoto()
{
	if (!(m_nCurrentStructureItem >= 0 && m_nCurrentStructureItem < static_cast<int>(m_aStructureItems.size())))
		return;

	const CStructureItem &si = m_aStructureItems[m_nCurrentStructureItem];

	switch (si.GetType())
	{
		case CStructureParser::texFile :
		case CStructureParser::bibFile :
			theApp.OpenLatexDocument(GetFilePath(si.GetPath()),FALSE,-1,FALSE,false);
			break;
		case CStructureParser::missingTexFile :
		case CStructureParser::missingGraphicFile :
		case CStructureParser::missingBibFile :
		case CStructureParser::graphicFile :
			theApp.OpenLatexDocument(GetFilePath(si.GetComment()),FALSE,si.GetLine(),FALSE,false);
			break;
		default:
			theApp.OpenLatexDocument(GetFilePath(si.GetPath()),FALSE,si.GetLine(),FALSE,false);
	}
}

void CLaTeXProject::OnItemInsertLabel()
{
	if (!(m_nCurrentStructureItem >= 0 && m_nCurrentStructureItem < static_cast<int>(m_aStructureItems.size())))
		return;

	LaTeXView *pView = theApp.GetActiveEditView();

	if (!pView)
		return;

	pView->InsertText(m_aStructureItems[m_nCurrentStructureItem].GetLabel());
	pView->SetFocus();
}

void CLaTeXProject::OnItemInsertPageref()
{
	if (!(m_nCurrentStructureItem >= 0 && m_nCurrentStructureItem < static_cast<int>(m_aStructureItems.size())))
		return;

	LaTeXView *pView = theApp.GetActiveEditView();
	if (!pView)
		return;

	pView->InsertText(FormatPageRef(m_aStructureItems[m_nCurrentStructureItem]));
	pView->SetFocus();
}

void CLaTeXProject::OnItemInsertRef()
{
	if (!(m_nCurrentStructureItem >= 0 && m_nCurrentStructureItem < static_cast<int>(m_aStructureItems.size())))
		return;

	LaTeXView *pView = theApp.GetActiveEditView();
	
	if (!pView)
		return;

	pView->InsertText(FormatRef(m_aStructureItems[m_nCurrentStructureItem]));
	pView->SetFocus();
}

const CString CLaTeXProject::FormatPageRef(const CStructureItem& item)
{
	CString result;

	if (item.HasLabels())
		result = AfxFormatString1(STE_LATEX_PAGEREF,item.GetLabel());

	return result;
}

const CString CLaTeXProject::FormatRef(const CStructureItem& item)
{
	CString result;

	if (item.HasLabels())
	{
		UINT strID = 0;

		switch (item.GetType())
		{
			case CStructureParser::equation :
				strID = STE_LATEX_EQREF;
				break;
			case CStructureParser::bibItem :
				strID = STE_LATEX_CITE;
				break;
			default:
				strID = STE_LATEX_REF;
				break;
		}

		result = AfxFormatString1(strID,item.GetLabel());
	}

	return result;
}

void CLaTeXProject::OnSpellProject()
{
	Speller* pSpell = theApp.GetSpeller();

	if (pSpell == NULL)
		return;

	// Setup path for ignored words
	const CString sIgnoredPath = GetIgnoredWordsFileName();

	SpellCheckDlg dlg(NULL,NULL);
	dlg.SetShowDoneMessage(false);
	dlg.SetCheckSelectionOnly(false);

	// Get own copy of structure items.
	StructureItemContainer aStructureItems;
	m_pStructureParser->GetStructureItems(&aStructureItems);

	for (int i = 0; i < aStructureItems.size(); ++i)
	{
		CStructureItem& si = aStructureItems[i];

		if (si.m_nType == CStructureParser::texFile)
		{
			bool bWasOpen = true;
			LaTeXDocument* pDoc = dynamic_cast<LaTeXDocument*>(theApp.GetOpenLatexDocument(GetFilePath(si.m_strPath),FALSE));

			if (pDoc == NULL)
			{
				pDoc = dynamic_cast<LaTeXDocument*>(theApp.OpenLatexDocument(GetFilePath(si.m_strPath),FALSE,-1,FALSE,false));
				bWasOpen = false;
			}

			if (pDoc == NULL)
			{
				// Can't open document, try another one.
				continue;
			}

			LaTeXView* pView = static_cast<LaTeXView*>(pDoc->GetView());

			ASSERT(pView);

			if (pView == NULL)
			{
				// View is NULL??
				continue;
			}

			dlg.Reset(pView,pSpell);

			// Save selection
			long ptStart = pView->GetCtrl().GetSelectionStart();
			long ptEnd = pView->GetCtrl().GetSelectionEnd();

			int result = dlg.DoModal();

			// begin ow
			if (theApp.GetSpeller()->IsAddedModified()) {
			    //TRACE("SC was modified, save ign words to %s...\n", sPath);
			    theApp.GetSpeller()->SaveIgnoredWords(sIgnoredPath);
			}
			// end ow

			// Restore selection
			pView->GetCtrl().SetSel(ptStart,ptEnd);

			if (!bWasOpen) 
			{
				//pView->SendMessage(WM_COMMAND,ID_FILE_CLOSE); // WM_COMMAND causes heap corruption
				pDoc->OnCloseDocument();
			}

			if (result == IDABORT)
				break;

			else if (result != IDOK)
				if (AfxMessageBox(AfxLoadString(IDS_SPELL_CONTINUE),MB_YESNO | MB_ICONQUESTION) != IDYES)
					break;
		}
	}
}

BOOL CLaTeXProject::CreateProjectViews()
{
	ASSERT(m_pwndMainFrame);
	m_pwndMainFrame->OnOpenProject(this);

	return TRUE;
}

void CLaTeXProject::DeleteProjectViews()
{
	ASSERT(m_pwndMainFrame);
	m_pwndMainFrame->OnCloseProject(this);
}

Interfaces::IProject *CLaTeXProject::GetProjectInterface()
{
	m_xProject.AddRef();
	return &m_xProject;
}


//-------------------------------------------------------------------
// class CLaTeXProject::XProject
//-------------------------------------------------------------------

IMPLEMENT_AGGREGATED_IUNKNOWN(CLaTeXProject,Project);

CString CLaTeXProject::XProject::GetTitle_() const
{
	GET_OUTER(CLaTeXProject,Project);

	return pThis->GetTitle();
}

CString CLaTeXProject::XProject::GetPath_() const
{
	GET_OUTER(CLaTeXProject,Project);

	return pThis->GetWorkingDir();
}

CString CLaTeXProject::XProject::GetMainFile_() const
{
	GET_OUTER(CLaTeXProject,Project);

	return pThis->GetMainPath();
}

BOOL CLaTeXProject::XProject::GetUsesBibTex_() const
{
	GET_OUTER(CLaTeXProject,Project);

	return pThis->GetRunBibTex();
}

void CLaTeXProject::XProject::SetUsesBibTex_(BOOL bUsesBibTex)
{
	GET_OUTER(CLaTeXProject,Project);

	pThis->SetRunBibTex(bUsesBibTex);
}

BOOL CLaTeXProject::XProject::GetUsesMakeIndex_() const
{
	GET_OUTER(CLaTeXProject,Project);

	return pThis->GetRunMakeIndex();
}

void CLaTeXProject::XProject::SetUsesMakeIndex_(BOOL bUsesMakeIndex)
{
	GET_OUTER(CLaTeXProject,Project);

	pThis->SetRunMakeIndex(bUsesMakeIndex);
}

int CLaTeXProject::GetInitialNavigatorTabIndex() const
{
	if (m_nInitialNavigatorTab < 0 || m_nInitialNavigatorTab > 3)
		m_nInitialNavigatorTab = 0;

	return m_nInitialNavigatorTab;
}

const StructureItemContainer& CLaTeXProject::GetStructureItems() const
{
	return m_aStructureItems;
}

const CString CLaTeXProject::GetIgnoredWordsFileName() const
{
	return
	    GetProjectDir() +
	    _T("\\ignored") +
	    GetTitle() +
	    CConfiguration::GetInstance()->m_strGuiLanguage + _T(".sc");
}

void CLaTeXProject::AddBookmark( const CString& filename, const CodeBookmark& b )
{
	const CString name = CPathTool::GetRelativePath(GetProjectDir(),filename,TRUE,FALSE);

	BookmarkContainerType& bms = bookmarks_[name];
	BookmarkContainerType::iterator it = std::find(bms.begin(),bms.end(),b);

	if (it != bms.end())
		bms.erase(it);

	bms.push_back(b);
}

void CLaTeXProject::RemoveBookmark( const CString& filename, const CodeBookmark& b )
{
	const CString name = CPathTool::GetRelativePath(GetProjectDir(),filename,TRUE,FALSE);
	FileBookmarksContainerType::iterator it = bookmarks_.find(name);

	if (it != bookmarks_.end()) 
	{
		BookmarkContainerType::iterator itb = std::find(it->second.begin(),it->second.end(),b);
		
		if (itb != it->second.end())
		{
			it->second.erase(itb);
		}
	}
}

bool CLaTeXProject::GetBookmarks( const CString& filename, BookmarkContainerType& bookmarks ) const
{
	bool result = false;
	bookmarks.clear();

	const CString name = CPathTool::GetRelativePath(GetProjectDir(),filename,TRUE,FALSE);

	FileBookmarksContainerType::const_iterator it = bookmarks_.find(name);

	if (it != bookmarks_.end())
	{
		bookmarks = it->second;
		result = true;
	}

	return result;
}

void CLaTeXProject::RemoveAllBookmarks( const CString& filename )
{
	const CString name = CPathTool::GetRelativePath(GetProjectDir(),filename,TRUE,FALSE);
	FileBookmarksContainerType::iterator it = bookmarks_.find(name);

	if (it != bookmarks_.end()) 
	{
		if (!it->second.empty()) 
			it->second.clear();
	}
}

bool CLaTeXProject::GetFoldingPoints( const CString& filename, FoldingPointContainerType& points ) const
{
	bool result = false;
	points.clear();

	const CString name = CPathTool::GetRelativePath(GetProjectDir(),filename,TRUE,FALSE);

	FileFoldingPointsContainerType::const_iterator it = folding_points_.find(name);

	if (it != folding_points_.end())
	{
		points = it->second;
		result = true;
	}

	return result;
}

void CLaTeXProject::SetFoldingPoints(const CString& filename, const FoldingPointContainerType& points)
{
	if (!points.empty())
	{
		const CString name = CPathTool::GetRelativePath(GetProjectDir(),filename,TRUE,FALSE);
		folding_points_[name] = points;
	}
}