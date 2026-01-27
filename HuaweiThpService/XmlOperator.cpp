#include "XmlOperator.h"
#include <Windows.h>
#include <fstream>
#include <sstream>

// 单例实例获取器
XmlOperator &XmlOperator::GetInstance() noexcept
{
	static XmlOperator instance{};
	return instance;
}

// 构造函数
XmlOperator::XmlOperator() noexcept
{
	CreateXmlFile();
}

// 如果 XML 文件不存在则创建
void XmlOperator::CreateXmlFile() noexcept
{
	try
	{
		// 检查文件是否存在
		const DWORD dwAttrib{GetFileAttributesA(XML_FILE_PATH)};
		if (dwAttrib != INVALID_FILE_ATTRIBUTES) [[likely]]
		{
			return; // 文件已存在
		}

		// 如果目录不存在则创建
		constexpr const char *huaweiDir{"C:\\Program Files\\Huawei"};
		constexpr const char *serviceDir{"C:\\Program Files\\Huawei\\HuaweiThpService"};
		CreateDirectoryA(huaweiDir, nullptr);
		CreateDirectoryA(serviceDir, nullptr);

		// 创建 XML 内容
		std::ostringstream xmlContent{};
		xmlContent << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
		xmlContent << "<Config>\n";
		xmlContent << "  <VHFFunction>0</VHFFunction>\n";
		xmlContent << "  <LogFunction>0</LogFunction>\n";
		xmlContent << "</Config>\n";

		// 写入文件
		std::ofstream file{XML_FILE_PATH, std::ios::out | std::ios::trunc};
		if (file.is_open()) [[likely]]
		{
			file << xmlContent.str();
			file.close();
		}
	}
	catch (...)
	{
		// 如需要，记录异常
	}
}

// 检查文件是否为有效的 XML
bool XmlOperator::IsXml(const std::string &path) noexcept
{
	try
	{
		std::ifstream file{path};
		if (!file.is_open()) [[unlikely]]
		{
			return false;
		}

		const std::string content{
			std::istreambuf_iterator<char>{file},
			std::istreambuf_iterator<char>{}};
		file.close();

		// 简单的 XML 验证 - 检查必需的元素
		if (content.find("<?xml") == std::string::npos) [[unlikely]]
		{
			return false;
		}
		if (content.find("<Config>") == std::string::npos) [[unlikely]]
		{
			return false;
		}
		if (content.find("</Config>") == std::string::npos) [[unlikely]]
		{
			return false;
		}

		return true;
	}
	catch (...)
	{
		return false;
	}
}

// 获取笔元素值
int XmlOperator::GetPenEleValue(int a) noexcept
{
	const std::lock_guard<std::mutex> lock{m_mutex};

	try
	{
		// 检查文件是否存在
		const DWORD dwAttrib{GetFileAttributesA(XML_FILE_PATH)};
		if (dwAttrib == INVALID_FILE_ATTRIBUTES) [[unlikely]]
		{
			CreateXmlFile();
		}

		// 检查文件是否为有效的 XML
		if (!IsXml(XML_FILE_PATH)) [[unlikely]]
		{
			DeleteFileA(XML_FILE_PATH);
			return 0;
		}

		// 读取文件内容
		std::ifstream file{XML_FILE_PATH};
		if (!file.is_open()) [[unlikely]]
		{
			return 0;
		}

		const std::string content{
			std::istreambuf_iterator<char>{file},
			std::istreambuf_iterator<char>{}};
		file.close();

		// 确定要搜索的标签
		std::string tagName{};
		switch (a)
		{
		case VHF_FUNCTION:
			tagName = "VHFFunction";
			break;
		case LOG_FUNCTION:
			tagName = "LogFunction";
			break;
		default:
			return 0;
		}

		// 使用简单的字符串解析提取值
		const std::string openTag{"<" + tagName + ">"};
		const std::string closeTag{"</" + tagName + ">"};

		size_t startPos{content.find(openTag)};
		if (startPos == std::string::npos) [[unlikely]]
		{
			return 0;
		}

		startPos += openTag.length();
		const size_t endPos{content.find(closeTag, startPos)};
		if (endPos == std::string::npos) [[unlikely]]
		{
			return 0;
		}

		std::string valueStr{content.substr(startPos, endPos - startPos)};

		// 去除空白
		valueStr.erase(0, valueStr.find_first_not_of(" \t\n\r"));
		valueStr.erase(valueStr.find_last_not_of(" \t\n\r") + 1);

		if (valueStr.empty()) [[unlikely]]
		{
			return 0;
		}

		// 解析整数值
		try
		{
			return std::stoi(valueStr);
		}
		catch (...)
		{
			return 0;
		}
	}
	catch (...)
	{
		return 0;
	}
}

// 设置笔元素值
int XmlOperator::SetPenEleValue(int penFunc) noexcept
{
	const std::lock_guard<std::mutex> lock{m_mutex};

	try
	{
		// 检查文件是否存在
		const DWORD dwAttrib{GetFileAttributesA(XML_FILE_PATH)};
		if (dwAttrib == INVALID_FILE_ATTRIBUTES) [[unlikely]]
		{
			CreateXmlFile();
		}

		// 检查文件是否为有效的 XML
		if (!IsXml(XML_FILE_PATH)) [[unlikely]]
		{
			DeleteXmlFile();
			return -1;
		}

		// 读取文件内容
		std::ifstream file{XML_FILE_PATH};
		if (!file.is_open()) [[unlikely]]
		{
			return -1;
		}

		std::string content{
			std::istreambuf_iterator<char>{file},
			std::istreambuf_iterator<char>{}};
		file.close();

		// 查找并替换 VHFFunction 值
		constexpr const char *tagName{"VHFFunction"};
		const std::string openTag{"<" + std::string{tagName} + ">"};
		const std::string closeTag{"</" + std::string{tagName} + ">"};

		size_t startPos{content.find(openTag)};
		if (startPos == std::string::npos) [[unlikely]]
		{
			return -1;
		}

		startPos += openTag.length();
		const size_t endPos{content.find(closeTag, startPos)};
		if (endPos == std::string::npos) [[unlikely]]
		{
			return -1;
		}

		// 替换值
		const std::string newValue{std::to_string(penFunc)};
		content.replace(startPos, endPos - startPos, newValue);

		// 写回文件
		try
		{
			std::ofstream outFile{XML_FILE_PATH, std::ios::out | std::ios::trunc};
			if (!outFile.is_open()) [[unlikely]]
			{
				DeleteXmlFile();
				return -1;
			}

			outFile << content;
			outFile.close();
			return 0;
		}
		catch (...)
		{
			DeleteXmlFile();
			return -1;
		}
	}
	catch (...)
	{
		return -1;
	}
}

// 删除 XML 文件
void XmlOperator::DeleteXmlFile() noexcept
{
	try
	{
		SetFileAttributesA(XML_FILE_PATH, FILE_ATTRIBUTE_NORMAL);
		DeleteFileA(XML_FILE_PATH);
	}
	catch (...)
	{
		// 忽略删除错误
	}
}
