#pragma once

#include "v8-inspector.h"
#include "TApiWebsocket.h"




using namespace v8_inspector;

namespace v8
{
	std::string GetString(const v8_inspector::StringView& String);
	std::string GetString(v8_inspector::StringBuffer& String);
}

class TV8StringViewContainer
{
public:
	TV8StringViewContainer(const std::string& Text) :
		mString	( Text )
	{
	}
	
	StringView	GetStringView()
	{
		auto* CStr8 = reinterpret_cast<const uint8_t*>(mString.c_str());
		return StringView( CStr8, mString.length() );
	}
	
	std::string		mString;
	StringView	mView;
};


class TV8Channel : public V8Inspector::Channel
{
public:
	TV8Channel(std::function<void(const std::string&)> OnResponse) :
		mOnResponse	( OnResponse )
	{
	}
	
	virtual void sendResponse(int callId,std::unique_ptr<StringBuffer> message) override;
	virtual void sendNotification(std::unique_ptr<StringBuffer> message) override;
	virtual void flushProtocolNotifications() override;
	
	std::function<void(const std::string&)>	mOnResponse;
};


