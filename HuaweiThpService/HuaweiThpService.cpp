#include "HuaweiThpService.h"
#include "XmlOperator.h"

// 静态成员初始化
SERVICE_STATUS HuaweiThpService::g_ServiceStatus{};
SERVICE_STATUS_HANDLE HuaweiThpService::g_StatusHandle{nullptr};
HuaweiThpService *HuaweiThpService::g_ServiceInstance{nullptr};

CallBackFunc HuaweiThpService::s_PrintEventLog{nullptr};
CallBackFunc HuaweiThpService::s_EventLogStatus{nullptr};
CallBackFunc HuaweiThpService::s_SetPenEleValue{nullptr};
CallBackFunc HuaweiThpService::s_GetPenEleValue{nullptr};

// 构造函数
HuaweiThpService::HuaweiThpService() noexcept
	: m_hDll{nullptr}, m_ThpFuncStart{nullptr}, m_ThpFuncStop{nullptr}, m_RegisterPrintEventLog{nullptr}, m_RegisterEventLogStatus{nullptr}, m_RegisterGetPenEleValue{nullptr}, m_RegisterSetPenEleValue{nullptr}, m_GetMESSAGE{nullptr}
{
	g_ServiceInstance = this;

	// 设置进程优先级为实时
	SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
}

// 析构函数
HuaweiThpService::~HuaweiThpService() noexcept
{
	UnloadThpDll();
}

// 服务主入口点
void WINAPI HuaweiThpService::ServiceMain(DWORD argc, LPSTR *argv) noexcept
{
	// 注册服务控制处理器
	g_StatusHandle = RegisterServiceCtrlHandlerA(SERVICE_NAME, ServiceCtrlHandler);
	if (g_StatusHandle == nullptr) [[unlikely]]
	{
		return;
	}

	// 初始化服务状态
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;

	// 报告初始状态
	if (g_ServiceInstance) [[likely]]
	{
		g_ServiceInstance->SetServiceStatus(SERVICE_START_PENDING);
	}

	// 启动服务
	if (g_ServiceInstance) [[likely]]
	{
		g_ServiceInstance->Start();
	}
}

// 服务控制处理器
void WINAPI HuaweiThpService::ServiceCtrlHandler(DWORD ctrlCode) noexcept
{
	switch (ctrlCode)
	{
	case SERVICE_CONTROL_STOP:
		if (g_ServiceInstance) [[likely]]
		{
			g_ServiceInstance->SetServiceStatus(SERVICE_STOP_PENDING);
			g_ServiceInstance->Stop();
			g_ServiceInstance->SetServiceStatus(SERVICE_STOPPED);
		}
		break;

	case SERVICE_CONTROL_SHUTDOWN:
		if (g_ServiceInstance) [[likely]]
		{
			g_ServiceInstance->Stop();
			g_ServiceInstance->SetServiceStatus(SERVICE_STOPPED);
		}
		break;

	case SERVICE_CONTROL_INTERROGATE:
		// 报告当前状态
		break;

	case SERVICE_CONTROL_PRESHUTDOWN:
		// 处理预关机
		break;

	default:
		break;
	}
}

// 启动服务
void HuaweiThpService::Start() noexcept
{
	// 加载 THP DLL
	if (!LoadThpDll()) [[unlikely]]
	{
		SetServiceStatus(SERVICE_STOPPED, ERROR_FILE_NOT_FOUND);
		return;
	}

	// 注册回调函数
	FunctionRegister();

	// 调用 THP 启动函数
	if (m_ThpFuncStart) [[likely]]
	{
		m_ThpFuncStart();
	}

	// 报告运行状态
	SetServiceStatus(SERVICE_RUNNING);
}

// 停止服务
void HuaweiThpService::Stop() noexcept
{
	// 调用 THP 停止函数
	if (m_ThpFuncStop) [[likely]]
	{
		m_ThpFuncStop();
	}

	// 卸载 DLL
	UnloadThpDll();
}

// 运行服务
void HuaweiThpService::Run() noexcept
{
	SERVICE_TABLE_ENTRYA ServiceTable[]{
		{const_cast<LPSTR>(SERVICE_NAME), ServiceMain},
		{nullptr, nullptr}};

	StartServiceCtrlDispatcherA(ServiceTable);
}

// 加载 THP DLL
bool HuaweiThpService::LoadThpDll() noexcept
{
	m_hDll = LoadLibraryA("THP_Service.dll");
	if (m_hDll == nullptr) [[unlikely]]
	{
		return false;
	}

	// 加载函数指针
	m_ThpFuncStart = reinterpret_cast<ThpFuncStartPtr>(GetProcAddress(m_hDll, "ThpFuncStart"));
	m_ThpFuncStop = reinterpret_cast<ThpFuncStopPtr>(GetProcAddress(m_hDll, "ThpFuncStop"));
	m_RegisterPrintEventLog = reinterpret_cast<RegisterPrintEventLogPtr>(GetProcAddress(m_hDll, "RegisterPrintEventLog"));
	m_RegisterEventLogStatus = reinterpret_cast<RegisterEventLogStatusPtr>(GetProcAddress(m_hDll, "RegisterEventLogStatus"));
	m_RegisterGetPenEleValue = reinterpret_cast<RegisterGetPenEleValuePtr>(GetProcAddress(m_hDll, "RegisterGetPenEleValue"));
	m_RegisterSetPenEleValue = reinterpret_cast<RegisterSetPenEleValuePtr>(GetProcAddress(m_hDll, "RegisterSetPenEleValue"));
	m_GetMESSAGE = reinterpret_cast<GetMESSAGEPtr>(GetProcAddress(m_hDll, "GetMESSAGE"));

	// 检查是否所有函数都成功加载
	if (!m_ThpFuncStart || !m_ThpFuncStop || !m_RegisterPrintEventLog ||
		!m_RegisterEventLogStatus || !m_RegisterGetPenEleValue ||
		!m_RegisterSetPenEleValue || !m_GetMESSAGE) [[unlikely]]
	{
		UnloadThpDll();
		return false;
	}

	return true;
}

// 卸载 THP DLL
void HuaweiThpService::UnloadThpDll() noexcept
{
	if (m_hDll) [[likely]]
	{
		FreeLibrary(m_hDll);
		m_hDll = nullptr;
	}

	m_ThpFuncStart = nullptr;
	m_ThpFuncStop = nullptr;
	m_RegisterPrintEventLog = nullptr;
	m_RegisterEventLogStatus = nullptr;
	m_RegisterGetPenEleValue = nullptr;
	m_RegisterSetPenEleValue = nullptr;
	m_GetMESSAGE = nullptr;
}

// 注册回调函数
void HuaweiThpService::FunctionRegister() const noexcept
{
	// 设置回调函数
	s_PrintEventLog = PrintEventLogCallback;
	s_EventLogStatus = EventLogStatusCallback;
	s_SetPenEleValue = SetPenEleValueCallback;
	s_GetPenEleValue = GetPenEleValueCallback;

	// 向 DLL 注册回调函数
	if (m_RegisterPrintEventLog) [[likely]]
	{
		m_RegisterPrintEventLog(s_PrintEventLog);
	}

	if (m_RegisterEventLogStatus) [[likely]]
	{
		m_RegisterEventLogStatus(s_EventLogStatus);
	}

	if (m_RegisterSetPenEleValue) [[likely]]
	{
		m_RegisterSetPenEleValue(s_SetPenEleValue);
	}

	if (m_RegisterGetPenEleValue) [[likely]]
	{
		m_RegisterGetPenEleValue(s_GetPenEleValue);
	}
}

// 设置服务状态
void HuaweiThpService::SetServiceStatus(DWORD currentState, DWORD win32ExitCode, DWORD waitHint) noexcept
{
	static DWORD dwCheckPoint{1};

	g_ServiceStatus.dwCurrentState = currentState;
	g_ServiceStatus.dwWin32ExitCode = win32ExitCode;
	g_ServiceStatus.dwWaitHint = waitHint;

	if (currentState == SERVICE_START_PENDING) [[unlikely]]
	{
		g_ServiceStatus.dwControlsAccepted = 0;
	}
	else [[likely]]
	{
		g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	}

	if (currentState == SERVICE_RUNNING || currentState == SERVICE_STOPPED) [[likely]]
	{
		g_ServiceStatus.dwCheckPoint = 0;
	}
	else [[unlikely]]
	{
		g_ServiceStatus.dwCheckPoint = dwCheckPoint++;
	}

	::SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

// 回调函数实现
int HuaweiThpService::PrintEventLogCallback(int arg) noexcept
{
	if (g_ServiceInstance && g_ServiceInstance->m_GetMESSAGE) [[likely]]
	{
		constexpr int bufferSize{MESSAGE_LENGTH};
		char buffer[MESSAGE_LENGTH]{};
		int size{bufferSize};
		g_ServiceInstance->m_GetMESSAGE(buffer, &size);

		// 如需要，记录到 Windows 事件日志
		// 可以在此处添加 EventLog 功能
	}
	return 0;
}

int HuaweiThpService::EventLogStatusCallback(int arg) noexcept
{
	// 事件日志状态回调
	return 0;
}

int HuaweiThpService::SetPenEleValueCallback(int arg) noexcept
{
	// 调用 XmlOperator 设置值
	return XmlOperator::GetInstance().SetPenEleValue(arg);
}

int HuaweiThpService::GetPenEleValueCallback(int arg) noexcept
{
	// 调用 XmlOperator 获取值
	return XmlOperator::GetInstance().GetPenEleValue(arg);
}
