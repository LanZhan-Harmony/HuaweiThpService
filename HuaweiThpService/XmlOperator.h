#pragma once

#include <mutex>

// XML 配置操作类
// 管理华为触摸服务的 XML 配置文件
class XmlOperator
{
public:
	// 单例实例
	[[nodiscard]] static XmlOperator &GetInstance() noexcept;

	// 从 XML 获取笔元素值
	[[nodiscard]] int GetPenEleValue(int a) noexcept;

	// 将笔元素值设置到 XML
	[[nodiscard]] int SetPenEleValue(int penFunc) noexcept;

private:
	// 单例模式的私有构造函数
	XmlOperator() noexcept;
	~XmlOperator() noexcept = default;

	// 删除拷贝构造函数和赋值运算符
	XmlOperator(const XmlOperator &) = delete;
	XmlOperator &operator=(const XmlOperator &) = delete;
	XmlOperator(XmlOperator &&) = delete;
	XmlOperator &operator=(XmlOperator &&) = delete;

	// XML 文件路径
	static constexpr const char *XML_FILE_PATH{"C:\\Program Files\\Huawei\\HuaweiThpService\\HuaweiTHP.config.xml"};

	// 函数类型常量
	static constexpr int VHF_FUNCTION{0};
	static constexpr int LOG_FUNCTION{1};

	// 线程安全
	std::mutex m_mutex;

	// 辅助方法
	static void CreateXmlFile() noexcept;
	[[nodiscard]] static bool IsXml(const std::string &path) noexcept;
	static void DeleteXmlFile() noexcept;
};
