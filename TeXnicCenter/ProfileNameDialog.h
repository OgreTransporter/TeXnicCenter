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

#if !defined(AFX_PROFILENAMEDIALOG_H__F9A231A0_1F87_11D5_A222_006097239934__INCLUDED_)
#define AFX_PROFILENAMEDIALOG_H__F9A231A0_1F87_11D5_A222_006097239934__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/**
Dialog that requests the user to enter a name for an output profile.

@author Sven Wiegand
*/
class CProfileNameDialog : public CDialog
{
// construction/destruction
public:
	CProfileNameDialog(LPCTSTR lpszName, CWnd* pParent = NULL);

// operations
public:
	CString GetName() const;

// overridings
protected:
	//{{AFX_VIRTUAL(CProfileNameDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

// message handlers
protected:
	//{{AFX_MSG(CProfileNameDialog)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// dialog field data
protected:
	//{{AFX_DATA(CProfileNameDialog)
	enum { IDD = IDD_PROFILE_NAME };
	CString	m_strName;
	//}}AFX_DATA
};


// inlines
inline
CString CProfileNameDialog::GetName() const
{
	return m_strName;
}


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ f�gt unmittelbar vor der vorhergehenden Zeile zus�tzliche Deklarationen ein.

#endif // AFX_PROFILENAMEDIALOG_H__F9A231A0_1F87_11D5_A222_006097239934__INCLUDED_
