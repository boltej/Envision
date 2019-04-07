#pragma once


// ProjectPathDlg dialog

class ProjectPathDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ProjectPathDlg)

public:
	ProjectPathDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~ProjectPathDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ENVXFILE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
   CString m_path;
   afx_msg void OnBnClickedBrowse();
   afx_msg void OnBnClickedOk();
   };
