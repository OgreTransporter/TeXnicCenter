/********************************************************************
*
* This file is part of the TeXnicCenter-system
*
* Copyright (C) 2002-$CurrentYear$ Chris Norris
* Copyright (C) 2002-$CurrentYear$ ToolsCenter
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

// CBackgroundThread.cpp: implementation of the CBackgroundThread class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BackgroundThread.h"
#include "CCrystalTextBuffer.h"
#include "editcmd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE( CBackgroundThread, CWinThread )

BEGIN_MESSAGE_MAP( CBackgroundThread, CWinThread )
	ON_MESSAGE(ID_BG_UPDATE_FILE, OnUpdateFile)
	ON_MESSAGE(ID_BG_UPDATE_LINE, OnUpdateLine)
	ON_MESSAGE(ID_BG_RESET_SPELLER, OnGetSpeller)
	ON_MESSAGE(ID_BG_ENABLE_SPELLER, OnEnableSpeller)
	ON_MESSAGE(ID_BG_INVALIDATE_VIEW, OnInvalidateView)
END_MESSAGE_MAP()


CBackgroundThread::CBackgroundThread() 
: m_evtDone(FALSE, TRUE, NULL, NULL)
{
	m_pSpell = NULL;
	m_nBusyCount = 0;
	m_bSpellEnabled = false;
}


CBackgroundThread::~CBackgroundThread()
{
	ASSERT( m_nBusyCount == 0 );
}


//////////////////////////////////////////////////////////////////////
// message handlers

LRESULT CBackgroundThread::OnUpdateFile(WPARAM wParam, LPARAM lParam)
{
	CCrystalTextView *pTextView = (CCrystalTextView *)lParam;
	ASSERT( pTextView );
	if ( IsValidView(pTextView) )
		if ( m_bSpellEnabled )
			SpellCheckBuffer( pTextView );
	return 0;
}


LRESULT CBackgroundThread::OnUpdateLine(WPARAM wParam, LPARAM lParam)
{
	CCrystalTextView *pTextView = (CCrystalTextView *)lParam;
	ASSERT( pTextView );
	int nLine = (int)wParam;
	if ( IsValidView(pTextView) )
		if ( m_bSpellEnabled )
			SpellCheckSingleLine( pTextView, nLine );
	return 0;
}


LRESULT CBackgroundThread::OnGetSpeller(WPARAM wParam, LPARAM lParam)
{
	CSpellerSource *pSource = (CSpellerSource *)lParam;
	if ( pSource )
	{
		MySpell *pSpell = pSource->GetSpeller();
		SetSpeller(pSpell);
	}
	return 0;
}


LRESULT CBackgroundThread::OnEnableSpeller(WPARAM wParam, LPARAM lParam)
{
	BOOL bSpellEnabled = (BOOL)wParam;
	if ( bSpellEnabled == m_bSpellEnabled )
		return 0;

	// Update the current view
	CCrystalTextView *pTextView = (CCrystalTextView *)lParam;
	if ( pTextView )
		if ( bSpellEnabled )
			SpellCheckBuffer( pTextView );
		else
			RemoveBufferAttributes( pTextView, CCrystalTextBuffer::CTextAttribute::spellError);

	m_bSpellEnabled = bSpellEnabled;
	return 0;	
}


LRESULT CBackgroundThread::OnInvalidateView(WPARAM wParam, LPARAM lParam)
{
	m_pInvalidViews.AddHead((void *)lParam);
	return 0;
}

//////////////////////////////////////////////////////////////////////
// overrides

BOOL CBackgroundThread::InitInstance( )
{
	return TRUE;
}


BOOL CBackgroundThread::OnIdle(LONG lCount)
{
	m_pInvalidViews.RemoveAll();
	return 0;
}


void CBackgroundThread::SetSpeller(MySpell *pSpell)
{
	m_pSpell = pSpell;
}


void CBackgroundThread::SpellCheckSingleLine(CCrystalTextView *pTextView, int nLine)
{
	if ( m_pSpell == NULL )
		return;

	ASSERT( pTextView );
	pTextView->Attach();

	::InterlockedIncrement( &m_nBusyCount );
	m_evtDone.ResetEvent();

	char pWordBuffer[MAXWORDLEN+1];
	CCrystalParser *pParser = pTextView->GetParser();
	CCrystalTextBuffer *pBuffer = pTextView->LocateTextBuffer();
	ASSERT( nLine < pBuffer->GetLineCount() );

	if ( !(nLine < pBuffer->GetLineCount()) )
	{
		pTextView->Detach();
		return; // Done
	}

	pBuffer->ClearLineAttributes( nLine, CCrystalTextBuffer::CTextAttribute::spellError );
	LPTSTR szLine = pBuffer->GetLineChars( nLine );
	int nStart=0, nEnd=0;
	pParser->NextWord( nLine, nStart, nEnd );

	while ( nStart != -1 )
	{
		if ( (nStart - nEnd) <  MAXWORDLEN )
		{
			int i = nStart;
			int j = 0;
			// Convert string to char*
			while ( i < nEnd  )
				pWordBuffer[j++] = (char)szLine[i++];
			pWordBuffer[j] = 0;
			if ( !m_pSpell->spell(pWordBuffer) )
			{
				CCrystalTextBuffer::CTextAttribute attribute( nStart, nEnd, CCrystalTextBuffer::CTextAttribute::spellError );
				pBuffer->InsertLineAttribute( nLine, attribute );
			}
		}
		nStart = nEnd;
		pParser->NextWord( nLine, nStart, nEnd) ;
	}

	::InterlockedDecrement( &m_nBusyCount );
	if ( m_nBusyCount == 0 )
		m_evtDone.SetEvent();

	pBuffer->UpdateViews( NULL, NULL, UPDATE_LINEATTR, nLine );
	pTextView->Detach();
}


void CBackgroundThread::SpellCheckBuffer(CCrystalTextView *pTextView)
{
	if ( m_pSpell == NULL )
		return;

	ASSERT( pTextView );
	pTextView->Attach();

	::InterlockedIncrement( &m_nBusyCount );
	m_evtDone.ResetEvent();

	int nLineIndex = 0;
	char pWordBuffer[MAXWORDLEN+1];
	CCrystalParser *pParser = pTextView->GetParser();
	CCrystalTextBuffer *pBuffer = pTextView->LocateTextBuffer();
	while ( nLineIndex < pBuffer->GetLineCount() )
	{
		pBuffer->ClearLineAttributes( nLineIndex, CCrystalTextBuffer::CTextAttribute::spellError );
		LPTSTR szLine = pBuffer->GetLineChars( nLineIndex );
		int nStart=0, nEnd=0;
		pParser->NextWord( nLineIndex, nStart, nEnd );

		while ( nStart != -1 )
		{
			if ( (nStart - nEnd) <  MAXWORDLEN )
			{
				int i = nStart;
				int j = 0;
				// Convert string to char*
				while ( i < nEnd  )
					pWordBuffer[j++] = (char)szLine[i++];
				pWordBuffer[j] = 0;
				if ( !m_pSpell->spell(pWordBuffer) )
				{
					CCrystalTextBuffer::CTextAttribute attribute( nStart, nEnd, CCrystalTextBuffer::CTextAttribute::spellError );
					pBuffer->InsertLineAttribute( nLineIndex, attribute );
				}
			}
			nStart = nEnd;
			pParser->NextWord( nLineIndex, nStart, nEnd);
		}
		pBuffer->UpdateViews( NULL, NULL, UPDATE_LINEATTR, nLineIndex );
		++nLineIndex;
	}

	::InterlockedDecrement( &m_nBusyCount );
	if ( m_nBusyCount == 0 )
		m_evtDone.SetEvent();

	pTextView->Detach();
	return;
}


void CBackgroundThread::RemoveBufferAttributes(CCrystalTextView *pTextView, CCrystalTextBuffer::CTextAttribute::tagAttribute attrType)
{
	ASSERT( pTextView );
	pTextView->Attach();

	CCrystalTextBuffer *pBuffer = pTextView->LocateTextBuffer();
	int nLineIndex = 0;
	while ( nLineIndex < pBuffer->GetLineCount() )
		pBuffer->ClearLineAttributes(nLineIndex++, attrType);

	pTextView->Detach();
}


BOOL CBackgroundThread::IsValidView(void *pView)
{
	// This in affect gives the ID_BG_INVALIDATE_VIEW messge the highest priority.
	MSG msg;
	if ( ::PeekMessage( &msg, this->m_pMainWnd->m_hWnd, ID_BG_INVALIDATE_VIEW, ID_BG_INVALIDATE_VIEW, TRUE) )
		OnInvalidateView( msg.wParam , msg.lParam );

	POSITION pos = m_pInvalidViews.GetHeadPosition();
	while ( pos != NULL )
	{
		if ( pView == m_pInvalidViews.GetNext(pos) )
			return FALSE;
	}
	return TRUE;
}
