
// AutoShotDlg.h: 头文件
//

#pragma once
#include "afxdialogex.h"
#include "CMyPicStatic.h"
#include "FScreenShot.h"
#include "FHighResTimer.h"


// CAutoShotDlg 对话框
class CAutoShotDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAutoShotDlg)
// 构造
public:
	CAutoShotDlg(CWnd* pParent = nullptr);	// 标准构造函数
	virtual ~CAutoShotDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AUTOSHOT_DIALOG };
#endif

	void TimerCallback();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

private:
	void SetEnableAllComponents(bool param1);

public:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	std::atomic<bool> m_internalTerminateFlag = false;

	CMyPicStatic m_picStatic;
	CString m_strHandle = _T("0");
	CString m_strCaption;
	CString m_strStyle;
	CString m_strClass;
	CString m_strRect;
	CString m_outImgDir;
	CString m_outImgDirFinal;

	int m_editInterval;
	int m_editNums;
	int m_checkCreateSubFold;
	int m_checkOpenAfterCompleted;
	int m_radioFormatBMP;
	int m_radioFormatPNG;
	int m_radioFormatJPG;

	afx_msg void OnChangeWndHandle();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnDeltaposSpinInterval(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinNums(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg void OnClose();
	CButton m_btnRun;
	CButton m_btnPause;
	CButton m_btnStop;
	CProgressCtrl m_progressRun;

	FWindowScreenshot m_shot;
	afx_msg void OnBnClickedButtonRun();
	
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnStnClickedStaticPicFinder();
};
