#pragma once

#include "CallBackFunc.h"
#include <Windows.h>

// 华为触摸服务类
class HuaweiThpService
{
public:
	HuaweiThpService() noexcept;
	~HuaweiThpService() noexcept;

	// 服务入口点
	static void WINAPI ServiceMain(DWORD argc, LPSTR *argv) noexcept;
	static void WINAPI ServiceCtrlHandler(DWORD ctrlCode) noexcept;

	// 启动服务
	void Start() noexcept;

	// 停止服务
	void Stop() noexcept;

	// 运行服务
	void Run() noexcept;

private:
	// 服务状态
	static SERVICE_STATUS g_ServiceStatus;
	static SERVICE_STATUS_HANDLE g_StatusHandle;
	static HuaweiThpService *g_ServiceInstance;

	// 服务常量
	static constexpr const char *SERVICE_NAME{"HUAWEIThpService"};
	static constexpr int MESSAGE_LENGTH{64};

	// DLL 函数声明
	using ThpFuncStartPtr = int (*)() noexcept;
	using ThpFuncStopPtr = int (*)() noexcept;
	using RegisterPrintEventLogPtr = void (*)(CallBackFunc func) noexcept;
	using RegisterEventLogStatusPtr = void (*)(CallBackFunc func) noexcept;
	using RegisterGetPenEleValuePtr = void (*)(CallBackFunc func) noexcept;
	using RegisterSetPenEleValuePtr = void (*)(CallBackFunc func) noexcept;
	using GetMESSAGEPtr = void (*)(char *buffer, int *bufferSize) noexcept;

	// DLL 句柄
	HMODULE m_hDll;

	// 函数指针
	ThpFuncStartPtr m_ThpFuncStart;
	ThpFuncStopPtr m_ThpFuncStop;
	RegisterPrintEventLogPtr m_RegisterPrintEventLog;
	RegisterEventLogStatusPtr m_RegisterEventLogStatus;
	RegisterGetPenEleValuePtr m_RegisterGetPenEleValue;
	RegisterSetPenEleValuePtr m_RegisterSetPenEleValue;
	GetMESSAGEPtr m_GetMESSAGE;

	// 回调函数
	static CallBackFunc s_PrintEventLog;
	static CallBackFunc s_EventLogStatus;
	static CallBackFunc s_SetPenEleValue;
	static CallBackFunc s_GetPenEleValue;

	// 辅助方法
	[[nodiscard]] bool LoadThpDll() noexcept;
	void UnloadThpDll() noexcept;
	void FunctionRegister() const noexcept;
	void SetServiceStatus(DWORD currentState, DWORD win32ExitCode = NO_ERROR, DWORD waitHint = 0) noexcept;

	// 回调函数实现
	static int PrintEventLogCallback(int arg) noexcept;
	static int EventLogStatusCallback(int arg) noexcept;
	static int SetPenEleValueCallback(int arg) noexcept;
	static int GetPenEleValueCallback(int arg) noexcept;
};
