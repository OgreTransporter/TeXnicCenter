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

#include "LatexProject.h"
#include "configuration.h"
#include "NavigatorView.h"
#include "RunTimeHelper.h"
#include "OleDrop.h"


class NavigatorTreeCtrl::DragSource :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDropSourceImpl<DragSource>
{
public:
	BEGIN_COM_MAP(DragSource)
		COM_INTERFACE_ENTRY(IDropSource)
	END_COM_MAP()
};

BEGIN_MESSAGE_MAP(NavigatorTreeCtrl,CTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED,&NavigatorTreeCtrl::OnSelchanged)
	ON_WM_CREATE()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT(NM_DBLCLK, &NavigatorTreeCtrl::OnNMDblclk)
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, &NavigatorTreeCtrl::OnTvnBeginDrag)
END_MESSAGE_MAP()


NavigatorTreeCtrl::NavigatorTreeCtrl()
: m_bFirstTime(TRUE)
, drag_source_(0)
{
	m_images.Attach(::ImageList_LoadImage(AfxGetInstanceHandle(),
	                        MAKEINTRESOURCE(IDB_ITEM_TYPES),16,1,RGB(255,0,255),IMAGE_BITMAP,
	                        LR_CREATEDIBSECTION));
}

NavigatorTreeCtrl::~NavigatorTreeCtrl()
{
}

BOOL NavigatorTreeCtrl::Create(CWnd *pwndParent)
{
	ASSERT(pwndParent && IsWindow(pwndParent->m_hWnd));
	if (!pwndParent || !IsWindow(pwndParent->m_hWnd))
		return FALSE;

	CRect rect;
	pwndParent->GetClientRect(rect);

	DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_LINESATROOT | TVS_DISABLEDRAGDROP;
	bool vista = RunTimeHelper::IsVista();

	if (!vista)
		style |= TVS_HASLINES;

	// Override, Windows XP or higher theme thin edge
	BOOL result = CWnd::CreateEx(WS_EX_CLIENTEDGE,WC_TREEVIEW,0,style,rect,pwndParent,0);

	if (result && vista)
	{
		::SetWindowTheme(m_hWnd,L"explorer",0);
		const DWORD style = TVS_EX_DOUBLEBUFFER | TVS_EX_FADEINOUTEXPANDOS | TVS_EX_AUTOHSCROLL;
		SendMessage(TVM_SETEXTENDEDSTYLE,style,style);
	}

	return result;
}

CString NavigatorTreeCtrl::GetItemPath(HTREEITEM hItem) const
{
	if (!hItem)
		return CString(_T(""));

	CString strPath = GetItemText(hItem);
	HTREEITEM hParent = hItem;

	while ((hParent = GetParentItem(hParent)) != NULL)
		strPath = GetItemText(hParent) + _T('\n') + strPath;

	return strPath;
}

HTREEITEM NavigatorTreeCtrl::GetItemByPath(LPCTSTR lpszPath) const
{
	ASSERT(lpszPath);

	HTREEITEM hItem = GetNextItem(NULL,TVGN_ROOT);
	CString strPath(lpszPath);
	CString strSubString;
	int nSeparator;
	BOOL bFound = FALSE;

	// get text of first item to find
	nSeparator = strPath.Find(_T('\n'));
	strSubString = strPath.Left((nSeparator == -1) ? strPath.GetLength() : nSeparator);
	if (nSeparator == -1)
		strPath.Empty();
	else
		strPath = strPath.Right(strPath.GetLength() - nSeparator - 1);

	// get item with specified path
	while (hItem && !bFound)
	{
		// find item in the current plain
		if (GetItemText(hItem) == strSubString)
		{
			if (strPath.IsEmpty())
				return hItem;
			else if (ItemHasChildren(hItem))
			{
				hItem = GetChildItem(hItem);

				// get text of next item to find
				nSeparator = strPath.Find(_T('\n'));
				strSubString = strPath.Left((nSeparator == -1) ? strPath.GetLength() : nSeparator);
				if (nSeparator == -1)
					strPath.Empty();
				else
					strPath = strPath.Right(strPath.GetLength() - nSeparator - 1);
			}
			else
				hItem = GetNextSiblingItem(hItem);
		}
		else
			hItem = GetNextSiblingItem(hItem);
	}

	return hItem;
}

void NavigatorTreeCtrl::GetExpandedItems(CStringArray &astrExpandedItems) const
{
	HTREEITEM hItem = GetNextItem(NULL,TVGN_ROOT);
	BOOL bStart = TRUE;

	astrExpandedItems.RemoveAll();

	while ((hItem = GetNextExpandedItem(hItem,bStart)))
	{
		astrExpandedItems.Add(GetItemPath(hItem));
		bStart = FALSE;
	}
}

void NavigatorTreeCtrl::ExpandItems(const CStringArray &astrItems)
{
	HTREEITEM hItem;

	for (int i = 0; i < astrItems.GetSize(); i++)
	{
		hItem = GetItemByPath(astrItems[i]);
		if (hItem)
			Expand(hItem,TVE_EXPAND);
	}
}

void NavigatorTreeCtrl::ExpandItemsByLevel(const int nLevel)
{
	//Safety
	if (nLevel < 0)
	{
		ASSERT(false);
		return;
	}

	//We start at the root with level zero
	// and traverse the tree in a depth-first manner.
	HTREEITEM hItem = GetNextItem(NULL,TVGN_ROOT);
	bool bLookForParent(false);
	int currLevel = 0;
	while (currLevel >= 0)
	{
		if ((currLevel <= nLevel) && !bLookForParent)
		{
			//Expand this item, if it has children
			//Furthermore, go down.
			if (ItemHasChildren(hItem))
			{
				Expand(hItem,TVE_EXPAND);
				hItem = GetChildItem(hItem);
				currLevel++;
				continue;
			}
		}
		else
		{
			//We reached the max level or are looking explicitly for a parent
			hItem = GetParentItem(hItem);
			--currLevel;
			bLookForParent = false;
		}

		//Current item has no children or is a parent that we already visited/expanded.
		//So we're looking for its next sibling.
		//If it does not exist, we go up in the tree.
		HTREEITEM hTemp = GetNextSiblingItem(hItem);
		if (hTemp)
		{
			hItem = hTemp;
		}
		else
		{
			bLookForParent = true;
		}
	}
}

HTREEITEM NavigatorTreeCtrl::GetNextExpandedItem(HTREEITEM hItem,BOOL bInclude /*= FALSE*/) const
{
	HTREEITEM hFound = hItem;

	do
	{
		if (!bInclude)
		{
			if (ItemHasChildren(hFound))
				hFound = GetChildItem(hFound);
			else
			{
				HTREEITEM hItem = hFound;
				hFound = GetNextSiblingItem(hItem);
				if (!hFound)
				{
					do
					{
						hFound = hItem = GetParentItem(hItem);
						if (!hFound)
							break;
					}
					while ((hFound = GetNextSiblingItem(hFound)) == NULL);
				}
			}
		}
		else
			bInclude = FALSE;
	}
	while (hFound && !(GetItemState(hFound,TVIS_EXPANDED) & TVIS_EXPANDED));

	return hFound;
}

int NavigatorTreeCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTreeCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	// set font
	UpdateFont();

	// modify list ctrl
	SetImageList(&m_images,TVSIL_NORMAL);
	SetImageList(&m_images,TVSIL_STATE);
	ModifyStyle(0,TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP);
	m_bFirstTime = FALSE;

	EnableDragDrop();

	return 0;
}

void NavigatorTreeCtrl::OnSelchanged(NMHDR* pNMHDR,LRESULT* pResult)
{
	if (CLaTeXProject* p = GetProject())
		p->SetCurrentStructureItem(((NM_TREEVIEW*)pNMHDR)->itemNew.lParam);

	*pResult = 0;
}

void NavigatorTreeCtrl::UpdateFont()
{
	m_font.DeleteObject();
	m_font.CreateFontIndirect(&CConfiguration::GetInstance()->m_fontNavigator);
	SetFont(&m_font);
}

void NavigatorTreeCtrl::OnSysColorChange()
{
	CTreeCtrl::OnSysColorChange();
	UpdateFont();
}

void NavigatorTreeCtrl::Clear(void)
{
	// No animation
	SetRedraw(FALSE);
	DeleteAllItems();
	SetRedraw();
	Invalidate();
}

void NavigatorTreeCtrl::OnNMDblclk(NMHDR* /*pNMHDR*/, LRESULT *pResult)
{
	HTREEITEM hItem = GetSelectedItem();

	if (hItem)
		GetProject()->SetCurrentStructureItem(GetItemData(hItem));

	GotoItem();

	*pResult = 0;
}

void NavigatorTreeCtrl::OnDestroy()
{
	if (IsDragDropEnabled())
		EnableDragDrop(false);

	if (IsAttached())
		GetProject()->RemoveView(this);

	CTreeCtrl::OnDestroy();
}

void NavigatorTreeCtrl::ShowContextMenu(CPoint &point)
{
	// handle only if generated by key (context menu key on win95 keyboard)
	// (mouse is handled by OnRButtonDown() -- problems with messages from CCJTabCtrlBar!)
	if (point.x == -1 && point.y == -1)
	{
		HTREEITEM hItem = GetSelectedItem();
		CRect rect;

		if (hItem && GetItemRect(hItem,rect,TRUE))
		{
			ClientToScreen(rect);
			theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_ITEM,rect.right,rect.bottom,this);
		}
	}
	else {
		CPoint pt = point;
		ScreenToClient(&pt);
		HTREEITEM hItem = HitTest(pt);	

		if (hItem)
		{
			SelectItem(hItem);
			theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_ITEM,point.x,point.y,this);
		}
		else
			theApp.m_pMainWnd->SendMessage(AFX_WM_TOOLBARMENU, 0,MAKELPARAM(point.x,point.y));
	}
}

void NavigatorTreeCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	ShowContextMenu(point);
}

void NavigatorTreeCtrl::EnableDragDrop( bool enable /*= true*/ )
{
	if (enable != IsDragDropEnabled())
	{
		if (enable) 
			ModifyStyle(TVS_DISABLEDRAGDROP,0);
		else 
		{		
			ModifyStyle(0,TVS_DISABLEDRAGDROP);

			if (drag_source_) {
				drag_source_->Release();
				drag_source_ = 0;
			}
		}
	}
}

bool NavigatorTreeCtrl::IsDragDropEnabled() const
{
	return (GetStyle() & TVS_DISABLEDRAGDROP) != TVS_DISABLEDRAGDROP;
}

BOOL NavigatorTreeCtrl::PreTranslateMessage(MSG* pMsg)
{
	BOOL handled = FALSE;

	if (pMsg->hwnd == m_hWnd && pMsg->message == WM_KEYDOWN) 
	{
		handled = TRUE;

		switch (pMsg->wParam) 
		{
			case VK_RETURN:
				GotoItem();
				break;
			case VK_SPACE:
				OnContextMenu(this,CPoint(-1,-1));
				break;
			default:
				handled = FALSE;
		}
	}
	
	if (!handled)
		handled = CTreeCtrl::PreTranslateMessage(pMsg);

	return handled;
}

void NavigatorTreeCtrl::GotoItem()
{
	AfxGetMainWnd()->SendMessage(WM_COMMAND,ID_ITEM_GOTO);
}

void NavigatorTreeCtrl::OnTvnBeginDrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	
	if (!drag_source_) 
		drag_source_ = new DataDragSource;

	if (drag_source_) 
	{
		CString text;
		DWORD effects;

		HTREEITEM item = pNMTreeView->itemNew.hItem;

		if (OnBeginDragDrop(item,text))
			effects = DROPEFFECT_COPY;
		else
			effects = 0;

		typedef CComObjectNoLock<SimpleDataObject> DataObject;

		DataObject* object = new DataObject;
		VERIFY(SUCCEEDED(object->SetData(text,text.GetLength())));
		
		DWORD result;
		::DoDragDrop(object,drag_source_,effects,&result);

		OnEndDragDrop(item);
	}

	*pResult = 0;
}

bool NavigatorTreeCtrl::OnBeginDragDrop(HTREEITEM item, CString& text)
{
	text.Empty();
	bool result = false;

	if (CLaTeXProject* p = theApp.GetProject())
	{
		const StructureItemContainer::size_type pos = 
			static_cast<StructureItemContainer::size_type>(GetItemData(item));

		if (pos < p->GetStructureItems().size()) {
			const CStructureItem& si = p->GetStructureItems()[pos];
			UINT state = 0;

			if (GetKeyState(VK_CONTROL) >> 15 & 1) // Ctrl pressed
				state |= MK_CONTROL;

			if (GetKeyState(VK_SHIFT) >> 15 & 1) // Shift pressed
				state |= MK_SHIFT;

			result = OnBeginDragDrop(si,text,state);
		}
	}

	if (result)
		static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(text);

	return result;
}

void NavigatorTreeCtrl::OnEndDragDrop(HTREEITEM /*item*/)
{
	static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(AFX_IDS_IDLEMESSAGE);
}

CLaTeXProject * NavigatorTreeCtrl::GetProject() const
{
	return (CLaTeXProject*)CProjectView::GetProject();
}

bool NavigatorTreeCtrl::OnBeginDragDrop(const CStructureItem& item, CString& text, UINT keystate)
{
	if (item.HasLabels()) 
	{
		switch (keystate) 
		{
			case 0: // \ref{label}
				text = CLaTeXProject::FormatRef(item);
				break;
			case MK_CONTROL: // label
				text = item.GetLabel();
				break; // \pageref{label}
			case MK_CONTROL|MK_SHIFT:
				text = CLaTeXProject::FormatPageRef(item);
				break;
		}
	}

	return !text.IsEmpty();
}
