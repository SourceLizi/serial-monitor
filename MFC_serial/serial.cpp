#include "serial.h" 
#include <stdio.h>
#include <string.h>
 
#include <WinSock2.h>
#include <windows.h>
#include <iostream>

using namespace std;

SerialPort::SerialPort(){}
 
SerialPort::~SerialPort(){
	StopReceiveThread();
	Close();
}

void SerialPort::VaildateSerialsFromBox(CComboBox* cmbox) {
	int counts = cmbox->GetCount();
	CString ComName, ComOpen;
	for (int i = 0; i < counts; i++) {
		cmbox->GetLBText(i, ComName);
		ComOpen = CString(R"(\\.\)") + ComName;
		HANDLE hCom = CreateFile(ComOpen, GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
		if (INVALID_HANDLE_VALUE == hCom) {
			if (SerialHandle != INVALID_HANDLE_VALUE 
				&& cmbox->GetCurSel() == i) {
				StopReceiveThread();
				Close();
			}
			cmbox->DeleteString(i);
		}
		else {
			CloseHandle(hCom);
		}
	}
	if (cmbox->GetCount() == 0)
	{
		cmbox->SetCurSel(-1);
	}
}

int SerialPort::EmumSerialsToBox(CComboBox* cmbox) {
	CHAR Name[32];
	UCHAR szPortName[32];
	LONG Status;
	TCHAR tName[16];
	DWORD dwIndex = 0, Type;
	DWORD dwName = sizeof(Name), dwSizeofPortName = sizeof(szPortName);
	CString CurrText;
	HKEY hKey;
	//long ret0 = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\SERIALCOMM\\"), 0, KEY_READ, &hKey); 
	//打开一个制定的注册表键,成功返回ERROR_SUCCESS即“0”值
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\SERIALCOMM\\"), 0, KEY_READ, &hKey)
		== ERROR_SUCCESS)
	{
		do
		{
			Status = RegEnumValueA(hKey, dwIndex++, Name, &dwName, NULL, &Type, szPortName, &dwSizeofPortName);//读取键值 
			if ((Status == ERROR_SUCCESS) || (Status == ERROR_MORE_DATA))
			{
				swprintf(tName, 16, L"%hs", szPortName);
				if (cmbox->FindString(0, tName)) {
					cmbox->AddString(tName);
				}
			}
			//每读取一次dwName和dwSizeofPortName都会被修改 
			dwName = sizeof(Name);
			dwSizeofPortName = sizeof(szPortName);
		} while ((Status == ERROR_SUCCESS) || (Status == ERROR_MORE_DATA));
		RegCloseKey(hKey);
	}
	if (cmbox->GetCurSel() < 0) {
		cmbox->SetCurSel(0);
	}
	return dwIndex - 1;
}
 
bool SerialPort::OpenPort(CString portname, BOOL synchronizeflag,
						int baudrate, DWORD read_timeout,int in_buf_len, int out_buf_len,
						char parity,
						char databit,
						char stopbit)
/*parity=NOPARITY; //无校验
		 ODDPARITY; //奇校验
		 EVENPARITY; //偶校验
		 MARKPARITY; //标记校验
  StopBits = ONESTOPBIT; //1位停止位
			 TWOSTOPBITS; //2位停止位
			 ONE5STOPBITS; //1.5位停止位*/
{
	//波特率为0其余函数调用返回均无异常，但读取的数据有误，应防守避免
	if (baudrate == 0) return false;

	if (databit < 4 || databit > 8) return false;

	this->synchronizeflag = synchronizeflag;
	HANDLE hCom = NULL;
	int synchronize_type;
	if (this->synchronizeflag){
		//同步
		synchronize_type=0;
	}
	else{
		//异步
		synchronize_type=FILE_FLAG_OVERLAPPED;
	}

	portname = CString(R"(\\.\)") + portname;
	hCom = CreateFileW(portname, //串口名
									GENERIC_READ | GENERIC_WRITE, //支持读写
									0, //独占方式，串口不支持共享
									NULL,//安全属性指针，默认值为NULL
									OPEN_EXISTING, //打开现有的串口文件
									synchronize_type, //0：同步方式，FILE_FLAG_OVERLAPPED：异步方式
									NULL);//用于复制文件句柄，默认值为NULL，对串口而言该参数必须置为NULL
	
	if(hCom == INVALID_HANDLE_VALUE)
	{
		return false;
	}
 
	//配置缓冲区大小 
	if(! SetupComm(hCom,in_buf_len, out_buf_len))
	{
		CloseHandle(hCom);
		return false;
	}
 
	// 配置参数 
	DCB p;
	if (!GetCommState(hCom, &p)) {
		CloseHandle(hCom);
		return false;
	}
	p.DCBlength = sizeof(DCB);
	p.fBinary = TRUE;
	p.BaudRate = baudrate; // 波特率
	p.ByteSize = databit; // 数据位
	p.Parity = parity;
	p.StopBits = stopbit;

	p.fInX = FALSE;
	p.fOutX = FALSE;
	p.fRtsControl = RTS_CONTROL_DISABLE;
	p.fDtrControl = DTR_CONTROL_DISABLE;

	COMMCONFIG config;
	memset(&config, 0, sizeof(COMMCONFIG));
	config.dwSize = sizeof(COMMCONFIG);
	config.dcb = p;
	config.dwProviderSubType = PST_RS232;

	if (!SetCommConfig(hCom, &config, sizeof(COMMCONFIG)) || !SetCommState(hCom, &p))
	{
		// 设置参数失败
		CloseHandle(hCom);
		return false;
	}
 
	//超时处理,单位：毫秒
	//总超时＝时间系数×读或写的字符数＋时间常量
	//可设置较小保证在ReadFile缓冲区满之前返回
	//TimeOuts.ReadIntervalTimeout = read_timeout; //读间隔超时
	TimeOuts.ReadIntervalTimeout = read_timeout; //读间隔超时
	TimeOuts.ReadTotalTimeoutMultiplier = 5; //读时间系数
	TimeOuts.ReadTotalTimeoutConstant = read_timeout; //读时间常量
	TimeOuts.WriteTotalTimeoutMultiplier = 0; // 写时间系数
	TimeOuts.WriteTotalTimeoutConstant = 10; //写时间常量
	SetCommTimeouts(hCom,&TimeOuts);
 
	PurgeComm(SerialHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
 
	memcpy(&SerialHandle, &hCom, sizeof(hCom));// 保存句柄
 
	return true;
}

BOOL SerialPort::isOpen() {
	return SerialHandle != INVALID_HANDLE_VALUE;
}
 
void SerialPort::Close()
{
	if (SerialHandle != INVALID_HANDLE_VALUE) {
		FlushFileBuffers(SerialHandle);
		PurgeComm(SerialHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
		CancelIo(SerialHandle);
		CloseHandle(SerialHandle);
		SerialHandle = INVALID_HANDLE_VALUE;
	}
}

COMMTIMEOUTS SerialPort::GetTimeoutStruct() {
	return TimeOuts;
}

int SerialPort::SetCommTimeout(COMMTIMEOUTS time_out) {
	TimeOuts = time_out;
	return SetCommTimeouts(SerialHandle, &TimeOuts);
}
 
int SerialPort::SendData(unsigned char* dat, unsigned int len)
{
	if (SerialHandle == INVALID_HANDLE_VALUE) return -1;
	DWORD dwBytesWrite = 0; //成功写入的数据字节数
	if (this->synchronizeflag)
	{
		// 同步方式
		BOOL bWriteStat = WriteFile(SerialHandle, //串口句柄
			dat, //数据首地址
			len, //要发送的数据字节数
			&dwBytesWrite, //DWORD*，用来接收返回成功发送的数据字节数
			NULL); //NULL为同步发送，OVERLAPPED*为异步发送
		if (!bWriteStat)
		{
			return 0;
		}
		return dwBytesWrite;
	}
	else
	{
		//异步方式
		DWORD dwErrorFlags; //错误标志
		COMSTAT comStat; //通讯状态
		OVERLAPPED m_osWrite; //异步输入输出结构体

		//创建一个用于OVERLAPPED的事件处理，不会真正用到，但系统要求这么做
		memset(&m_osWrite, 0, sizeof(m_osWrite));
		m_osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, L"WriteEvent");

		ClearCommError(SerialHandle, &dwErrorFlags, &comStat); //清除通讯错误，获得设备当前状态
		BOOL bWriteStat = WriteFile(SerialHandle, //串口句柄
			dat, //数据首地址
			len, //要发送的数据字节数
			&dwBytesWrite, //DWORD*，用来接收返回成功发送的数据字节数
			&m_osWrite); //NULL为同步发送，OVERLAPPED*为异步发送
		if (!bWriteStat)
		{
			if (GetLastError() == ERROR_IO_PENDING) //如果串口正在写入
			{
				if (WaitForSingleObject(m_osWrite.hEvent, 1000) == WAIT_OBJECT_0) {
					dwBytesWrite = len;
				}
				//等待写入事件1秒钟
			}
			else
			{
				ClearCommError(SerialHandle, &dwErrorFlags, &comStat); //清除通讯错误
			}
		}
		CancelIoEx(SerialHandle, &m_osWrite);
		CloseHandle(m_osWrite.hEvent); //关闭并释放hEvent内存
		return dwBytesWrite;
	}
}

int SerialPort::StartReceiveThread(HWND hMsg, UINT Msg_type,
	unsigned int buff_len, unsigned short frame_len) {
	if (!synchronizeflag) return -2;
	if (SerialHandle == INVALID_HANDLE_VALUE
		|| hMsg == INVALID_HANDLE_VALUE) return -1;

	m_para.buffer = new unsigned char[buff_len];
	memset(m_para.buffer, 0x0, buff_len * sizeof(char));

	m_para.buff_size = buff_len;
	m_para.frame_size = frame_len;
	m_para.hMsg = hMsg;
	m_para.MsgType = Msg_type;
	m_para.running_flag = &thread_running;
	m_para.serial_handle = &SerialHandle;

	thread_running = 1;
	hThread = CreateThread(NULL, 0, _Receiving, (LPVOID)&m_para, 0, NULL);
	return hThread == INVALID_HANDLE_VALUE;
}


DWORD WINAPI _Receiving(LPVOID sparam) {
	thread_param para = *((thread_param*)sparam);
	DWORD dwErrorFlags, wCount; //错误标志
	COMSTAT comStat; //通讯状态

	while (*(para.running_flag) && *(para.serial_handle) != INVALID_HANDLE_VALUE) {

		ClearCommError(*para.serial_handle, &dwErrorFlags, &comStat); //清除通讯错误，获得设备当前状态
		//if (!comStat.cbInQue)//如果输入缓冲区字节数为0，则返回
		if(comStat.cbInQue < para.frame_size)//防止收到不完整的数据帧
			continue;
		BOOL bReadStat = ReadFile(*para.serial_handle, //串口句柄
			para.buffer, //数据首地址
			para.buff_size, //要读取的数据最大字节数
			&wCount, //DWORD*,用来接收返回成功读取的数据字节数
			NULL); //NULL为同步发送，OVERLAPPED*为异步发送
		if (wCount != 0) {
			SendMessageTimeout(para.hMsg, para.MsgType,
				(WPARAM)para.buffer, (LPARAM)wCount,
				SMTO_NORMAL, 1500, NULL);
			wCount = 0;
			//PurgeComm(*para.serial_handle, PURGE_RXCLEAR | PURGE_RXABORT);
		}
	}
	return 0;
}

void SerialPort::StopReceiveThread() {
	if (hThread != INVALID_HANDLE_VALUE && thread_running != 0) {
		thread_running = 0;
		if (WaitForSingleObject(hThread, 3000) == WAIT_TIMEOUT) {
			TerminateThread(hThread, -1);
		}
		delete m_para.buffer;
		CloseHandle(hThread);
		hThread = INVALID_HANDLE_VALUE;
	}
}
 
int SerialPort::ReceiveData(unsigned char* pData, unsigned int buffer_len)
{
	if (SerialHandle == INVALID_HANDLE_VALUE) return 0;
	memset(pData, 0x0, buffer_len*sizeof(char));
	DWORD wCount = 0;
	if (this->synchronizeflag)
	{
		//同步方式
		BOOL bReadStat = ReadFile(SerialHandle, //串口句柄
			pData, //数据首地址
			buffer_len, //要读取的数据最大字节数
			&wCount, //DWORD*,用来接收返回成功读取的数据字节数
			NULL); //NULL为同步发送，OVERLAPPED*为异步发送
		if (bReadStat == FALSE) return -3;
	}
	else
	{
		//异步方式
		DWORD dwErrorFlags; //错误标志
		COMSTAT comStat; //通讯状态
		OVERLAPPED m_osRead; //异步输入输出结构体
 
		//创建一个用于OVERLAPPED的事件处理，不会真正用到，但系统要求这么做
		memset(&m_osRead, 0, sizeof(m_osRead));
		m_osRead.hEvent = CreateEvent(NULL, TRUE, FALSE, L"ReadEvent");
 
		ClearCommError(SerialHandle, &dwErrorFlags, &comStat); //清除通讯错误，获得设备当前状态
		if (!comStat.cbInQue) return 0; //如果输入缓冲区字节数为0，则返回false
		BOOL bReadStat = ReadFile(SerialHandle, //串口句柄
			pData, //数据首地址
			buffer_len, //要读取的数据最大字节数
			&wCount, //DWORD*,用来接收返回成功读取的数据字节数
			&m_osRead); //NULL为同步发送，OVERLAPPED*为异步发送
		if (!bReadStat)
		{
			if (GetLastError() == ERROR_IO_PENDING) //如果串口正在读取中
			{
				//GetOverlappedResult函数的最后一个参数设为TRUE
				//函数会一直等待，直到读操作完成或由于错误而返回
				GetOverlappedResultEx(SerialHandle, &m_osRead, &wCount,2000,FALSE);
				CancelIoEx(SerialHandle, &m_osRead);
				CloseHandle(m_osRead.hEvent); //关闭并释放hEvent的内存
			}
			else
			{
				ClearCommError(SerialHandle, &dwErrorFlags, &comStat); //清除通讯错误
				CancelIoEx(SerialHandle, &m_osRead);
				CloseHandle(m_osRead.hEvent); //关闭并释放hEvent的内存
				return 0;
			}
		}
	}
	return wCount;
}