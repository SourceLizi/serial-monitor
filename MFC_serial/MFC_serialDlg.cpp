
// MFC_serialDlg.cpp: 实现文件
//
#define POINTS_NUM 300
#define TIME_SCOPE 5
#define WM_SENDMSG WM_USER+1001
#define TIMER_EVENT 205

#include "framework.h"
#include "MFC_serial.h"
#include "MFC_serialDlg.h"
#include "afxdialogex.h"
#include "atlstr.h"
//#include "ChartCtrl/ChartCtrl.h"
#include "ChartCtrl/ChartLineSerie.h"
#include <math.h>
#include <Dbt.h>

#include "serial.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CComboBox* m_combobox;
CEdit* EditControl;

CChartAxis *pAxisX, *pAxisY;
CChartLineSerie *pLineSerie, *pLineSerieRef;
double x[POINTS_NUM], y[POINTS_NUM],Ref[POINTS_NUM];

SerialPort m_serialport;
HWND hWindow;

int Tx, Rx, Err;
LARGE_INTEGER time_old, time_now;
LARGE_INTEGER nFreq;
double delta_time;

BOOL IsTesting = FALSE;
unsigned short current_ref = 0;

void append_message(CString str);

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonTest();
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCserialDlg 对话框

CMFCserialDlg::CMFCserialDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFC_SERIAL_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCserialDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHART, m_chart);
}

BEGIN_MESSAGE_MAP(CMFCserialDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CMFCserialDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CMFCserialDlg::OnBnClickedCancel)
	ON_MESSAGE(WM_SENDMSG, OnSendMsgHandler)
	ON_WM_DEVICECHANGE()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON1, &CMFCserialDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_UPDATE, &CMFCserialDlg::OnBnClickedUpdate)
	ON_BN_CLICKED(IDC_BUTTON2, &CMFCserialDlg::OnBnClickedButtonTest)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON4, &CMFCserialDlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// CMFCserialDlg 消息处理程序

BOOL CMFCserialDlg::OnInitDialog()
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

	// TODO: 在此添加额外的初始化代码
	m_combobox = (CComboBox*)GetDlgItem(IDC_COMBO1);
	EditControl = (CEdit*)GetDlgItem(IDC_EDIT1);
	hWindow = GetSafeHwnd();
	QueryPerformanceFrequency(&nFreq);

	pAxisY = m_chart.CreateStandardAxis(CChartCtrl::LeftAxis);
	pAxisY->SetMinMax(1000, 4500);
	pAxisY->SetAutomatic(false);
	//pAxisY->SetAutomatic(true);
	pAxisX = m_chart.CreateStandardAxis(CChartCtrl::BottomAxis);
	pAxisX->SetMinMax(0, TIME_SCOPE);
	pAxisX->SetAutomatic(false);

	//memset(y, 0, sizeof(double)*POINTS_NUM);
	for (int i = 0; i < POINTS_NUM; i++) {
		x[i] = i;  y[i] = 0; Ref[i] = 0;
	}

	m_chart.RemoveAllSeries();//先清空
	pLineSerieRef = m_chart.CreateLineSerie();
	pLineSerie = m_chart.CreateLineSerie();

	pLineSerieRef->SetColor(RGB(0, 0xCD, 0));
	pLineSerieRef->SetSeriesOrdering(poXOrdering);
	pLineSerie->SetColor(RGB(0xFF, 0, 0));
	pLineSerie->SetSeriesOrdering(poXOrdering);

	m_serialport.EmumSerialsToBox(m_combobox);
	
	SetDlgItemText(IDC_EDIT2, _T("115200"));

	SetDlgItemText(IDC_EDIT7, _T("3000"));
	SetDlgItemText(IDC_EDIT_P, _T("20"));
	SetDlgItemText(IDC_EDIT_I, _T("0"));
	SetDlgItemText(IDC_EDIT_D, _T("15"));

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMFCserialDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFCserialDlg::OnPaint()
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
HCURSOR CMFCserialDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


BOOL CMFCserialDlg::OnDeviceChange(UINT nEventType, DWORD dwData)
{
	switch (nEventType)
	{
	case DBT_DEVICEREMOVECOMPLETE://移除设备
		m_serialport.VaildateSerialsFromBox(m_combobox);
		break;
	case DBT_DEVICEARRIVAL://添加设备
		m_serialport.EmumSerialsToBox(m_combobox);
		break;
	default:
		break;
	}
	return TRUE;
}

void CMFCserialDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CString com, baud;
	GetDlgItemText(IDC_COMBO1, com);
	GetDlgItemText(IDC_EDIT2, baud);

	int baudrate = _ttoi(baud);

	if (!com.IsEmpty()) {
		if (m_serialport.OpenPort(com,TRUE, baudrate,30))
		{
			GetDlgItem(IDC_EDIT2)->EnableWindow(FALSE);
			GetDlgItem(IDC_COMBO1)->EnableWindow(FALSE);
			Tx = 0; Rx = 0; Err = 0;
			append_message(_T("Serial port open OK"));
			//目前线程内只实现了同步接收，因此用异步方式CreateFile在此函数下无效
			if (m_serialport.StartReceiveThread(hWindow, WM_SENDMSG, 
				128, sizeof(struct bundle)) == 0) {
				append_message(_T("Start receiveing"));
			}
			else {
				append_message(_T("Failed when creating thread!"));
			}
		}
		else {
			//SetDlgItemText(IDC_EDIT1, _T("Serial port open failed!"));
			append_message(_T("Serial port open failed!"));
		}
	}
	//CDialogEx::OnOK();
}

afx_msg LRESULT CMFCserialDlg::OnSendMsgHandler(WPARAM w, LPARAM l) {
	/*CString str,tmp;
	for (long i = 0; i < l; i++) {
		tmp.Format(_T("0x%X "), ((unsigned char*)w)[i]);
		str.Append(tmp);
	}
	tmp.Format(_T("count=%d"), l);
	str.Append(tmp);
	append_message(str);*/
	if (ChartEnable && l >= (long)sizeof(struct bundle)) {
		long i = 0;
		unsigned char* raw = (unsigned char *)w;
		while (i + (long)sizeof(struct bundle) <= l) {
			m_data = (struct bundle*)(raw + i);
			if (m_data->magic_head == (short)0xFC03 && m_data->magic_tail == (short)0x03FC) {
				QueryPerformanceCounter(&time_now);
				current_ref = m_data->ref;
				if (point_count < POINTS_NUM) {
					if (point_count == 0) {
						x[0] = 0;
					}
					else {
						delta_time = (double)(time_now.QuadPart - time_old.QuadPart) / nFreq.QuadPart;
						x[point_count] = delta_time + x[point_count - 1];
					}
					y[point_count] = m_data->measure;
					Ref[point_count] = m_data->ref;
					m_chart.EnableRefresh(false);
					pAxisX->SetMinMax(x[0], x[point_count]);
					pLineSerieRef->AddPoint(x[point_count], Ref[point_count]);
					pLineSerie->AddPoint(x[point_count], y[point_count]);
					m_chart.EnableRefresh(true);
					point_count++;
				}
				else {
					for (int k = 0; k < POINTS_NUM - 1; k++)
					{
						x[k] = x[k + 1];
						y[k] = y[k + 1];
						Ref[k] = Ref[k + 1];
					}
					//x[POINTS_NUM - 1] = x[POINTS_NUM - 2] + 1;
					delta_time = (double)(time_now.QuadPart - time_old.QuadPart) / nFreq.QuadPart;
					x[POINTS_NUM - 1] = delta_time + x[POINTS_NUM - 2];
					y[POINTS_NUM - 1] = m_data->measure;
					Ref[POINTS_NUM - 1] = m_data->ref;
					m_chart.EnableRefresh(false);
					pAxisX->SetMinMax(x[0], x[POINTS_NUM - 1]);
					pLineSerieRef->ClearSerie();
					pLineSerieRef->AddPoints(x, Ref, POINTS_NUM);
					pLineSerie->ClearSerie();
					pLineSerie->AddPoints(x, y, POINTS_NUM);
					m_chart.EnableRefresh(true);
				}
				time_old = time_now;
				UpdateStatus(l, 0, 0);
				i += sizeof(m_data);
			}else {
				//detected error
				UpdateStatus(0, 0, 1);
				i++;
			}
		}
	}
	return 0;
}

void CMFCserialDlg::UpdateStatus(int dRx,int dTx, int dErr) {
	CString Status;
	Rx += dRx; Tx += dTx; Err += dErr;
	Status.Format(_T("Tx: %d, Rx: %d, Err: %d"), Tx, Rx, Err);
	SetDlgItemText(IDC_STATIC, Status);
}

void append_message(CString str)
{
	/*int len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, str, -1, msg_buff, len, NULL, NULL);
	::PostMessage(hWindow, WM_SENDMSG, (WPARAM)msg_buff, 0);
	Sleep(50);*/
	CString old_str;
	EditControl->GetWindowText(old_str);
	str += _T("\r\n");
	EditControl->SetWindowText(old_str + str);
	EditControl->UpdateWindow();
	EditControl->LineScroll(EditControl->GetLineCount(), 0);
}


void CMFCserialDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	m_serialport.StopReceiveThread();
	append_message(_T("Stopped receive thread"));
	m_serialport.Close();
	append_message(_T("Closed serial"));
	GetDlgItem(IDC_EDIT2)->EnableWindow(TRUE);
	GetDlgItem(IDC_COMBO1)->EnableWindow(TRUE);
	//CDialogEx::OnCancel();
}

void CMFCserialDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_serialport.StopReceiveThread();
	m_serialport.Close();
	PostQuitMessage(0);
	CDialogEx::OnClose();
}


void CMFCserialDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	if (ChartEnable) {
		SetDlgItemText(IDC_BUTTON1, _T("开始"));
		ChartEnable = FALSE;
	}
	else {
		SetDlgItemText(IDC_BUTTON1, _T("暂停"));
		ChartEnable = TRUE;
	}
}


void CMFCserialDlg::OnBnClickedUpdate()
{
	// TODO: 在此添加控件通知处理程序代码
	if (IsTesting) return;
	CString P, I, D,ref;
	struct pid_para PIDupdate;
	GetDlgItemText(IDC_EDIT_P, P);
	GetDlgItemText(IDC_EDIT_I, I);
	GetDlgItemText(IDC_EDIT_D, D);
	GetDlgItemText(IDC_EDIT7, ref);
	PIDupdate.head = 0xFF;
	PIDupdate.PID[0] = _ttoi(P);
	PIDupdate.PID[1] = _ttoi(I);
	PIDupdate.PID[2] = _ttoi(D);
	PIDupdate.ref = _ttoi(ref);
	int count = m_serialport.SendData((unsigned char*)&PIDupdate,
		sizeof(struct pid_para));
	CTime tm = CTime::GetCurrentTime();
	if (count == sizeof(struct pid_para)) {
		SetDlgItemText(IDC_STATICU,tm.Format(_T("%X\n已更新")));
		UpdateStatus(0, sizeof(struct pid_para), 0);
	}
	else {
		SetDlgItemText(IDC_STATICU, tm.Format(_T("%X\n更新失败！")));
		//SetDlgItemText(IDC_STATICU, _T("更新失败！"));
	}
}

int test_i = 0, sin_value = 0;
unsigned short m_test_value = 3500;

void CMFCserialDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == TIMER_EVENT) {
		//在这里写入测试算法
		if (m_test_value == 3500) m_test_value = 2000;
		else if (m_test_value == 2000) m_test_value = 3500;
		/*sin_value = int(1000*sin(test_i / 80.0));
		m_test_value = sin_value + 3000;
		test_i+=5;
		if (test_i > 500) test_i = 0;*/
		//定时发送
		PIDtest.ref = m_test_value;
		m_serialport.SendData((unsigned char*)&PIDtest,
			sizeof(struct pid_para));
	}
	CDialogEx::OnTimer(nIDEvent);
}


void CMFCserialDlg::OnBnClickedButtonTest()
{
	if (IsTesting) {
		KillTimer(TIMER_EVENT);
		SetDlgItemText(IDC_BUTTON2, _T("开始测试"));
		IsTesting = FALSE;
	}
	else {
		CString P, I, D;
		GetDlgItemText(IDC_EDIT_P, P);
		GetDlgItemText(IDC_EDIT_I, I);
		GetDlgItemText(IDC_EDIT_D, D);
		PIDtest.head = 0xFF;
		PIDtest.PID[0] = _ttoi(P);
		PIDtest.PID[1] = _ttoi(I);
		PIDtest.PID[2] = _ttoi(D);
		SetTimer(TIMER_EVENT, 3000, NULL);
		SetDlgItemText(IDC_BUTTON2, _T("停止测试"));
		IsTesting = TRUE;
	}
	// TODO: 在此添加控件通知处理程序代码
}


void CMFCserialDlg::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码
	FILE* fp;
	int max_points;
	if (point_count == 0)
		return;
	else if (point_count < POINTS_NUM) 
		max_points = point_count;
	else 
		max_points = POINTS_NUM;
	char name_str[32];
	struct tm ltm;
	time_t nowtime = time(NULL);
	localtime_s(&ltm, &nowtime);
	strftime(name_str, 32, "%Y%m%d_%H%M%S-pid_data.csv", &ltm);
	fopen_s(&fp, name_str, "w");
	if (fp != NULL) {
		fprintf_s(fp, "x,y,ref\n");
		for (int i = 0; i < max_points; i++) {
			fprintf_s(fp, "%g,%g,%g\n", x[i]-x[0], y[i], Ref[i]);
		}
		fclose(fp);
		AfxMessageBox(_T("保存成功！"));
	}else AfxMessageBox(_T("保存失败"));
}
