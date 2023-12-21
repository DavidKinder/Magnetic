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
#include "MagneticDlg.h"
#include "MagneticDoc.h"
#include "MagneticView.h"
#include "MainFrm.h"
#include "DpiFunctions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Implementation of CMainFrame
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CMainFrame, MenuBarFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, MenuBarFrameWnd)
  //{{AFX_MSG_MAP(CMainFrame)
  ON_WM_CREATE()
  ON_COMMAND(ID_HELP, OnHelpFinder)
  ON_WM_PALETTECHANGED()
  ON_WM_QUERYNEWPALETTE()
  ON_WM_SETTINGCHANGE()
  //}}AFX_MSG_MAP
  ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
END_MESSAGE_MAP()

static UINT indicators[] =
{
  ID_SEPARATOR,           // status line indicator
  ID_INDICATOR_CAPS,
  ID_INDICATOR_NUM,
};

CMainFrame::CMainFrame() : m_dpi(96), m_modalDialog(NULL)
{
}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::SetModalDialog(CWnd* dialog)
{
  m_modalDialog = dialog;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
  CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();

  m_dpi = DPI::getWindowDPI(this);

  // Now we have a window, set the font height
  pApp->GetLogFont()->lfHeight = -MulDiv(pApp->GetFontPoints(),m_dpi,72);

  // Restore the window size and position from DPI neutral values
  CRect rPlace = pApp->GetWindowRect();
  if (rPlace.Width() > 0)
  {
    DPI::ContextUnaware dpiUnaware;
    MoveWindow(rPlace);
  }

  m_menuBar.AddNoIconId(ID_FILE_RECORD);
  m_menuBar.AddNoIconId(ID_FILE_PLAYBACK);
  m_menuBar.AddNoIconId(ID_FILE_SCRIPT);

  if (MenuBarFrameWnd::OnCreate(lpCreateStruct) == -1)
    return -1;
  if (!CreateNewBar(IDR_MAINFRAME,IDR_TOOLBAR))
    return -1;

  if (!m_statusBar.Create(this) ||
      !m_statusBar.SetIndicators(indicators,sizeof(indicators)/sizeof(UINT)))
  {
    return -1;
  }

  BOOL bToolBar, bStatusBar;
  pApp->GetControlBars(bToolBar,bStatusBar);
  ShowControlBar(&m_toolBar,bToolBar,TRUE);
  ShowControlBar(&m_statusBar,bStatusBar,TRUE);

  // Turn on dark mode, if needed
  SetDarkMode(DarkMode::GetEnabled(DARKMODE_REGISTRY));
  return 0;
}

BOOL CMainFrame::DestroyWindow() 
{
  CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();

  // Save the window position in a DPI neutral form
  WINDOWPLACEMENT Place;
  {
    DPI::ContextUnaware dpiUnaware;
    GetWindowPlacement(&Place);
  }

  int& iMax = pApp->GetWindowMax();
  CRect& rPlace = pApp->GetWindowRect();

  iMax = (Place.showCmd == SW_SHOWMAXIMIZED);
  rPlace = Place.rcNormalPosition;

  BOOL bToolBar = m_toolBar.GetStyle() & WS_VISIBLE;
  BOOL bStatusBar = m_statusBar.GetStyle() & WS_VISIBLE;
  pApp->SetControlBars(bToolBar,bStatusBar);

  return MenuBarFrameWnd::DestroyWindow();
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
  cs.style &= ~FWS_ADDTOTITLE;
  return MenuBarFrameWnd::PreCreateWindow(cs);
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

void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
  MenuBarFrameWnd::OnSettingChange(uFlags,lpszSection);

  if ((m_dark != NULL) != DarkMode::IsEnabled(DARKMODE_REGISTRY))
  {
    SetDarkMode(DarkMode::GetEnabled(DARKMODE_REGISTRY));
    if (m_modalDialog != NULL)
    {
      if (m_modalDialog->IsKindOf(RUNTIME_CLASS(CMagneticDlg)))
        ((CMagneticDlg*)m_modalDialog)->SetDarkMode(DarkMode::GetActive(m_modalDialog));
    }

    CMagneticView* pView = CMagneticView::GetView();
    if (pView)
    {
      CPictureWnd& PicWnd = pView->GetPictureWindow();
      if (PicWnd.GetSafeHwnd() != 0)
        DarkMode::SetDarkTitle(&PicWnd,m_dark != NULL);
    }

    DarkMode::SetAppDarkMode(m_dark);
  }
}

LRESULT CMainFrame::OnDpiChanged(WPARAM wparam, LPARAM lparam)
{
  MoveWindow((LPRECT)lparam,TRUE);

  int newDpi = (int)HIWORD(wparam);
  if (m_dpi != newDpi)
  {
    CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
    pApp->GetLogFont()->lfHeight = -MulDiv(pApp->GetFontPoints(),newDpi,72);

    CMagneticView* pView = CMagneticView::GetView();
    if (pView)
    {
      pView->TextClearup();
      pView->TextSetup();
      pView->Invalidate();
    }
    m_dpi = newDpi;
  }

  // Force the menu and status bars to update
  UpdateDPI(newDpi);
  m_statusBar.SetIndicators(indicators,sizeof(indicators)/sizeof(UINT));
  return 0;
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
  MenuBarFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
  MenuBarFrameWnd::Dump(dc);
}

#endif //_DEBUG
