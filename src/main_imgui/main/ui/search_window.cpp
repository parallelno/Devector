#include "ui/search_window.h"

#include <format>
#include "utils/str_utils.h"

dev::SearchWindow::SearchWindow(Hardware& _hardware, Debugger& _debugger,
								dev::Scheduler& _scheduler,
								bool* _visibleP)
	:
	BaseWindow("Search", DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
		_scheduler, _visibleP),
	m_hardware(_hardware), m_debugger(_debugger)
{}


void dev::SearchWindow::Draw(const dev::Signals _signals,
							dev::Scheduler::SignalData _data)
{
	bool isRunning = dev::Signals::HW_RUNNING & _signals;

	if (m_searchEnabled) ImGui::BeginDisabled();

	if (ImGui::InputInt("Start Address", &m_searchStartAddr, 1, 100,
		ImGuiInputTextFlags_CharsHexadecimal))
	{
		m_searchStartAddr = dev::Max(0, m_searchStartAddr);
		m_searchStartAddr = dev::Min(
			m_searchStartAddr,
			Memory::MEMORY_GLOBAL_LEN - 1);
	};

	if (ImGui::InputInt("End Address", &m_searchEndAddr, 1, 100,
		ImGuiInputTextFlags_CharsHexadecimal))
	{
		m_searchEndAddr = dev::Max(0, m_searchEndAddr);
		m_searchEndAddr = dev::Min(
			m_searchEndAddr, Memory::MEMORY_GLOBAL_LEN - 1);
	}

	if (m_searchEnabled) ImGui::EndDisabled();

	if (ImGui::InputInt("Search Value", &m_searchVal, 1, 100,
		ImGuiInputTextFlags_CharsHexadecimal))
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
		ImGui::Text("Too many results: %d", (int)m_searchResults.size());
	}
	else if (ImGui::BeginListBox("##searchResults", ImVec2(-FLT_MIN, 100)))
	{
		for (auto addr : m_searchResults)
		{
			ImGui::PushID(addr);
			if (ImGui::Selectable(std::format("0x{:04X}", addr).c_str(), false,
				ImGuiSelectableFlags_SpanAllColumns))
			{
				// TODO: check if this is needed
				//m_hardware.Request(Hardware::Req::DEBUGGER_SET_PC, addr);
			}
			ImGui::PopID();
		}
		ImGui::EndListBox();
	}

	if (!m_searchEnabled) ImGui::EndDisabled();
}