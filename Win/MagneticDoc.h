/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 1.0
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// MagneticDoc.h: Empty document class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAGNETICDOC_H__459160EC_887D_11D1_ACE4_8081A5F82D24__INCLUDED_)
#define AFX_MAGNETICDOC_H__459160EC_887D_11D1_ACE4_8081A5F82D24__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CMagneticDoc : public CDocument
{
protected: // create from serialization only
	CMagneticDoc();
	DECLARE_DYNCREATE(CMagneticDoc)

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMagneticDoc)
	public:
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMagneticDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CMagneticDoc)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAGNETICDOC_H__459160EC_887D_11D1_ACE4_8081A5F82D24__INCLUDED_)
