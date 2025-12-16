#define _CRT_SECURE_NO_WARNINGS
// AutoShotDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "FHighResTimer.h"
#include "AutoShot.h"
#include "AutoShotDlg.h"
#include "afxdialogex.h"
#include "Resource.h"

#include <filesystem>
#include <memory>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "Dwmapi.lib")
#ifdef _DEBUG
#pragma comment(lib, "opencv_core480d.lib")  // 根据您的OpenCV版本调整
#else
#pragma comment(lib, "opencv_core480.lib") 
#endif


// CAutoShotDlg 对话框

IMPLEMENT_DYNAMIC(CAutoShotDlg, CDialogEx)

CAutoShotDlg::CAutoShotDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_AUTOSHOT_DIALOG, pParent)
	, m_editInterval(0)
	, m_checkCreateSubFold(0)
	, m_checkOpenAfterCompleted(0)
	, m_radioFormatBMP(0)
	, m_radioFormatPNG(0)
	, m_radioFormatJPG(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON_APP);
}

CAutoShotDlg::~CAutoShotDlg()
{
}

void CAutoShotDlg::TimerCallback()
{
	auto hwnd = (HWND)_tcstoul(m_strHandle, NULL, 16);

	auto cur = m_progressRun.GetPos();
	

	const TCHAR* suffix[] = {_T("bmp"), _T("png"), _T("jpg")};

	CString finalPath;
	finalPath.Format(_T("%s\\%d.%s"), m_outImgDirFinal, cur + 1, suffix[m_radioFormatBMP]);

	if (hwnd) {
		if (m_shot.CaptureWindowDXGI(hwnd, finalPath.GetString())) {
			std::cout << "DXGI screenshot saved successfully!" << std::endl;
		} else {
			// 如果DXGI失败，回退到GDI方法
			if (m_shot.CaptureWindowGDI(hwnd, finalPath.GetString())) {
				std::cout << "GDI screenshot saved successfully!" << std::endl;
			} else {
				std::cerr << "All screenshot methods failed!" << std::endl;
			}
		}
	} else {
		// 如果没有找到特定窗口，截取整个桌面
		auto desktop = GetDesktopWindow();
		if (m_shot.CaptureWindowGDI(desktop->m_hWnd, finalPath.GetString())) {
			std::cout << "Desktop screenshot saved successfully!" << std::endl;
		}
	}

	CString status;
	auto fPercent = (cur + 1) * 100.0 / m_editNums;
	status.Format(_T("%.2f%% %d/%d"), fPercent, cur + 1, m_editNums);
	SetDlgItemText(IDC_EDIT_PROGRESS_RATIO, status);

	m_progressRun.StepIt();

	if (m_progressRun.GetPos() >= m_editNums) {
		if (m_checkOpenAfterCompleted) {
			CString param = CString("/select,") + m_outImgDirFinal + _T("\\");
			ShellExecute(0, _T("open"), _T("Explorer.exe"), param, 0, SW_SHOWNORMAL);
		}

		//MessageBox(_T("已全部运行完成"), _T("成功"), MB_ICONINFORMATION | MB_OK);
		SetEnableAllComponents(true);
		m_btnPause.EnableWindow(false);
		m_btnPause.SetWindowTextW(_T("暂停 (&P)"));
		m_btnStop.EnableWindow(false);
		KillTimer(1);
	}
}

void CAutoShotDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_PIC_FINDER, m_picStatic);
	DDX_Text(pDX, IDC_EDIT_WND_HANDLE, m_strHandle);
	DDX_Text(pDX, IDC_EDIT_WND_Caption, m_strCaption);
	DDX_Text(pDX, IDC_EDIT_WND_STYLE, m_strStyle);
	DDX_Text(pDX, IDC_EDIT_WND_CLASS, m_strClass);
	DDX_Text(pDX, IDC_EDIT_WND_RECT, m_strRect);
	DDX_Text(pDX, IDC_EDIT_OUT_IMG_DIR, m_outImgDir);
	DDX_Text(pDX, IDC_EDIT_INTERVAL, m_editInterval);
	DDX_Text(pDX, IDC_EDIT_NUMS, m_editNums);
	DDX_Check(pDX, IDC_CHECK_CREATE_SUB_FOLD, m_checkCreateSubFold);
	DDX_Check(pDX, IDC_CHECK_OPEN_AFTER_COMPLETED, m_checkOpenAfterCompleted);
	DDX_Radio(pDX, IDC_RADIO_FORMAT_BMP, m_radioFormatBMP);
	DDX_Control(pDX, IDC_BUTTON_RUN, m_btnRun);
	DDX_Control(pDX, IDC_BUTTON_PAUSE, m_btnPause);
	DDX_Control(pDX, IDC_BUTTON_STOP, m_btnStop);
	DDX_Control(pDX, IDC_PROGRESS_RUN, m_progressRun);
}

BEGIN_MESSAGE_MAP(CAutoShotDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CAutoShotDlg::OnBnClickedButton1)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_INTERVAL, &CAutoShotDlg::OnDeltaposSpinInterval)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_NUMS, &CAutoShotDlg::OnDeltaposSpinNums)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_RUN, &CAutoShotDlg::OnBnClickedButtonRun)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CAutoShotDlg::OnBnClickedButtonStop)
	ON_WM_TIMER()
	ON_STN_CLICKED(IDC_STATIC_PIC_FINDER, &CAutoShotDlg::OnStnClickedStaticPicFinder)
END_MESSAGE_MAP()


// CAutoShotDlg 消息处理程序

BOOL CAutoShotDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	ShowWindow(SW_NORMAL);

	// TODO: 在此添加额外的初始化代码
	this->OnChangeWndHandle();


	this->UpdateData(TRUE);
	m_outImgDir = AfxGetApp()->GetProfileString(_T("Setting"), _T("m_outImgDir"));
	m_editInterval = AfxGetApp()->GetProfileInt(_T("Setting"), _T("m_editInterval"), 2000);
	m_editNums = AfxGetApp()->GetProfileInt(_T("Setting"), _T("m_editNums"), 1000);
	m_checkCreateSubFold = AfxGetApp()->GetProfileInt(_T("Setting"), _T("m_checkCreateSubFold"), BST_CHECKED);
	m_checkOpenAfterCompleted = AfxGetApp()->GetProfileInt(_T("Setting"), _T("m_checkOpenAfterCompleted"), BST_UNCHECKED);

	m_radioFormatBMP = AfxGetApp()->GetProfileInt(_T("Setting"), _T("m_radioFormatBMP"), 2);
	/*
	m_radioFormatPNG = AfxGetApp()->GetProfileInt(_T("Setting"), _T("m_radioFormatPNG"), 2);
	m_radioFormatJPG = AfxGetApp()->GetProfileInt(_T("Setting"), _T("m_radioFormatJPG"), 2);*/
	this->UpdateData(FALSE);

	return FALSE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CAutoShotDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialogEx::OnSysCommand(nID, lParam);
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CAutoShotDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CAutoShotDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CAutoShotDlg::OnChangeWndHandle()
{
	UpdateData(TRUE);

	auto hWnd = (HWND)_tcstoul(m_strHandle, NULL, 16);

	if (!::IsWindow(hWnd) && hWnd != 0)
	{
		return;
	}


	::GetWindowText(hWnd, m_strCaption.GetBufferSetLength(MAXWORD), MAXWORD);
	::GetClassName(hWnd, m_strClass.GetBufferSetLength(MAXWORD), MAXWORD);

	WINDOWINFO wi;
	wi.cbSize = sizeof(WINDOWINFO);
	::GetWindowInfo(hWnd, &wi);
	m_strStyle.Format(_T("%p"), wi.dwStyle);
	CRect rc{ wi.rcWindow };
	m_strRect.Format(_T("(%d,%d)-(%d,%d) %dx%d"), rc.left, rc.top, rc.right, rc.bottom, rc.Width(), rc.Height());

	UpdateData(FALSE);
}


//
//void CAutoShotDlg::OnBnClickedOk()
//{
//	// TODO: 在此添加控件通知处理程序代码
//	//CDialogEx::OnOK();
//}
//
//
//void CAutoShotDlg::OnBnClickedCancel()
//{
//	
//	
//}


void CAutoShotDlg::OnBnClickedButton1()
{
	CString dir;
	CFolderPickerDialog dlg_folder;
	if (dlg_folder.DoModal() == IDOK) {
		dir = dlg_folder.GetPathName();
	} else {
		dir = "";
	}

	UpdateData(TRUE);
	m_outImgDir.Format(_T("%s"), dir);
	UpdateData(FALSE);
}


void CAutoShotDlg::OnDeltaposSpinInterval(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	UpdateData(true);
	CString ss;
	if (pNMUpDown->iDelta == -1)
	{
		m_editInterval += 100;
	}
	else if (pNMUpDown->iDelta == 1)
	{
		if (m_editInterval > 100) {
			m_editInterval -= 100;
		}
		else {
			m_editInterval = 0;
		}
	}
	UpdateData(false);
	*pResult = 0;
}


void CAutoShotDlg::OnDeltaposSpinNums(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	UpdateData(true);
	CString ss;
	if (pNMUpDown->iDelta == -1)
	{
		m_editNums += 5;
	}
	else if (pNMUpDown->iDelta == 1)
	{
		if (m_editNums > 5) {
			m_editNums -= 5;
		}
		else {
			m_editNums = 0;
		}
	}
	UpdateData(false);
	*pResult = 0;
}


void CAutoShotDlg::OnClose()
{
	auto confirm = MessageBox(_T("是否退出程序？"), _T("警告"), MB_OKCANCEL | MB_ICONQUESTION);
	if (confirm == IDOK) {
		UpdateData(TRUE);
		AfxGetApp()->WriteProfileString(_T("Setting"), _T("m_outImgDir"), m_outImgDir);
		AfxGetApp()->WriteProfileInt(_T("Setting"), _T("m_editInterval"), m_editInterval);
		AfxGetApp()->WriteProfileInt(_T("Setting"), _T("m_editNums"), m_editNums);
		AfxGetApp()->WriteProfileInt(_T("Setting"), _T("m_checkCreateSubFold"), m_checkCreateSubFold);
		AfxGetApp()->WriteProfileInt(_T("Setting"), _T("m_checkOpenAfterCompleted"), m_checkOpenAfterCompleted);
		AfxGetApp()->WriteProfileInt(_T("Setting"), _T("m_radioFormatBMP"), m_radioFormatBMP);
		UpdateData(FALSE);

		KillTimer(1);
		CDialogEx::OnClose();
	}
}


void CAutoShotDlg::OnBnClickedButtonRun()
{
	::InvalidateRect(0, nullptr, 1);
	UpdateData(true);
	SetEnableAllComponents(false);
	m_btnPause.EnableWindow(true);
	m_btnPause.SetWindowTextW(_T("暂停 (&P)"));
	m_btnStop.EnableWindow(true);

	TCHAR buffer[80];
	{   // 时间戳当作子文件夹名
		auto now = std::chrono::system_clock::now();
		std::time_t t = std::chrono::system_clock::to_time_t(now);
		std::tm* now_tm = std::localtime(&t);
		std::wcsftime(buffer, sizeof(buffer), _T("%Y-%m-%d-%H-%M-%S"), now_tm);
	}
	m_outImgDirFinal = m_outImgDir + _T("\\") + buffer;
	bool res = std::filesystem::create_directories(m_outImgDirFinal.GetString());
	if (!res) {
		MessageBox(_T("创建目录失败"), _T("错误"), MB_ICONERROR);
		return;
	}

	m_progressRun.SetRange(0, m_editNums);
	m_progressRun.SetPos(0);
	m_progressRun.SetStep(1);

	/*if (m_timer) {
		m_timer->stop();
	}
	m_timer = std::make_unique<FHighResTimer>(std::chrono::milliseconds(m_editInterval), true, [&]() { this->TimerCallback(); });
	m_timer->start();*/
	KillTimer(1);
	SetTimer(1, m_editInterval, nullptr);

	UpdateData(false);
}

void CAutoShotDlg::SetEnableAllComponents(bool enabled)
{
	UpdateData(true);
	auto excepts = { IDC_PROGRESS_RUN, IDC_CHECK_OPEN_AFTER_COMPLETED, IDC_BUTTON_STOP, IDC_BUTTON_PAUSE, IDC_EDIT_PROGRESS_RATIO };
	for (int i = 1000; i <= 2000; ++i) {
		CWnd* pWnd = GetDlgItem(i);
		if (!pWnd)
			continue;
		if (std::any_of(excepts.begin(), excepts.end(), [i](int except) { return i == except; })) {
			pWnd->EnableWindow(true);
		}
		else {
			pWnd->EnableWindow(enabled);
		}
	}
	UpdateData(false);
}


void CAutoShotDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	ExitProcess(0);
}


void CAutoShotDlg::OnBnClickedButtonStop()
{
	auto confirm = MessageBox(_T("任务正在进行中，是否停止？"), _T("警告"), MB_OKCANCEL | MB_ICONQUESTION);
	if (confirm == IDOK) {
		KillTimer(1);
		m_progressRun.SetPos(0);
		SetEnableAllComponents(true);
		m_btnPause.EnableWindow(false);
		m_btnPause.SetWindowTextW(_T("暂停 (&P)"));
		m_btnStop.EnableWindow(false);
	}
}


void CAutoShotDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1) {
		this->TimerCallback();
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CAutoShotDlg::OnStnClickedStaticPicFinder()
{
	UpdateData(true);
	UpdateData(false);
}
