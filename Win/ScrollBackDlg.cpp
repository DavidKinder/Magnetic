/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 2
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// ScrollBackDlg.cpp: Scrollback dialog class
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "Magnetic.h"
#include "ScrollBackDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Implementation of the CScrollBackDlg dialog
/////////////////////////////////////////////////////////////////////////////

#define WM_SAMESIZEASMAIN (WM_APP+1)

CScrollBackDlg::CScrollBackDlg(CWnd* pParent /*=NULL*/)
  : CMagneticDlg(CScrollBackDlg::IDD, pParent)
{
  //{{AFX_DATA_INIT(CScrollBackDlg)
    // NOTE: the ClassWizard will add member initialization here
  //}}AFX_DATA_INIT
}

void CScrollBackDlg::DoDataExchange(CDataExchange* pDX)
{
  CMagneticDlg::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CScrollBackDlg)
    // NOTE: the ClassWizard will add DDX and DDV calls here
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CScrollBackDlg, CMagneticDlg)
  //{{AFX_MSG_MAP(CScrollBackDlg)
  ON_WM_SIZE()
  ON_BN_CLICKED(IDC_COPY, OnCopy)
  //}}AFX_MSG_MAP
  ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
  ON_MESSAGE(WM_SAMESIZEASMAIN, OnSameSizeAsMain)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScrollBackDlg message handlers
/////////////////////////////////////////////////////////////////////////////

BOOL CScrollBackDlg::OnInitDialog() 
{
  CMagneticDlg::OnInitDialog();
  CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
  
  // Subclass the text control
  if (m_RichEdit.SubclassDlgItem(IDC_TEXT,this) == FALSE)
    return FALSE;

  // Subclass the buttons
  m_CopyButton.SubclassDlgItem(IDC_COPY,this);
  m_CloseButton.SubclassDlgItem(IDOK,this);

  // Change the window icon
  SetIcon(pApp->LoadIcon(IDR_MAINFRAME),TRUE);

  // Resize the dialog
  CRect DialogRect;
  AfxGetMainWnd()->GetWindowRect(DialogRect);
  MoveWindow(DialogRect);

  // Set the control to format the text so that it fits
  // into the window
  m_RichEdit.SetTargetDevice(NULL,0);
  
  // Set the background colour
  m_RichEdit.SetBackgroundColor(FALSE,GetSysColor(COLOR_3DFACE));

  // Put the text into the control
  m_RichEdit.SetWindowText(m_strScrollback);

  // Put the cursor at the end of the buffer
  m_RichEdit.SetSel(-1,-1);
  m_RichEdit.SendMessage(EM_SCROLLCARET);

  return TRUE;
}

void CScrollBackDlg::OnSize(UINT nType, int cx, int cy) 
{
  CMagneticDlg::OnSize(nType, cx, cy);
  ResizeRichEdit();
}

void CScrollBackDlg::OnCopy()
{
  m_RichEdit.Copy();
}

LRESULT CScrollBackDlg::OnDpiChanged(WPARAM, LPARAM)
{
  Default();

  // Same monitor?
  if (DPI::getMonitorRect(this) == DPI::getMonitorRect(AfxGetMainWnd()))
    PostMessage(WM_SAMESIZEASMAIN);
  return 0;
}

LRESULT CScrollBackDlg::OnSameSizeAsMain(WPARAM, LPARAM)
{
  // Resize the dialog to be the same as the main window
  CRect DialogRect;
  AfxGetMainWnd()->GetWindowRect(DialogRect);
  MoveWindow(DialogRect);
  return 0;
}

void CScrollBackDlg::ResizeRichEdit(void)
{
  if (m_RichEdit.GetSafeHwnd())
  {
    CRect editRect;
    m_RichEdit.GetWindowRect(editRect);
    ScreenToClient(editRect);

    CRect clientRect;
    GetClientRect(clientRect);

    m_RichEdit.MoveWindow(0,editRect.top,
      clientRect.Width(),clientRect.Height()-editRect.top);
  }
}

void CScrollBackDlg::SetDarkMode(DarkMode* dark)
{
  CMagneticDlg::SetDarkMode(dark);

  if (GetSafeHwnd() != 0)
    m_RichEdit.SetDarkMode(dark,DarkMode::Back);
}

CString& CScrollBackDlg::GetScrollback(void)
{
  return m_strScrollback;
}
