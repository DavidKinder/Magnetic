// HintDialog.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "magnetic.h"
#include "HintDialog.h"

extern "C"
{
#include "defs.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CHintDialog 


CHintDialog::CHintDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CHintDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHintDialog)
		// HINWEIS: Der Klassen-Assistent fügt hier Elementinitialisierung ein
	//}}AFX_DATA_INIT
}


void CHintDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHintDialog)
	DDX_Control(pDX, IDC_TOPICS, m_topicbutton);
	DDX_Control(pDX, IDC_SHOWHINT, m_hintbutton);
	DDX_Control(pDX, IDC_PREVIOUS, m_prevbutton);
	DDX_Control(pDX, IDC_HINTLIST, m_hlistbox);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHintDialog, CDialog)
	//{{AFX_MSG_MAP(CHintDialog)
	ON_BN_CLICKED(IDC_PREVIOUS, OnPrevious)
	ON_BN_CLICKED(IDC_TOPICS, OnTopics)
	ON_BN_CLICKED(IDC_SHOWHINT, OnShowhint)
	ON_LBN_SELCHANGE(IDC_HINTLIST, OnSelchangeHintlist)
	ON_LBN_DBLCLK(IDC_HINTLIST, OnDblclkHintlist)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten CHintDialog 
BOOL CHintDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	
    if (allHints != NULL) {
		currHint = 0;
		visibleHints = 0;
		LoadHintSet(0);
	} else {
		currHint = -1;
	}
	//m_hlistbox.AddString("Test\0");
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Eigenschaftenseiten sollten FALSE zurückgeben
}

void CHintDialog::SetHints(struct ms_hint* hints) 
{
   allHints = hints;
}

int CHintDialog::LoadHintSet( int elnumber )
{
    m_hlistbox.ResetContent();
	if (allHints != NULL) {
		for (int i=0; i < allHints[elnumber].elcount; i++) {
			if (allHints[elnumber].nodetype == 1) {
	           m_hlistbox.AddString((char *)(&(allHints[elnumber].content[i])));
			   visibleHints = 0;
			} else {
				CString lbitem;
				if (i <= visibleHints) {
				   lbitem.Format("%d %s",i+1,(char *)(&(allHints[elnumber].content[i])));
				} else {
				   lbitem.Format("%d ",i+1);
				}
               m_hlistbox.AddString(lbitem);
			}
		}

		currHint = elnumber;

		// set controls
		if (allHints[elnumber].parent == 0xFFFF) {
			m_topicbutton.EnableWindow(false);
			m_prevbutton.EnableWindow(false);
		} else {
			m_topicbutton.EnableWindow(true);
			m_prevbutton.EnableWindow(true);
		}
        
		if ((allHints[currHint].nodetype == 1) ||
			((allHints[currHint].nodetype != 1) && (allHints[currHint].elcount == visibleHints+1)) )
		   m_hintbutton.EnableWindow(false);

		return elnumber;
	} else {
		return -1;
	}
}

void CHintDialog::OnPrevious() 
{
	// TODO: Code für die Behandlungsroutine der Steuerelement-Benachrichtigung hier einfügen
    if (currHint != -1)
	   LoadHintSet(allHints[currHint].parent);
	
}

void CHintDialog::OnTopics() 
{
	// TODO: Code für die Behandlungsroutine der Steuerelement-Benachrichtigung hier einfügen
    if (currHint != -1)
	   LoadHintSet(0);
	
}

void CHintDialog::OnShowhint() 
{
	// TODO: Code für die Behandlungsroutine der Steuerelement-Benachrichtigung hier einfügen
	UpdateHintList();
	
}

void CHintDialog::OnSelchangeHintlist() 
{
	// TODO: Code für die Behandlungsroutine der Steuerelement-Benachrichtigung hier einfügen
    if ((m_hlistbox.GetCurSel() == LB_ERR) ||
		((allHints[currHint].nodetype != 1) && (visibleHints+1  == allHints[currHint].elcount) ))
		m_hintbutton.EnableWindow(false);
	else {

		m_hintbutton.EnableWindow(true);
	}
}

void CHintDialog::OnDblclkHintlist() 
{
	// TODO: Code für die Behandlungsroutine der Steuerelement-Benachrichtigung hier einfügen
    if (!((m_hlistbox.GetCurSel() == LB_ERR) ||
		((allHints[currHint].nodetype != 1) && (visibleHints+1  == allHints[currHint].elcount) )))
	UpdateHintList();
}

void CHintDialog::UpdateHintList()
{
	// TODO: Code für die Behandlungsroutine der Steuerelement-Benachrichtigung hier einfügen
	int currSel = m_hlistbox.GetCurSel();
	int newIndex;
	if (allHints[currHint].nodetype == 1) {
	   newIndex = allHints[currHint].links[currSel];
	   visibleHints = 0;
    } else {
		newIndex = currHint;
		visibleHints++;
	}
	LoadHintSet(newIndex);
}
