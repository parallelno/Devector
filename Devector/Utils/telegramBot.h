#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <string>
#include <vector>

class TelegramBot
{
protected:
	static const constexpr int STATUS_OK = 200;
	static const constexpr double RESEND_MESAGE_DELAY = 1.0;
	static const constexpr size_t MAX_SEND_ATTEMPTS = 10;

	struct KeyboardButton
	{
		std::string text;
		bool request_contact;
		bool request_location;
	};

	using ReplyKeyboardRow = std::vector<KeyboardButton>;

	struct ReplyKeyboardMarkup
	{
		std::vector<ReplyKeyboardRow> keyboard;
		bool resize_keyboard;
		bool one_time_keyboard;
		bool selective;
	};

	struct ReplyKeyboardHide
	{
		bool hide;
		bool selective;
	};

	struct Incoming
	{
		int64_t chat_id;
		int64_t message_id;
		uint64_t userId;
		std::string text;
		std::vector<std::string> entities;
	};

	struct Content
	{
		std::string text;
		std::string parse_mode;
		ReplyKeyboardMarkup reply_keyboard;
		ReplyKeyboardHide hide_reply_keyboard;
	};

	std::string m_token;

	const double m_updateTimeout;
	const double m_reconnectTimeout;
	unsigned int m_limit;
	unsigned int m_timeout;
	unsigned int m_lastUpdateId;
	unsigned int m_lastFileId;

	std::string m_domain = "https://api.telegram.org";

	//virtual void MessageHandler(Incoming _data) = 0;
	void SendMsg(
		const int64_t _chatId,
		const std::string& _msg,
		const unsigned int _replyToMessageId = 0);

	void SendMsgs(
		const int64_t _chatId,
		const std::string& _prefix,
		const std::vector<std::string>& _msgs);
	
	bool Update();

public:
	TelegramBot(
		const std::string _token,
		const double _updateTimeout = 1.0,
		const double _reconnectTimeout = 10.0,
		const unsigned int _limit = 100,
		const unsigned int _timeout = 30
		);

	void Run();
};