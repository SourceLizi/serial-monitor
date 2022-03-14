
// MFC_serialDlg.h: 头文件
//
#include "resource.h"
#include "ChartCtrl/ChartCtrl.h"

#pragma once


// CMFCserialDlg 对话框
class CMFCserialDlg : public CDialogEx
{
// 构造
public:
	CMFCserialDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFC_SERIAL_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	struct bundle {
		short magic_head;
		unsigned short measure;
		unsigned short ref;
		short magic_tail;
	};

	struct pid_para {
		unsigned char head;
		unsigned char PID[3];
		unsigned short ref;
	};

public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD dwData);
	afx_msg LRESULT OnSendMsgHandler(WPARAM w, LPARAM l);
	void UpdateStatus(int dRx, int dTx, int dErr);
	afx_msg void OnClose();
	CChartCtrl m_chart;
	BOOL ChartEnable = TRUE;
	int point_count = 0;
	struct bundle* m_data;
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedUpdate();
	afx_msg void OnBnClickedButtonTest();
	struct pid_para PIDtest;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButton4();
};
