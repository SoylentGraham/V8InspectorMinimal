#include "TV8MessageChannel.h"
#include <SoyFileSystem.h>

#if defined(ENABLE_WEBSOCKET)
TWebsocketMessageChannel::TWebsocketMessageChannel(uint16_t ListenPort,std::function<void(const std::string&)> OnMessage) :
	TV8MessageChannel	( OnMessage )
{
	auto OnTextMessage = [this](SoyRef Connection,const std::string& Message)
	{
		mWebsocketClient = Connection;
		this->OnMessage( Message );
	};
	auto OnBinaryMessage = [](SoyRef Connection,const Array<uint8_t>& Message)
	{
		std::Debug << "Got binary messaage from websocket x" << Message.GetSize() << std::endl;
	};
	
	auto InspectorPorts = {ListenPort, static_cast<uint16_t>(0) };
	for ( auto InspectorPort : InspectorPorts )
	{
		try
		{
			mWebsocketServer.reset( new TWebsocketServer(InspectorPort,OnTextMessage,OnBinaryMessage) );
			break;
		}
		catch(std::exception& e)
		{
			std::Debug << e.what() << std::endl;
		}
	}
	if ( !mWebsocketServer )
		throw Soy::AssertException("Failed to open websocket server");
	
	
	auto GetWebsocketAddress = [&]
	{
		std::string WebsocketAddress;
		auto EnumSocketAddress = [&](const std::string& InterfaceName,SoySockAddr& Address)
		{
			if ( !WebsocketAddress.empty() )
				return;
			std::stringstream WebsocketAddressStr;
			WebsocketAddressStr << Address;
			WebsocketAddress = WebsocketAddressStr.str();
		};
		auto& Socket = mWebsocketServer->GetSocket();
		Socket.GetSocketAddresses(EnumSocketAddress);
		
		return WebsocketAddress;
	};
	
	auto GetChromeDebuggerUrl = [&]
	{
		//	lets skip the auto stuff for now!
		//	chrome-devtools://devtools/bundled/inspector.html?experiments=true&v8only=true&ws=127.0.0.1:14549
		auto WebsocketAddress = GetWebsocketAddress();
		std::stringstream ChromeUrl;
		ChromeUrl << "chrome-devtools://devtools/bundled/inspector.html?experiments=true&v8only=true&ws=" << WebsocketAddress;
		return ChromeUrl.str();
	};
	
	std::stringstream OpenUrl;
	OpenUrl << "http://" << GetChromeDebuggerUrl();
	auto str = OpenUrl.str();
	::Platform::ShellOpenUrl(str);
	std::Debug << "Open chrome inspector: " << GetChromeDebuggerUrl() << std::endl;
	
}
#endif

TPredefinedMessageChannel::TPredefinedMessageChannel(const std::vector<std::string>& Messages,std::function<void(const std::string&)> OnMessage) :
	TV8MessageChannel	( OnMessage )
{
	for ( auto& Message : Messages )
	{
		OnMessage( Message );
	}
}
