#ifndef _SERIAL_PORT_H
#define _SERIAL_PORT_H

#include <iostream>
#include "afxdialogex.h"
using namespace std;

typedef struct
{
	char* running_flag;
	HANDLE* serial_handle;
	HWND hMsg;
	UINT MsgType;
	unsigned char* buffer;
	unsigned int buff_size;
	unsigned short frame_size;
} thread_param;


class SerialPort
{
public:
	SerialPort();
	~SerialPort();
	// 打开串口,成功返回true，失败返回false
	// portname(串口名): 在Windows下是"COM1""COM2"等
	// baudrate(波特率): 9600、19200、38400、43000、56000、57600、115200 
	// parity(校验位): 0为无校验，1为奇校验，2为偶校验，3为标记校验
	// databit(数据位): 4-8，通常为8位
	// stopbit(停止位): 1为1位停止位，2为2位停止位,3为1.5位停止位
	// synchronizable(同步、异步): 0为异步，1为同步
	int EmumSerialsToBox(CComboBox* cmbox);
	void VaildateSerialsFromBox(CComboBox* cmbox);

	bool OpenPort(CString portname, BOOL synchronizeflag = FALSE,
		int baudrate = 115200, DWORD read_timeout=100, int in_buf_len=1024, int out_buf_len=1024,
		char parity = NOPARITY, 
		char databit = 0x08,
		char stopbit = ONESTOPBIT);

	//关闭串口
	void Close();

	//串口是否打开
	BOOL isOpen();

	COMMTIMEOUTS GetTimeoutStruct();

	int SetCommTimeout(COMMTIMEOUTS time_out);

	//发送数据或写数据，成功返回发送数据长度，失败返回0
	int SendData(unsigned char* dat, unsigned int len);

	//接受数据或读数据，成功返回读取实际数据的长度，失败返回0(请预先分配储存区域)
	int ReceiveData(unsigned char* pData, unsigned int buffer_len);

	//启动接收线程，数据以sendmessage的形式返回，frame_len默认应为1，成功则返回0
	int StartReceiveThread(HWND hMsg, UINT Msg_type, 
		unsigned int buff_len, unsigned short frame_len);

	void StopReceiveThread();
private:
	HANDLE hThread = INVALID_HANDLE_VALUE;
	HANDLE SerialHandle = INVALID_HANDLE_VALUE;
	BOOL synchronizeflag = FALSE;
	COMMTIMEOUTS TimeOuts;

	char thread_running = 0;
	thread_param m_para;
};

DWORD WINAPI _Receiving(LPVOID sparam);

#endif
