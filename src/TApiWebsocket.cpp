#include "TApiWebsocket.h"
#include <SoySocket.h>

using namespace v8;

const char GetAddress_FunctionName[] = "GetAddress";
const char Send_FunctionName[] = "Send";
const char GetPeers_FunctionName[] = "GetPeers";



TWebsocketServer::TWebsocketServer(uint16_t ListenPort,std::function<void(SoyRef,const std::string&)> OnTextMessage,std::function<void(SoyRef,const Array<uint8_t>&)> OnBinaryMessage) :
	SoyWorkerThread		( Soy::StreamToString(std::stringstream()<<"WebsocketServer("<<ListenPort<<")"), SoyWorkerWaitMode::Sleep ),
	mOnTextMessage		( OnTextMessage ),
	mOnBinaryMessage	( OnBinaryMessage )
{
	mSocket.reset( new SoySocket() );
	mSocket->CreateTcp(true);
	mSocket->ListenTcp(ListenPort);
	
	mSocket->mOnConnect = [=](SoyRef ClientRef)
	{
		AddClient(ClientRef);
	};
	mSocket->mOnDisconnect = [=](SoyRef ClientRef)
	{
		RemoveClient(ClientRef);
	};
	
	Start();
	
}
	

bool TWebsocketServer::Iteration()
{
	if ( !mSocket )
		return false;
	
	if ( !mSocket->IsCreated() )
		return true;
	
	//	non blocking so lets just do everything in a loop
	auto NewClient = mSocket->WaitForClient();
	if ( NewClient.IsValid() )
		std::Debug << "new client connected: " << NewClient << std::endl;
	/*
	Array<char> RecvBuffer;
	auto RecvFromConnection = [&](SoyRef ClientRef,SoySocketConnection ClientCon)
	{
		RecvBuffer.Clear();
		ClientCon.Recieve( GetArrayBridge(RecvBuffer) );
		if ( !RecvBuffer.IsEmpty() )
		{
			auto Client = GetClient(ClientRef);
			Client->OnRecvData( GetArrayBridge(RecvBuffer) );
		}
	};
	mSocket->EnumConnections( RecvFromConnection );
	*/
	return true;
}


std::shared_ptr<TWebsocketServerPeer> TWebsocketServer::GetClient(SoyRef ClientRef)
{
	std::lock_guard<std::recursive_mutex> Lock(mClientsLock);
	for ( int c=0;	c<mClients.GetSize();	c++ )
	{
		auto& pClient = mClients[c];
		if ( pClient->mConnectionRef == ClientRef )
			return pClient;
	}

	throw Soy::AssertException("Client not found");
}

void TWebsocketServer::AddClient(SoyRef ClientRef)
{
	std::shared_ptr<TWebsocketServerPeer> Client( new TWebsocketServerPeer( mSocket, ClientRef, mOnTextMessage, mOnBinaryMessage ) );
	std::lock_guard<std::recursive_mutex> Lock(mClientsLock);
	mClients.PushBack(Client);
}

void TWebsocketServer::RemoveClient(SoyRef ClientRef)
{
	
}


void TWebsocketServerPeer::OnDataRecieved(std::shared_ptr<WebSocket::TRequestProtocol>& pData)
{
	auto& Data = *pData;
	
	//	this was the http request, send the reply
	if ( pData->mReplyMessage )
	{
		if ( pData->mHandshake.mHasSentAcceptReply )
			throw Soy::AssertException("Already sent handshake reply");
		/*
		auto& Message = std::Debug;
		Message << "http request " << Data.mHost << " " << Data.mUrl << std::endl;
		for ( auto Header : Data.mHeaders )
		{
			Message << Header.first << ": " << Header.second << std::endl;
		}
		Message << "Content size: " << Data.mContent.GetSize();
		for ( int i=0;	i<Data.mContent.GetSize();	i++ )
			Message << Data.mContent[i];
		Message << std::endl;
		
		*/
	
		std::Debug << "Sending handshake response to " << this->GetSocketAddress() << std::endl;
		pData->mHandshake.mHasSentAcceptReply = true;
		Push( pData->mReplyMessage );
		return;
	}
	
	//	packet with data!
	if ( pData->mMessage )
	{
		auto& Packet = *pData->mMessage;
		if ( Packet.IsCompleteTextMessage() )
		{
			mOnTextMessage( this->mConnectionRef, Packet.mTextData );
			//	gr: there's a bit of a disconnect here between Some-reponse-packet data and our persistent data
			//		should probbaly change this to like
			//	pData->PopTextMessage()
			//		and a call back to owner to clear. or a "alloc new data" thing for the response class
			std::lock_guard<std::recursive_mutex> Lock(mCurrentMessageLock);
			mCurrentMessage.reset();
		}
		if ( Packet.IsCompleteBinaryMessage() )
		{
			mOnBinaryMessage( this->mConnectionRef, Packet.mBinaryData );
			//	see above
			std::lock_guard<std::recursive_mutex> Lock(mCurrentMessageLock);
			mCurrentMessage.reset();
		}
	}
}


void TWebsocketServer::Send(SoyRef ClientRef,const std::string& Message)
{
	auto Peer = GetClient(ClientRef);
	if ( !Peer )
	{
		std::stringstream Error;
		Error << "No peer matching " << ClientRef;
		throw Soy::AssertException(Error.str());
	}
	Peer->Send(Message);
}


void TWebsocketServer::Send(SoyRef ClientRef,const ArrayBridge<uint8_t>& Message)
{
	auto Peer = GetClient(ClientRef);
	if ( !Peer )
	{
		std::stringstream Error;
		Error << "No peer matching " << ClientRef;
		throw Soy::AssertException(Error.str());
	}
	Peer->Send(Message);
}


std::shared_ptr<Soy::TReadProtocol> TWebsocketServerPeer::AllocProtocol()
{
	//	this needs revising... or locking at least
	std::lock_guard<std::recursive_mutex> Lock(mCurrentMessageLock);
	if ( !mCurrentMessage )
		mCurrentMessage.reset( new WebSocket::TMessageBuffer() );
	
	auto* NewProtocol = new WebSocket::TRequestProtocol(mHandshake,mCurrentMessage);
	return std::shared_ptr<Soy::TReadProtocol>( NewProtocol );
}


void TWebsocketServerPeer::Send(const std::string& Message)
{
	//	gr: cannot send if handshake hasn't completed
	if ( !this->mHandshake.IsCompleted() )
	{
		throw Soy::AssertException("Sending message before handshake complete");
	}
	std::shared_ptr<Soy::TWriteProtocol> Packet( new WebSocket::TMessageProtocol( this->mHandshake, Message ) );
	Push( Packet );
}

void TWebsocketServerPeer::Send(const ArrayBridge<uint8_t>& Message)
{
	throw Soy::AssertException("Encode to websocket protocol");
	
}

