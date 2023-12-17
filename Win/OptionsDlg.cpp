/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 2
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// OptionsDlg.cpp: Options dialog class
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "Magnetic.h"
#include "OptionsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Implementation of the COptionsDlg dialog
/////////////////////////////////////////////////////////////////////////////

COptionsDlg::COptionsDlg(CWnd* pParent /*=NULL*/) : CMagneticDlg(COptionsDlg::IDD, pParent)
{
  //{{AFX_DATA_INIT(COptionsDlg)
  m_dScaleFactor = 0.0;
  m_bPredict = FALSE;
  m_iSeed = 0;
  m_iShowPics = -1;
  m_dGamma = 0.0;
  m_bAnimWait = FALSE;
  m_bHintWindow = FALSE;
  //}}AFX_DATA_INIT

  m_DefaultFColour = 0;
  m_DefaultBColour = 0;
  m_DefaultGColour = 0;
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
  CMagneticDlg::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(COptionsDlg)
  DDX_Text(pDX, IDC_SCALE, m_dScaleFactor);
  DDV_MinMaxDouble(pDX, m_dScaleFactor, 0.5, 5.);
  DDX_Check(pDX, IDC_PREDICT, m_bPredict);
  DDX_Text(pDX, IDC_SEED, m_iSeed);
  DDX_CBIndex(pDX, IDC_SHOWPIC, m_iShowPics);
  DDX_Text(pDX, IDC_GAMMA, m_dGamma);
  DDV_MinMaxDouble(pDX, m_dGamma, 0.5, 5.);
  DDX_Check(pDX, IDC_ANIM_WAIT, m_bAnimWait);
  DDX_Check(pDX, IDC_HINT_WINDOW, m_bHintWindow);
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COptionsDlg, CMagneticDlg)
  //{{AFX_MSG_MAP(COptionsDlg)
  ON_WM_HELPINFO()
  ON_BN_CLICKED(IDC_PREDICT, OnChangePredict)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg message handlers
/////////////////////////////////////////////////////////////////////////////

BOOL COptionsDlg::OnInitDialog() 
{
  CMagneticDlg::OnInitDialog();
  CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
  
  // Subclass the spin controls
  if (m_SpinScale.SubclassDlgItem(IDC_SPINSCALE,this) == FALSE)
    return FALSE;
  m_SpinScale.SetRange(1,5);
  if (m_SpinGamma.SubclassDlgItem(IDC_SPINGAMMA,this) == FALSE)
    return FALSE;
  m_SpinGamma.SetRange(1,5);
  
  // Subclass the colour controls
  if (m_FColour.SubclassDlgItem(IDC_FORE,this) == FALSE)
    return FALSE;
  if (m_BColour.SubclassDlgItem(IDC_BACK,this) == FALSE)
    return FALSE;
  if (m_GColour.SubclassDlgItem(IDC_GFX,this) == FALSE)
    return FALSE;

  // Subclass the controls for dark mode
  if (m_OK.SubclassDlgItem(IDOK,this) == FALSE)
    return FALSE;
  if (m_Cancel.SubclassDlgItem(IDCANCEL,this) == FALSE)
    return FALSE;
  if (m_GrahicsGroup.SubclassDlgItem(IDC_GRAPHICS_GROUP,this) == FALSE)
    return FALSE;
  if (m_ColoursGroup.SubclassDlgItem(IDC_COLOURS_GROUP,this) == FALSE)
    return FALSE;
  if (m_OtherGroup.SubclassDlgItem(IDC_OTHER_GROUP,this) == FALSE)
    return FALSE;
  if (m_ShowPicsCombo.SubclassDlgItem(IDC_SHOWPIC,this) == FALSE)
    return FALSE;
  if (m_Scale.SubclassDlgItem(IDC_SCALE,this) == FALSE)
    return FALSE;
  m_Scale.SetSpinButton(&m_SpinScale);
  if (m_Gamma.SubclassDlgItem(IDC_GAMMA,this) == FALSE)
    return FALSE;
  m_Gamma.SetSpinButton(&m_SpinGamma);
  if (m_HintCheck.SubclassDlgItem(IDC_HINT_WINDOW,this,IDR_DARK_CHECK) == FALSE)
    return FALSE;
  if (m_AnimWaitCheck.SubclassDlgItem(IDC_ANIM_WAIT,this,IDR_DARK_CHECK) == FALSE)
    return FALSE;
  if (m_PredictCheck.SubclassDlgItem(IDC_PREDICT,this,IDR_DARK_CHECK) == FALSE)
    return FALSE;
  if (m_Seed.SubclassDlgItem(IDC_SEED,this) == FALSE)
    return FALSE;

  // Set the colours
  DarkMode* dark = DarkMode::GetActive(this);
  m_DefaultFColour = pApp->GetDefaultForeColour(dark);
  m_DefaultBColour = pApp->GetDefaultBackColour(dark);
  m_DefaultGColour = pApp->GetDefaultGfxColour(dark);
  m_FColour.SetCurrentColour(pApp->GetForeColour(dark));
  m_BColour.SetCurrentColour(pApp->GetBackColour(dark));
  m_GColour.SetCurrentColour(pApp->GetGfxColour(dark));

  m_Seed.EnableWindow(m_bPredict == TRUE);
  return TRUE;
}

HWND WINAPI AfxHtmlHelp(HWND hWnd, LPCTSTR szHelpFilePath, UINT nCmd, DWORD_PTR dwData);

BOOL COptionsDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
  static int m_helpIds[] =
  {
    IDC_SHOWPIC,1,
    IDC_PIC_LABEL,1,
    IDC_SCALE,2,
    IDC_SCALE_LABEL,2,
    IDC_SPINSCALE,2,
    IDC_FORE,3,
    IDC_TEXT_LABEL,3,
    IDC_BACK,4,
    IDC_BACK_LABEL,4,
    IDC_GAMMA,5,
    IDC_GAMMA_LABEL,5,
    IDC_SPINGAMMA,5,
    IDC_GFX,6,
    IDC_GFX_LABEL,6,
    IDC_PREDICT,7,
    IDC_SEED,7,
    IDC_SEED_LABEL,7,
    IDC_ANIM_WAIT,8,
    IDC_HINT_WINDOW,9,
    0,0
  };

  if (pHelpInfo->iContextType == HELPINFO_WINDOW)
  {
    // Is there a help topic for this control?
    int* id = m_helpIds;
    while (*id != 0)
    {
      if (pHelpInfo->iCtrlId == *id)
      {
        CString helpFile(AfxGetApp()->m_pszHelpFilePath);
        helpFile.Append("::/options.txt");

        // Show the help popup
        AfxHtmlHelp((HWND)pHelpInfo->hItemHandle,helpFile,
          HH_TP_HELP_WM_HELP,(DWORD_PTR)m_helpIds);
        return TRUE;
      }
      id += 2;
    }
  }
  return TRUE;
}

void COptionsDlg::OnChangePredict() 
{
  m_Seed.EnableWindow(m_PredictCheck.GetCheck() == 1);
}

void COptionsDlg::SetDarkMode(DarkMode* dark)
{
  CMagneticDlg::SetDarkMode(dark);

  // Adjust the colours, if set to default values
  CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
  COLORREF DefaultFColour = pApp->GetDefaultForeColour(dark);
  COLORREF DefaultBColour = pApp->GetDefaultBackColour(dark);
  COLORREF DefaultGColour = pApp->GetDefaultGfxColour(dark);
  if (m_FColour.GetCurrentColour() == m_DefaultFColour)
    m_FColour.SetCurrentColour(DefaultFColour);
  if (m_BColour.GetCurrentColour() == m_DefaultBColour)
    m_BColour.SetCurrentColour(DefaultBColour);
  if (m_GColour.GetCurrentColour() == m_DefaultGColour)
    m_GColour.SetCurrentColour(DefaultGColour);
  m_DefaultFColour = DefaultFColour;
  m_DefaultBColour = DefaultBColour;
  m_DefaultGColour = DefaultGColour;
}

COLORREF COptionsDlg::GetForeColour(void)
{
  return m_FColour.GetCurrentColour();
}

COLORREF COptionsDlg::GetBackColour(void)
{
  return m_BColour.GetCurrentColour();
}

COLORREF COptionsDlg::GetGfxColour(void)
{
  return m_GColour.GetCurrentColour();
}
