/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 2
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// MainFrm.cpp: Implementation of the frame class
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "Magnetic.h"
#include "MagneticDoc.h"
#include "MagneticView.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Implementation of CMainFrame
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_HELP, OnHelpFinder)
	ON_WM_MEASUREITEM()
	ON_WM_MENUCHAR()
	ON_WM_INITMENUPOPUP()
	ON_WM_PALETTECHANGED()
	ON_WM_QUERYNEWPALETTE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this,TBSTYLE_FLAT,
		WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS|CBRS_FLYBY))
		return -1;
	if (!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
		return -1;

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	BOOL bToolBar, bStatusBar;
	pApp->GetControlBars(bToolBar,bStatusBar);
	ShowControlBar(&m_wndToolBar,bToolBar,TRUE);
	ShowControlBar(&m_wndStatusBar,bStatusBar,TRUE);

	return 0;
}

BOOL CMainFrame::DestroyWindow() 
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();

	// Save the window position
	WINDOWPLACEMENT Place;
	GetWindowPlacement(&Place);

	int& iMax = pApp->GetWindowMax();
	CRect& rPlace = pApp->GetWindowRect();

	iMax = (Place.showCmd == SW_SHOWMAXIMIZED);
	rPlace = Place.rcNormalPosition;

	BOOL bToolBar = m_wndToolBar.GetStyle() & WS_VISIBLE;
	BOOL bStatusBar = m_wndStatusBar.GetStyle() & WS_VISIBLE;
	pApp->SetControlBars(bToolBar,bStatusBar);

	return CFrameWnd::DestroyWindow();
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();

	cs.style &= ~FWS_ADDTOTITLE;

	CRect& rPlace = pApp->GetWindowRect();
	if (rPlace.Width() > 0)
	{
		cs.x = rPlace.left;
		cs.y = rPlace.top;
		cs.cx = rPlace.Width();
		cs.cy = rPlace.Height();
	}

	return CFrameWnd::PreCreateWindow(cs);
}

HMENU CMainFrame::NewMenu()
{
	m_menu.LoadMenu(IDR_MAINFRAME);
	m_menu.LoadToolbar(IDR_MAINFRAME);
	return m_menu.Detach();
}

void CMainFrame::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	BOOL setflag = FALSE;

	if (lpMeasureItemStruct->CtlType == ODT_MENU)
	{
		if (IsMenu((HMENU)lpMeasureItemStruct->itemID))
		{
			CMenu* cmenu = CMenu::FromHandle((HMENU)lpMeasureItemStruct->itemID);
			if (m_menu.IsMenu(cmenu))
			{
				m_menu.MeasureItem(lpMeasureItemStruct);
				setflag = TRUE;
			}
		}
	}
	if (!setflag)
		CFrameWnd::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

LRESULT CMainFrame::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu) 
{
	LRESULT lresult;

	if (m_menu.IsMenu(pMenu))
		lresult = BCMenu::FindKeyboardShortcut(nChar, nFlags, pMenu);
	else
		lresult = CFrameWnd::OnMenuChar(nChar, nFlags, pMenu);
	return lresult;
}

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu) 
{
	CFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
	if (!bSysMenu)
	{
		if (m_menu.IsMenu(pPopupMenu))
			BCMenu::UpdateMenu(pPopupMenu);
	}
}

void CMainFrame::OnPaletteChanged(CWnd*) 
{
	CMagneticView* pView = CMagneticView::GetView();
	if (pView == NULL)
		return;

	CDC* pDC = GetDC();
	pView->GetPicture().SetPalette(pDC,this);
	ReleaseDC(pDC);
}

BOOL CMainFrame::OnQueryNewPalette() 
{
	CMagneticView* pView = CMagneticView::GetView();
	if (pView == NULL)
		return 0;

	CDC* pDC = GetDC();
	int iColours = pView->GetPicture().SetPalette(pDC,this);
	ReleaseDC(pDC);
	return iColours;
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG
