/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 2
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// Magnetic.cpp: Implementation of application class
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <string.h>
#include <stdlib.h>

#include "Magnetic.h"
#include "MagneticDoc.h"
#include "MagneticView.h"
#include "MainFrm.h"
#include "OptionsDlg.h"
#include "Dialogs.h"
#include "ImagePNG.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL AFXAPI _AfxSetRegKey(LPCTSTR lpszKey, LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);
void AFXAPI AfxGetModuleShortFileName(HINSTANCE hInst, CString& strShortName);

/////////////////////////////////////////////////////////////////////////////
// Implementation of CMagneticApp
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CMagneticApp, CWinApp)
  //{{AFX_MSG_MAP(CMagneticApp)
  ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
  ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
  ON_COMMAND(ID_VIEW_FONT, OnViewFont)
  ON_UPDATE_COMMAND_UI(ID_FILE_MRU_FILE1, OnUpdateRecentFileMenu)
  ON_COMMAND(ID_VIEW_OPTIONS, OnViewOptions)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CMagneticApp theApp;

CMagneticApp::CMagneticApp()
{
  EnableHtmlHelp();
}

BOOL CMagneticApp::InitInstance()
{
  ::CoInitialize(NULL);
  AfxEnableControlContainer();
  AfxInitRichEdit2();

  SetRegistryKey(_T("David Kinder"));
  LoadStdProfileSettings();

  NONCLIENTMETRICS ncm;
  ::ZeroMemory(&ncm,sizeof ncm);
  ncm.cbSize = sizeof ncm;
  ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof ncm,&ncm,0);

  CRect screen = DPI::getMonitorRect(CWnd::GetDesktopWindow());
  int scalePics = 100;
  if ((screen.Width() > 800) && (screen.Height() > 600))
    scalePics = 200;

  // Load Magnetic display settings
  int fontSize = GetProfileInt("Display","Font Size",10);
  if (fontSize < 0)
    m_iFontPoints = -MulDiv(fontSize,72,DPI::getSystemDPI());
  else
    m_iFontPoints = fontSize;
  m_LogFont.lfHeight = -MulDiv(m_iFontPoints,DPI::getSystemDPI(),72);
  m_LogFont.lfCharSet = ANSI_CHARSET;
  m_LogFont.lfOutPrecision = OUT_TT_PRECIS;
  m_LogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  m_LogFont.lfQuality = PROOF_QUALITY;
  m_LogFont.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
  strncpy(m_LogFont.lfFaceName,GetProfileString(
    "Display","Font Name",ncm.lfMessageFont.lfFaceName),LF_FACESIZE);

  SetRedrawStatus(Redraw::NoRedraw);

  m_WindowRect.left = GetProfileInt("Window","Left",0);
  m_WindowRect.top = GetProfileInt("Window","Top",0);
  m_WindowRect.right = GetProfileInt("Window","Right",0);
  m_WindowRect.bottom = GetProfileInt("Window","Bottom",0);
  m_iWindowMax = GetProfileInt("Window","Maximized",0);
  if (m_iWindowMax)
    m_nCmdShow = SW_SHOWMAXIMIZED;
  m_bToolBar = GetProfileInt("Window","Toolbar",1) ? TRUE : FALSE;
  m_bStatusBar = GetProfileInt("Window","Status Bar",1) ? TRUE : FALSE;

  m_PicTopLeft.x = GetProfileInt("Picture","Left",0);
  m_PicTopLeft.y = GetProfileInt("Picture","Top",0);

  m_ShowGfx = (ShowGraphics)GetProfileInt("Picture","Show",
    ShowGraphics::MainWindow);
  m_dScaleFactor = (double)GetProfileInt("Picture","Scale",scalePics)*0.01;
  m_dGamma = (double)GetProfileInt("Picture","Gamma",100)*0.01;
  m_ForeColour = GetProfileInt("Display","Foreground",~0);
  m_BackColour = GetProfileInt("Display","Background",~0);
  m_GfxColour = GetProfileInt("Display","Graphics",~0);
  m_bHintWindow = GetProfileInt("Hints","Use hint window",1);
  m_bAnimWait = GetProfileInt("Debug","Wait for animations",0);
  m_bPredict = GetProfileInt("Debug","Predictable",0);
  m_iSeed = GetProfileInt("Debug","Seed",0);

  m_HintsRect.left = GetProfileInt("Hints","Left",0);
  m_HintsRect.top = GetProfileInt("Hints","Top",0);
  m_HintsRect.right = GetProfileInt("Hints","Right",0);
  m_HintsRect.bottom = GetProfileInt("Hints","Bottom",0);

  m_iGameLoaded = 0;

  CSingleDocTemplate* pDocTemplate;
  pDocTemplate = new CSingleDocTemplate(
    IDR_MAINFRAME,
    RUNTIME_CLASS(CMagneticDoc),
    RUNTIME_CLASS(CMainFrame),
    RUNTIME_CLASS(CMagneticView));
  AddDocTemplate(pDocTemplate);

  EnableShellOpen();
  RegisterShellFileTypes(FALSE);

  // Set up the icon for Magnetic game files
  CString path, type, key, value;
  AfxGetModuleShortFileName(AfxGetInstanceHandle(),path);
  pDocTemplate->GetDocString(type,CDocTemplate::regFileTypeId);
  key.Format("%s\\DefaultIcon",type.GetString());
  value.Format("%s,%d",path.GetString(),0);
  _AfxSetRegKey(key,value);

  // Notify the shell that associations have changed
  ::SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,0,0);

  // Turn on dark mode for the application, if necessary
  if (DarkMode::IsEnabled(DARKMODE_REGISTRY))
    DarkMode::SetAppDarkMode();

  // Create file dialog for loading games
  m_pNewGameDialog = new SimpleFileDialog(TRUE,NULL,
    GetProfileString("Settings","Last File",""),
    OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_ENABLESIZING,
    "Magnetic Files (*.mag)|*.mag|All Files (*.*)|*.*||",NULL);
  if (m_pNewGameDialog == NULL)
    return FALSE;
  m_pNewGameDialog->m_ofn.lpstrTitle = "Open a Magnetic Scrolls game";

  CCommandLineInfo cmdInfo;
  ParseCommandLine(cmdInfo);

  if (!ProcessShellCommand(cmdInfo))
    return FALSE;

  // Create font dialog
  m_pFontDialog = new DPI::FontDialog(&m_LogFont,CF_SCREENFONTS,m_pMainWnd);
  if (m_pFontDialog == NULL)
    return FALSE;

  m_pMainWnd->ShowWindow(SW_SHOW);
  m_pMainWnd->UpdateWindow();
  m_pMainWnd->DragAcceptFiles();
  return TRUE;
}

int CMagneticApp::ExitInstance() 
{
  // Write out settings
  WriteProfileString("Settings","Last File",m_pNewGameDialog->GetPathName());

  WriteProfileString("Display","Font Name",CString(m_LogFont.lfFaceName));
  WriteProfileInt("Display","Font Size",m_iFontPoints);
  WriteProfileInt("Display","Foreground",m_ForeColour);
  WriteProfileInt("Display","Background",m_BackColour);
  WriteProfileInt("Display","Graphics",m_GfxColour);

  WriteProfileInt("Hints","Use hint window",m_bHintWindow);

  WriteProfileInt("Debug","Wait for animations",m_bAnimWait);
  WriteProfileInt("Debug","Predictable",m_bPredict);
  WriteProfileInt("Debug","Seed",m_iSeed);

  WriteProfileInt("Window","Left",m_WindowRect.left);
  WriteProfileInt("Window","Top",m_WindowRect.top);
  WriteProfileInt("Window","Right",m_WindowRect.right);
  WriteProfileInt("Window","Bottom",m_WindowRect.bottom);
  WriteProfileInt("Window","Maximized",m_iWindowMax);
  WriteProfileInt("Window","Toolbar",m_bToolBar ? 1 : 0);
  WriteProfileInt("Window","Status Bar",m_bStatusBar ? 1 : 0);

  WriteProfileInt("Picture","Left",m_PicTopLeft.x);
  WriteProfileInt("Picture","Top",m_PicTopLeft.y);
  WriteProfileInt("Picture","Show",m_ShowGfx);
  WriteProfileInt("Picture","Scale",(int)(m_dScaleFactor*100));
  WriteProfileInt("Picture","Gamma",(int)(m_dGamma*100));

  WriteProfileInt("Hints","Left",m_HintsRect.left);
  WriteProfileInt("Hints","Top",m_HintsRect.top);
  WriteProfileInt("Hints","Right",m_HintsRect.right);
  WriteProfileInt("Hints","Bottom",m_HintsRect.bottom);

  // Free memory
  delete m_pNewGameDialog;
  delete m_pFontDialog;
  ms_freemem();

  return CWinApp::ExitInstance();
}

BOOL CMagneticApp::OnIdle(LONG lCount)
{
  BOOL bIdle = CWinApp::OnIdle(lCount);

  // Run interpeter opcodes. If the interpreter is running,
  // take more idle time.
  if (ms_rungame())
    bIdle = TRUE;
  else
  {
    if (m_iGameLoaded)
    {
      OnFileNew();
      CMagneticView* pView = CMagneticView::GetView();
      if (pView)
        pView->ClearAll();
    }
  }

  return bIdle;
}

void CMagneticApp::OnFileOpen() 
{
  if (ms_is_running())
    SetRedrawStatus(Redraw::ThisLine);

  if (m_pNewGameDialog->DoModal() == IDOK)
  {
    if (ms_is_running())
      SetRedrawStatus(Redraw::EndLine);
    OpenDocumentFile(m_pNewGameDialog->GetPathName());
  }
}

void CMagneticApp::OnViewFont() 
{
  SetRedrawStatus(Redraw::ThisLine);

  // Change the display font
  if (m_pFontDialog->DoModal() == IDOK)
  {
    m_iFontPoints = m_pFontDialog->m_cf.iPointSize / 10;

    CMagneticView* pView = CMagneticView::GetView();
    if (pView)
    {
      pView->TextClearup();
      pView->TextSetup();
      pView->Invalidate();
    }
  }
}

void CMagneticApp::OnViewOptions() 
{
  SetRedrawStatus(Redraw::ThisLine);

  COptionsDlg Options;

  // Set up initial values in the dialog
  Options.m_iShowPics = m_ShowGfx;
  Options.m_dScaleFactor = m_dScaleFactor;
  Options.m_dGamma = m_dGamma;
  Options.m_bHintWindow = m_bHintWindow;
  Options.m_bAnimWait = m_bAnimWait;
  Options.m_bPredict = m_bPredict;
  Options.m_iSeed = m_iSeed;

  if (Options.DoModal() == IDOK)
  {
    m_ShowGfx = (ShowGraphics)Options.m_iShowPics;
    m_dScaleFactor = Options.m_dScaleFactor;
    m_dGamma = Options.m_dGamma;
    m_bHintWindow = Options.m_bHintWindow;
    m_bAnimWait = Options.m_bAnimWait;
    m_bPredict = Options.m_bPredict;
    m_iSeed = Options.m_iSeed;

    m_ForeColour = Options.GetForeColour();
    m_BackColour = Options.GetBackColour();
    m_GfxColour = Options.GetGfxColour();

    DarkMode* dark = DarkMode::GetActive(AfxGetMainWnd());
    if (dark)
    {
      if (m_ForeColour == dark->GetColour(DarkMode::Fore))
        m_ForeColour = (COLORREF)~0;
      if (m_BackColour == dark->GetColour(DarkMode::Back))
        m_BackColour = (COLORREF)~0;
      if (m_GfxColour == dark->GetColour(DarkMode::Darkest))
        m_GfxColour = (COLORREF)~0;
    }
    else
    {
      if (m_ForeColour == GetSysColor(COLOR_WINDOWTEXT))
        m_ForeColour = (COLORREF)~0;
      if (m_BackColour == GetSysColor(COLOR_WINDOW))
        m_BackColour = (COLORREF)~0;
      if (m_GfxColour == GetSysColor(COLOR_APPWORKSPACE))
        m_GfxColour = (COLORREF)~0;
    }

    CMagneticView* pView = CMagneticView::GetView();
    if (pView)
      pView->SetPictureWindowState();
  }
}

void CMagneticApp::OnUpdateRecentFileMenu(CCmdUI* pCmdUI) 
{
  int nID = pCmdUI->m_nID;
  CWinApp::OnUpdateRecentFileMenu(pCmdUI);

  if (m_pRecentFileList == NULL)
    return;
  if (pCmdUI->m_pMenu == NULL)
    return;

  CMagneticView* pView = CMagneticView::GetView();
  if (pView)
  {
    if (pView->GetMorePrompt())
    {
      for (int iMRU = 0; iMRU < m_pRecentFileList->m_nSize; iMRU++)
        pCmdUI->m_pMenu->EnableMenuItem(nID + iMRU, MF_DISABLED|MF_GRAYED);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// Get and set program settings
/////////////////////////////////////////////////////////////////////////////

LOGFONT* CMagneticApp::GetLogFont(void)
{
  return &m_LogFont;
}

int CMagneticApp::GetFontPoints(void)
{
  return m_iFontPoints;
}

void CMagneticApp::SetRedrawStatus(Redraw Status)
{
  m_RedrawStatus = Status;
}

CMagneticApp::Redraw CMagneticApp::GetRedrawStatus(void)
{
  Redraw Return = m_RedrawStatus;
  m_RedrawStatus = Redraw::NoRedraw;
  return Return;
}

COLORREF CMagneticApp::GetForeColour(DarkMode* dark)
{
  if (m_ForeColour == ~0)
    return GetDefaultForeColour(dark);
  return m_ForeColour;
}

COLORREF CMagneticApp::GetBackColour(DarkMode* dark)
{
  if (m_BackColour == ~0)
    return GetDefaultBackColour(dark);
  return m_BackColour;
}

COLORREF CMagneticApp::GetGfxColour(DarkMode* dark)
{
  if (m_GfxColour == ~0)
    return GetDefaultGfxColour(dark);
  return m_GfxColour;
}

COLORREF CMagneticApp::GetDefaultForeColour(DarkMode* dark)
{
  if (dark)
    return dark->GetColour(DarkMode::Fore);
  return GetSysColor(COLOR_WINDOWTEXT);
}

COLORREF CMagneticApp::GetDefaultBackColour(DarkMode* dark)
{
  if (dark)
    return dark->GetColour(DarkMode::Back);
  return GetSysColor(COLOR_WINDOW);
}

COLORREF CMagneticApp::GetDefaultGfxColour(DarkMode* dark)
{
  if (dark)
    return dark->GetColour(DarkMode::Darkest);
  return GetSysColor(COLOR_APPWORKSPACE);
}

CRect& CMagneticApp::GetWindowRect(void)
{
  return m_WindowRect;
}

int& CMagneticApp::GetWindowMax(void)
{
  return m_iWindowMax;
}

CPoint& CMagneticApp::GetPicTopLeft(void)
{
  return m_PicTopLeft;
}

void CMagneticApp::GetControlBars(BOOL& bToolBar, BOOL& bStatusBar)
{
  bToolBar = m_bToolBar;
  bStatusBar = m_bStatusBar;
}

void CMagneticApp::SetControlBars(BOOL bToolBar, BOOL bStatusBar)
{
  m_bToolBar = bToolBar;
  m_bStatusBar = bStatusBar;
}

CRect& CMagneticApp::GetHintsRect(void)
{
  return m_HintsRect;
}

CMagneticApp::ShowGraphics CMagneticApp::GetShowGraphics(void)
{
  return m_ShowGfx;
}

void CMagneticApp::SetShowGraphics(ShowGraphics Show)
{
  m_ShowGfx = Show;
}

double CMagneticApp::GetScaleFactor(void)
{
  return m_dScaleFactor;
}

BOOL CMagneticApp::GetUseHintWindow(void)
{
  return m_bHintWindow;
}

BOOL CMagneticApp::GetAnimWait(void)
{
  return m_bAnimWait;
}

BOOL CMagneticApp::GetPredictable(void)
{
  return m_bPredict;
}

int CMagneticApp::GetRandomSeed(void)
{
  return m_iSeed;
}

double CMagneticApp::GetGamma(void)
{
  return m_dGamma;
}

int CMagneticApp::GetGameLoaded(void)
{
  return m_iGameLoaded;
}

void CMagneticApp::SetGameLoaded(int iLoaded)
{
  m_iGameLoaded = iLoaded;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog class
/////////////////////////////////////////////////////////////////////////////

class CLogoStatic : public CStatic
{
public:
  virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:
  afx_msg void OnNcPaint();
  DECLARE_MESSAGE_MAP()

private:
  ImagePNG m_baseLogo;
  ImagePNG m_scaledLogo;
};

BEGIN_MESSAGE_MAP(CLogoStatic, CStatic)
  ON_WM_NCPAINT()
END_MESSAGE_MAP()

void CLogoStatic::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
  CDC* dc = CDC::FromHandle(lpDrawItemStruct->hDC);
  CRect r(lpDrawItemStruct->rcItem);

  if (!m_baseLogo.Pixels())
  {
    m_baseLogo.SetBackground(RGB(0xFF,0xFF,0xFF));
    m_baseLogo.LoadResource(IDR_LOGO);
  }
  if (m_baseLogo.Pixels())
  {
    if (!m_scaledLogo.Pixels() || (m_scaledLogo.Size() != r.Size()))
      m_scaledLogo.Scale(m_baseLogo,r.Size());
  }
  if (m_scaledLogo.Pixels())
    m_scaledLogo.Draw(dc,r.TopLeft());
}

void CLogoStatic::OnNcPaint()
{
  DarkMode* dark = DarkMode::GetActive(this);
  if (dark)
  {
    CWindowDC dc(this);
    CRect r = dark->PrepareNonClientBorder(this,dc);
    dc.FillSolidRect(r,dark->GetColour(DarkMode::Dark2));
    dc.SelectClipRgn(NULL);
  }
  else
    Default();
}

class CAboutDlg : public CMagneticDlg
{
public:
  CAboutDlg();

// Dialog Data
  //{{AFX_DATA(CAboutDlg)
  enum { IDD = IDD_ABOUTBOX };
  //}}AFX_DATA

  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CAboutDlg)
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:
  // Generated message map functions
  //{{AFX_MSG(CAboutDlg)
  virtual BOOL OnInitDialog();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

  CLogoStatic m_logo;
  DarkModeGroupBox m_credits;
  DarkModeButton m_ok;
};

CAboutDlg::CAboutDlg() : CMagneticDlg(CAboutDlg::IDD)
{
  //{{AFX_DATA_INIT(CAboutDlg)
  //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
  CMagneticDlg::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CAboutDlg)
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CMagneticDlg)
  //{{AFX_MSG_MAP(CAboutDlg)
    // No message handlers
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog() 
{
  CMagneticDlg::OnInitDialog();

  // Subclass the static logo bitmap control
  if (m_logo.SubclassDlgItem(IDC_LOGO,this) == FALSE)
    return FALSE;
  CRect logoRect;
  m_logo.GetWindowRect(logoRect);
  ScreenToClient(logoRect);
  double aspect = ((double)logoRect.Width())/logoRect.Height();

  // Subclass the controls for dark mode
  m_credits.SubclassDlgItem(IDC_CREDITS,this);
  m_ok.SubclassDlgItem(IDOK,this);

  // Get the credits group control
  CRect creditsRect;
  m_credits.GetWindowRect(creditsRect);
  ScreenToClient(creditsRect);

  // Resize the logo
  logoRect.right = creditsRect.left-logoRect.left;
  logoRect.bottom = logoRect.top+(int)(logoRect.Width()/aspect);
  m_logo.MoveWindow(logoRect);
  return TRUE;
}

// App command to run the dialog
void CMagneticApp::OnAppAbout()
{
  CAboutDlg AboutDlg;
  AboutDlg.DoModal();
}
