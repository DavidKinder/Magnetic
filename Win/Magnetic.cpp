/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 1.0
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

extern "C"
{
#include "defs.h"
extern type8 running;
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

BOOL CMagneticApp::InitInstance()
{
#ifdef _AFXDLL
	Enable3dControls();
#else
	Enable3dControlsStatic();
#endif

	AfxEnableControlContainer();
	AfxInitRichEdit();

	SetRegistryKey(_T("David Kinder"));
	LoadStdProfileSettings();

	// Load Magnetic display settings
	m_LogFont.lfHeight = GetProfileInt("Display","Font Size",-16);
	m_LogFont.lfCharSet = ANSI_CHARSET;
	m_LogFont.lfOutPrecision = OUT_TT_PRECIS;
	m_LogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	m_LogFont.lfQuality = PROOF_QUALITY;
	m_LogFont.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
	strncpy(m_LogFont.lfFaceName,
		GetProfileString("Display","Font Name","Times New Roman"),LF_FACESIZE);
	m_Margins.cx = 8;
	m_Margins.cy = 4;

	SetRedrawStatus(NoActionNeeded);

	m_WindowRect.left = GetProfileInt("Window","Left",0);
	m_WindowRect.top = GetProfileInt("Window","Top",0);
	m_WindowRect.right = GetProfileInt("Window","Right",0);
	m_WindowRect.bottom = GetProfileInt("Window","Bottom",0);
	m_iWindowMax = GetProfileInt("Window","Maximized",0);
	if (m_iWindowMax)
		m_nCmdShow = SW_SHOWMAXIMIZED;

	m_PicTopLeft.x = GetProfileInt("Picture","Left",0);
	m_PicTopLeft.y = GetProfileInt("Picture","Top",0);

	m_bShowPics = GetProfileInt("Picture","Show",1);
	m_dScaleFactor = (double)GetProfileInt("Picture","Scale",100)*0.01;
	m_dScaleTitles = (double)GetProfileInt("Titles","Scale",100)*0.01;
	m_ForeColour = GetProfileInt("Display","Foreground",~0);
	m_BackColour = GetProfileInt("Display","Background",~0);

	m_iGameLoaded = 0;

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CMagneticDoc),
		RUNTIME_CLASS(CMainFrame),
		RUNTIME_CLASS(CMagneticView));
	AddDocTemplate(pDocTemplate);

	// Create file dialog for loading games
	m_pNewGameDialog = new CFileDialog(TRUE,NULL,
		GetProfileString("Settings","Last File",""),
		OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,
		"Magnetic Files (*.mag)|*.mag|All Files (*.*)|*.*||");
	if (m_pNewGameDialog == NULL)
		return FALSE;
	m_pNewGameDialog->m_ofn.lpstrTitle = "Open a Magnetic Scrolls game";

	// Create font dialog
	m_pFontDialog = new CFontDialog(&m_LogFont,CF_SCREENFONTS);
	if (m_pFontDialog == NULL)
		return FALSE;

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	CMenu* pMenu = m_pMainWnd->GetMenu();
  if (pMenu)
		pMenu->DestroyMenu();
  HMENU hMenu = ((CMainFrame*)m_pMainWnd)->NewMenu();
  pMenu = CMenu::FromHandle(hMenu);
  m_pMainWnd->SetMenu(pMenu);
  ((CMainFrame*)m_pMainWnd)->m_hMenuDefault = hMenu;

	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}

int CMagneticApp::ExitInstance() 
{
	// Write out settings
	WriteProfileString("Settings","Last File",m_pNewGameDialog->GetPathName());

	WriteProfileString("Display","Font Name",CString(m_LogFont.lfFaceName));
	WriteProfileInt("Display","Font Size",m_LogFont.lfHeight);
	WriteProfileInt("Display","Foreground",m_ForeColour);
	WriteProfileInt("Display","Background",m_BackColour);

	WriteProfileInt("Window","Left",m_WindowRect.left);
	WriteProfileInt("Window","Top",m_WindowRect.top);
	WriteProfileInt("Window","Right",m_WindowRect.right);
	WriteProfileInt("Window","Bottom",m_WindowRect.bottom);
	WriteProfileInt("Window","Maximized",m_iWindowMax);

	WriteProfileInt("Picture","Left",m_PicTopLeft.x);
	WriteProfileInt("Picture","Top",m_PicTopLeft.y);
	WriteProfileInt("Picture","Show",m_bShowPics);
	WriteProfileInt("Picture","Scale",(int)(m_dScaleFactor*100));
	WriteProfileInt("Titles","Scale",(int)(m_dScaleTitles*100));

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
		CMagneticView* pView = CMagneticView::GetView();

		if (m_iGameLoaded)
		{
			OnFileNew();
			pView->ClearAll();
		}
	}

	return bIdle;
}

void CMagneticApp::OnFileOpen() 
{
	if (running)
		SetRedrawStatus(RedrawThisLine);

	if (m_pNewGameDialog->DoModal() == IDOK)
	{
		if (running)
			SetRedrawStatus(RedrawEndLine);
		OpenDocumentFile(m_pNewGameDialog->GetPathName());
	}
}

void CMagneticApp::OnViewFont() 
{
	SetRedrawStatus(RedrawThisLine);
	// Change the display font
	if (m_pFontDialog->DoModal() == IDOK)
	{
		CMagneticView* pView = CMagneticView::GetView();
		pView->TextClearup();
		pView->TextSetup();
		pView->Invalidate();
	}
}

void CMagneticApp::OnViewOptions() 
{
	SetRedrawStatus(RedrawThisLine);

	COptionsDlg Options;

	// Set up initial values in the dialog
	Options.m_bShowPics = m_bShowPics;
	Options.m_dScaleFactor = m_dScaleFactor;
	Options.m_dScaleTitles = m_dScaleTitles;

	if (Options.DoModal() == IDOK)
	{
		m_bShowPics = Options.m_bShowPics;
		m_dScaleFactor = Options.m_dScaleFactor;
		m_dScaleTitles = Options.m_dScaleTitles;

		m_ForeColour = Options.m_FColour.currentcolor;
		if (m_ForeColour == GetSysColor(COLOR_WINDOWTEXT))
			m_ForeColour = ~0;
		m_BackColour = Options.m_BColour.currentcolor;
		if (m_BackColour == GetSysColor(COLOR_WINDOW))
			m_BackColour = ~0;

		// Reset the picture window
		ms_showpic(0,0);
		ms_showpic(0,3);
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
	if (pView == NULL)
		return;

	if (pView->m_iMorePrompt == TRUE)
	{
		for (int iMRU = 0; iMRU < m_pRecentFileList->m_nSize; iMRU++)
			pCmdUI->m_pMenu->EnableMenuItem(nID + iMRU, MF_DISABLED|MF_GRAYED);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Get and set program settings
/////////////////////////////////////////////////////////////////////////////

LOGFONT* CMagneticApp::GetLogFont(void)
{
	return &m_LogFont;
}

void CMagneticApp::SetRedrawStatus(int iNewStatus)
{
	m_iRedrawStatus = iNewStatus;
}

int CMagneticApp::GetRedrawStatus(void)
{
	int iReturn = m_iRedrawStatus;
	m_iRedrawStatus = FALSE;
	return iReturn;
}

CSize& CMagneticApp::GetMargins()
{
	return m_Margins;
}

COLORREF CMagneticApp::GetForeColour(void)
{
	COLORREF Colour;

	if (m_ForeColour == ~0)
		Colour = GetSysColor(COLOR_WINDOWTEXT);
	else
		Colour = m_ForeColour;

	return Colour;
}

COLORREF CMagneticApp::GetBackColour(void)
{
	COLORREF Colour;

	if (m_BackColour == ~0)
		Colour = GetSysColor(COLOR_WINDOW);
	else
		Colour = m_BackColour;

	return Colour;
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

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog class
/////////////////////////////////////////////////////////////////////////////

class CAboutDlg : public CDialog
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
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CMagneticApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}
