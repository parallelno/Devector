#include "telegramBot.h"
#include "utils/utils.h"
#include "utils/jsonUtils.h"
#include <iostream>

#include <fstream>
#include <thread>
#include <future>

#include "httplib.h"
#include "json.hpp"

#include <direct.h>
#include <filesystem>

TelegramBot::TelegramBot(
	const std::string _token,
	const double _updateTimeout,
	const double _reconnectTimeout,
	const unsigned int _limit,
	const unsigned int _timeout
	)
	:
	m_token(_token),
	m_updateTimeout(_updateTimeout),
	m_reconnectTimeout(_reconnectTimeout),
	m_limit(_limit),
	m_timeout(_timeout),
	m_lastUpdateId(0),
	m_lastFileId(0)
{}

void TelegramBot::Run()
{
	while (true)
	{
		auto result = Update();
		if (!result)
		{
			dev::ThreadSleep(m_reconnectTimeout);
		}
		dev::ThreadSleep(m_updateTimeout);
	}
}

bool TelegramBot::Update()
{
	httplib::Client cli(m_domain);
	cli.enable_server_certificate_verification(false);
	httplib::Params params;

	params.emplace("limit", std::to_string(m_limit));
	if (m_timeout > 0)
	{
		params.emplace("timeout", std::to_string(m_timeout));
		cli.set_read_timeout(m_timeout + 1, 0);
	}
	if (m_lastUpdateId > 0)
	{
		params.emplace("offset", std::to_string(m_lastUpdateId + 1));
	}

	auto res = cli.Post(("/bot" + m_token + "/getUpdates").c_str(), params);
	if (!res)
	{
		dev::Log("Error! Can't get updates.");
		dev::Log("HTTPLIB Error code : {}.", static_cast<int>(res.error()));
		return false;
	}

	if (res->status != STATUS_OK)
	{
		dev::Log("Error! Can't get updates. HTTP status code: {}.", res->status);
		return false;
	}
	
	auto bodyJ = nlohmann::json::parse(res->body);

	if (bodyJ["ok"] == false || bodyJ["result"].empty()) return false;

	for (const auto& updateJ : bodyJ["result"])
	{
		m_lastUpdateId = updateJ["update_id"];

		if (updateJ.contains("message") == false) return true;

		auto messageJ = updateJ["message"];
		Incoming messageData{ 
			.chat_id = messageJ["chat"]["id"], 
			.message_id = messageJ["message_id"],
			.userId = messageJ["from"]["id"]
		};

		if (messageJ.contains("text"))
			messageData.text = messageJ["text"];

		if (messageJ.contains("entities"))
		{
			for (const auto& entityJ : messageJ["entities"])
			{
				messageData.entities.emplace_back
				(
					messageData.text.substr( entityJ["offset"], entityJ["length"]) 
				);
			}
		}

		dev::Log("Message from {}, chatId: {}, msg: {}",
			messageJ["from"]["first_name"].get<std::string>(),
			messageData.chat_id,
			messageData.text);

		//std::thread msg(&TelegramBot::MessageHandler, this, messageData);
		//msg.detach();
	}
	return true;
}

void TelegramBot::SendMsg(
	const int64_t _chatId, 
	const std::string& _msg,
	const unsigned int _replyToMessageId)
{
	if (_msg.empty()) return;
	Content message;
	message.parse_mode = "HTML";
	message.text = _msg;

	bool rkeyboard = false;

	dev::Log("Sending a message to chatId: {}", _chatId);
	httplib::Client cli(m_domain);
	cli.enable_server_certificate_verification(false);

	httplib::Params params;
	params.emplace("parse_mode", message.parse_mode);
	params.emplace("chat_id", std::to_string(_chatId));
	params.emplace("text", message.text);

	if (_replyToMessageId != 0)
	{
		params.emplace("reply_tomessage_id", std::to_string(_replyToMessageId));
	}
	if (!message.reply_keyboard.keyboard.empty() && !rkeyboard)
	{
		nlohmann::json keyboard;
		unsigned int i = 0, j;
		for (const auto& element : message.reply_keyboard.keyboard)
		{
			j = 0;
			for (const auto& e : element)
			{
				if (!e.text.empty())
				{
					keyboard["keyboard"][i][j]["text"] = e.text;
				}
				if (e.request_contact == true)
				{
					keyboard["keyboard"][i][j]["request_contact"] = true;
				}
				if (e.request_location == true)
				{
					keyboard["keyboard"][i][j]["request_location"] = true;
				}
				j++;
			}
			i++;
		}
		if (message.reply_keyboard.resize_keyboard == true)
		{
			keyboard["resize_keyboard"] = true;
		}
		if (message.reply_keyboard.one_time_keyboard == true)
		{
			keyboard["one_time_keyboard"] = true;
		}
		if (message.reply_keyboard.selective == true)
		{
			keyboard["selective"] = true;
		}

		std::string serialized = keyboard.dump();
		params.emplace("reply_markup", serialized);

		rkeyboard = true;
	}
	if (message.hide_reply_keyboard.hide == true && !rkeyboard)
	{
		nlohmann::json json;
		json["hide_keyboard"] = true;
		if (message.hide_reply_keyboard.selective == true)
		{
			json["selective"] = true;
		}

		std::string serialized = json.dump();
		params.emplace("reply_markup", serialized);
	}

	for (size_t i = 0; i < MAX_SEND_ATTEMPTS; i++, dev::ThreadSleep(RESEND_MESAGE_DELAY))
	{
		auto res = cli.Post(("/bot" + m_token + "/sendMessage").c_str(), params);
		if (!res)
		{
			dev::Log("Error! Can't send a text message to chatId: {}", _chatId);
			dev::Log("HTTPLIB Error code: {}.", static_cast<int>(res.error()));
		}
		else 
		if (res->status != STATUS_OK)
		{
			dev::Log("Error! Can't send a text message to chatId: {}. HTTP status code: {}.", _chatId, res->status);
		}
		else
		{
			dev::Log("Successfully sent.");
			break;
		}
		dev::Log("Resend message. An attempt: {} of {}", i, MAX_SEND_ATTEMPTS);
	}
}

void TelegramBot::SendMsgs(
	const int64_t _chatId, 
	const std::string& _prefix, 
	const std::vector<std::string>& _msgs)
{
	if (_msgs.empty()) return;
	SendMsg(_chatId, _prefix);

	for (const auto& msg : _msgs)
	{
		SendMsg(_chatId, msg);
	}
}