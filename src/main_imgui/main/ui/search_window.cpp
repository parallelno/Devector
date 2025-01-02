#include "ui/search_window.h"

#include <format>
#include "utils/str_utils.h"

dev::SearchWindow::SearchWindow(Hardware& _hardware, Debugger& _debugger,
	const float* const _dpiScaleP,
	ReqUI& _reqUI)
	:
	BaseWindow("Search", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, _dpiScaleP),
	m_hardware(_hardware), m_debugger(_debugger), 
	m_reqUI(_reqUI)
{}

void dev::SearchWindow::Update(bool& _visible, const bool _isRunning)
{
	BaseWindow::Update();

	if (_visible && ImGui::Begin(m_name.c_str(), &_visible, ImGuiWindowFlags_NoCollapse))
	{
		UpdateData(_isRunning);
		Draw(_isRunning);

		ImGui::End();
	}
}

void dev::SearchWindow::Draw(const bool _isRunning)
{
	if (m_searchEnabled) ImGui::BeginDisabled();

	if (ImGui::InputInt("Start Address", &m_searchStartAddr, 1, 100, ImGuiInputTextFlags_CharsHexadecimal))
	{
		m_searchStartAddr = dev::Max(0, m_searchStartAddr);
		m_searchStartAddr = dev::Min(m_searchStartAddr, Memory::MEMORY_GLOBAL_LEN - 1);
	};

	if (ImGui::InputInt("End Address", &m_searchEndAddr, 1, 100, ImGuiInputTextFlags_CharsHexadecimal))
	{
		m_searchEndAddr = dev::Max(0, m_searchEndAddr);
		m_searchEndAddr = dev::Min(m_searchEndAddr, Memory::MEMORY_GLOBAL_LEN - 1);
	}

	if (m_searchEnabled) ImGui::EndDisabled();
	
	if (ImGui::InputInt("Search Value", &m_searchVal, 1, 100, ImGuiInputTextFlags_CharsHexadecimal))
	{
		m_searchVal = dev::Max(0, m_searchVal);
		m_searchVal = dev::Min(m_searchVal, 0xFF);
	}

	if (ImGui::Checkbox("Start Search", &m_searchEnabled))
	{
		// search started, so clean the results
		m_searchResults.clear();

		if (m_searchEnabled){
			const auto& memP = *m_hardware.GetRam();

			for (int addr = m_searchStartAddr; addr <= m_searchEndAddr; addr++)
			{
				if (memP[addr] == m_searchVal)
				{
					m_searchResults.push_back(addr);
				}
			}
		}
	} 

	if (!m_searchEnabled) ImGui::BeginDisabled();

	if (ImGui::Button("Update Search"))
	{
		const auto& memP = *m_hardware.GetRam();

		auto m_searchResultsIt = m_searchResults.begin();

		while (m_searchResultsIt != m_searchResults.end())
		{
			auto addr = *m_searchResultsIt;

			if (memP[addr] != m_searchVal)
			{
				m_searchResultsIt = m_searchResults.erase(m_searchResultsIt);
			}
			else
			{
				m_searchResultsIt++;
			}
		}
	}

	ImGui::Separator();

	if (m_searchResults.empty())
	{
		ImGui::Text("No results");
	}
	else if (m_searchResults.size() > 100)
	{
		ImGui::Text("Too many results: %d", m_searchResults.size());
	}
	else if (ImGui::BeginListBox("##searchResults", ImVec2(-FLT_MIN, 100)))
	{
		for (auto addr : m_searchResults)
		{
			ImGui::PushID(addr);
			if (ImGui::Selectable(std::format("0x{:04X}", addr).c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
			{
				//m_hardware.Request(Hardware::Req::DEBUGGER_SET_PC, addr);
			}
			ImGui::PopID();
		}
		ImGui::EndListBox();
	}

	if (!m_searchEnabled) ImGui::EndDisabled();
}

void dev::SearchWindow::UpdateData(const bool _isRunning)
{
	// check if the hardware updated its state
	uint64_t cc = m_hardware.Request(Hardware::Req::GET_CC)->at("cc");
	auto ccDiff = cc - m_ccLast;
	if (ccDiff == 0) return;
	m_ccLast = cc;

	// update
}