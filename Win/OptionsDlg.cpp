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

COptionsDlg::COptionsDlg(CWnd* pParent /*=NULL*/) : CDialog(COptionsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptionsDlg)
	m_dScaleFactor = 0.0;
	m_dScaleTitles = 0.0;
	m_bPredict = FALSE;
	m_iSeed = 0;
	m_iShowPics = -1;
	m_dGamma = 0.0;
	m_bAnimWait = FALSE;
	//}}AFX_DATA_INIT
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsDlg)
	DDX_Control(pDX, IDC_PREDICT, m_Predict);
	DDX_Control(pDX, IDC_SEED, m_Seed);
	DDX_Control(pDX, IDC_SEED_LABEL, m_SeedLabel);
	DDX_Text(pDX, IDC_SCALE, m_dScaleFactor);
	DDV_MinMaxDouble(pDX, m_dScaleFactor, 0.5, 5.);
	DDX_Text(pDX, IDC_SCALET, m_dScaleTitles);
	DDV_MinMaxDouble(pDX, m_dScaleTitles, 0.5, 5.);
	DDX_Check(pDX, IDC_PREDICT, m_bPredict);
	DDX_Text(pDX, IDC_SEED, m_iSeed);
	DDX_CBIndex(pDX, IDC_SHOWPIC, m_iShowPics);
	DDX_Text(pDX, IDC_GAMMA, m_dGamma);
	DDV_MinMaxDouble(pDX, m_dGamma, 0.5, 5.);
	DDX_Check(pDX, IDC_ANIM_WAIT, m_bAnimWait);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COptionsDlg, CDialog)
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
	CDialog::OnInitDialog();
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	
	// Subclass the spin controls
	if (m_Spin.SubclassDlgItem(IDC_SPIN,this) == FALSE)
		return FALSE;
	m_Spin.SetRange(1,5);
	if (m_SpinTitles.SubclassDlgItem(IDC_SPINT,this) == FALSE)
		return FALSE;
	m_SpinTitles.SetRange(1,5);
	if (m_SpinGamma.SubclassDlgItem(IDC_SPING,this) == FALSE)
		return FALSE;
	m_SpinGamma.SetRange(1,5);
	
	// Subclass the colour controls
	if (m_FColour.SubclassDlgItem(IDC_FORE,this) == FALSE)
		return FALSE;
	if (m_BColour.SubclassDlgItem(IDC_BACK,this) == FALSE)
		return FALSE;
	if (m_GColour.SubclassDlgItem(IDC_GFX,this) == FALSE)
		return FALSE;

	// Set the colours
	m_FColour.currentcolor = pApp->GetForeColour();
	m_BColour.currentcolor = pApp->GetBackColour();
	m_GColour.currentcolor = pApp->GetGfxColour();

	m_Seed.EnableWindow(m_bPredict == TRUE);

	// Set the help contexts
	GetDlgItem(IDC_SHOWPIC)->SetWindowContextHelpId(0x10000);
	GetDlgItem(IDC_PIC_LABEL)->SetWindowContextHelpId(0x10000);
	GetDlgItem(IDC_SCALE)->SetWindowContextHelpId(0x10001);
	GetDlgItem(IDC_SCALE_LABEL)->SetWindowContextHelpId(0x10001);
	m_Spin.SetWindowContextHelpId(0x10001);
	m_FColour.SetWindowContextHelpId(0x10002);
	GetDlgItem(IDC_TEXT_LABEL)->SetWindowContextHelpId(0x10002);
	m_BColour.SetWindowContextHelpId(0x10003);
	GetDlgItem(IDC_BACK_LABEL)->SetWindowContextHelpId(0x10003);
	m_GColour.SetWindowContextHelpId(0x10006);
	GetDlgItem(IDC_GFX_LABEL)->SetWindowContextHelpId(0x10006);
	GetDlgItem(IDC_SCALET)->SetWindowContextHelpId(0x10004);
	GetDlgItem(IDC_SCALET_LABEL)->SetWindowContextHelpId(0x10004);
	m_SpinTitles.SetWindowContextHelpId(0x10004);
	GetDlgItem(IDC_GAMMA)->SetWindowContextHelpId(0x10005);
	GetDlgItem(IDC_GAMMA_LABEL)->SetWindowContextHelpId(0x10005);
	m_SpinGamma.SetWindowContextHelpId(0x10005);
	GetDlgItem(IDC_PREDICT)->SetWindowContextHelpId(0x10007);
	m_Seed.SetWindowContextHelpId(0x10007);
	GetDlgItem(IDC_SEED_LABEL)->SetWindowContextHelpId(0x10007);
	GetDlgItem(IDC_ANIM_WAIT)->SetWindowContextHelpId(0x10008);

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

void COptionsDlg::OnChangePredict() 
{
	m_Seed.EnableWindow(m_Predict.GetCheck() == 1);
}

COLORREF COptionsDlg::GetForeColour(void)
{
	return m_FColour.currentcolor;
}

COLORREF COptionsDlg::GetBackColour(void)
{
	return m_BColour.currentcolor;
}

COLORREF COptionsDlg::GetGfxColour(void)
{
	return m_GColour.currentcolor;
}
