#if !defined(AFX_HINTDIALOG_H__7075FB36_BD0D_431B_B3D0_FCA5C587C85E__INCLUDED_)
#define AFX_HINTDIALOG_H__7075FB36_BD0D_431B_B3D0_FCA5C587C85E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// HintDialog.h : Header-Datei
//
#include "magnetic.h"
/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CHintDialog 

class CHintDialog : public CDialog
{
// Konstruktion
public:
	CHintDialog(CWnd* pParent = NULL);   // Standardkonstruktor

// Dialogfelddaten
	//{{AFX_DATA(CHintDialog)
	enum { IDD = IDD_HINTS };
	CButton	m_topicbutton;
	CButton	m_hintbutton;
	CButton	m_prevbutton;
	CListBox m_hlistbox;
	//}}AFX_DATA


// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(CHintDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung
protected:

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(CHintDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnPrevious();
	afx_msg void OnTopics();
	afx_msg void OnShowhint();
	afx_msg void OnSelchangeHintlist();
	afx_msg void OnDblclkHintlist();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	struct ms_hint* allHints;
	int currHint;
	int visibleHints;

	int LoadHintSet( int elnumber );
	void UpdateHintList();
public:
    void SetHints(struct ms_hint* allHints);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_HINTDIALOG_H__7075FB36_BD0D_431B_B3D0_FCA5C587C85E__INCLUDED_
