/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 1.0
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

COptionsDlg::COptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(COptionsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptionsDlg)
	m_bShowPics = FALSE;
	m_dScaleFactor = 0.0;
	m_dScaleTitles = 0.0;
	//}}AFX_DATA_INIT
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsDlg)
	DDX_Check(pDX, IDC_SHOWPIC, m_bShowPics);
	DDX_Text(pDX, IDC_SCALE, m_dScaleFactor);
	DDV_MinMaxDouble(pDX, m_dScaleFactor, 0.5, 5.);
	DDX_Text(pDX, IDC_SCALET, m_dScaleTitles);
	DDV_MinMaxDouble(pDX, m_dScaleTitles, 0.5, 5.);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
	//{{AFX_MSG_MAP(COptionsDlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg message handlers
/////////////////////////////////////////////////////////////////////////////

BOOL COptionsDlg::OnInitDialog() 
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	CDialog::OnInitDialog();
	
	// Subclass the spin controls
	if (m_Spin.SubclassDlgItem(IDC_SPIN,this) == FALSE)
		return FALSE;
	m_Spin.SetRange(1,5);
	if (m_SpinTitles.SubclassDlgItem(IDC_SPINT,this) == FALSE)
		return FALSE;
	m_SpinTitles.SetRange(1,5);
	
	// Subclass the colour controls
	if (m_FColour.SubclassDlgItem(IDC_FORE,this) == FALSE)
		return FALSE;
	if (m_BColour.SubclassDlgItem(IDC_BACK,this) == FALSE)
		return FALSE;

	// Set the colours
	m_FColour.currentcolor = pApp->GetForeColour();
	m_BColour.currentcolor = pApp->GetBackColour();

	// Set the help contexts
	GetDlgItem(IDC_SHOWPIC)->SetWindowContextHelpId(0x10000);
	GetDlgItem(IDC_SCALE)->SetWindowContextHelpId(0x10001);
	GetDlgItem(IDC_SCALE_LABEL)->SetWindowContextHelpId(0x10001);
	m_Spin.SetWindowContextHelpId(0x10001);
	m_FColour.SetWindowContextHelpId(0x10002);
	GetDlgItem(IDC_TEXT_LABEL)->SetWindowContextHelpId(0x10002);
	m_BColour.SetWindowContextHelpId(0x10003);
	GetDlgItem(IDC_BACK_LABEL)->SetWindowContextHelpId(0x10003);
	GetDlgItem(IDC_SCALET)->SetWindowContextHelpId(0x10004);
	GetDlgItem(IDC_SCALET_LABEL)->SetWindowContextHelpId(0x10004);
	m_SpinTitles.SetWindowContextHelpId(0x10004);

	return TRUE;
}

BOOL COptionsDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	if (pHelpInfo->dwContextId != 0)
		AfxGetApp()->WinHelp(pHelpInfo->dwContextId,HELP_CONTEXTPOPUP);
	else
		AfxGetApp()->WinHelp(0,HELP_CONTENTS);
	return TRUE;
}
