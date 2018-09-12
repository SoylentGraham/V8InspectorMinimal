#include "TV8Inspector.h"
#include <SoyJson.h>
#include <SoyFilesystem.h>

using namespace v8_inspector;



std::string v8::GetString(const v8_inspector::StringView& String)
{
	std::stringstream StringOut;
	auto* Chars = String.characters8();
	auto CharStep = String.is8Bit() ? 1 : 2;
	for ( int i=0;	i<String.length();	i++ )
	{
		char c = Chars[ i * CharStep ];
		StringOut << c;
	}
	return StringOut.str();
}

std::string v8::GetString(v8_inspector::StringBuffer& String)
{
	return GetString( String.string() );
}


void TV8Channel::sendResponse(int callId,std::unique_ptr<StringBuffer> message)
{
	/*
	 Chrome tools message: {"id":1,"method":"Network.enable","params":{"maxPostDataSize":65536}}
	 Chrome tools message: {"id":2,"method":"Page.enable"}
	 Chrome tools message: {"id":3,"method":"Page.getResourceTree"}
	 Chrome tools message: {"id":4,"method":"Profiler.enable"}
	 Chrome tools message: {"id":5,"method":"Runtime.enable"}
	 Chrome tools message: {"id":6,"method":"Debugger.enable"}
	 Chrome tools message: {"id":7,"method":"Debugger.setPauseOnExceptions","params":{"state":"none"}}
	 Chrome tools message: {"id":8,"method":"Debugger.setAsyncCallStackDepth","params":{"maxDepth":32}}
	 */
	auto MessageStdString = v8::GetString(*message);
	std::Debug << "Channel response: " << MessageStdString << std::endl;
	
	mOnResponse( MessageStdString );
	/*
	//	from inspector-test.cc and taskrunner...
	//	send this to the context as a job
	auto MessageString = message->string();
	auto MessageStringData = GetRemoteArray( MessageString.characters16(), MessageString.length() );
	Array<uint16_t> Message16;
	Message16.Copy(MessageStringData);
	
	auto Job = [=](v8::Local<v8::Context> Context)
	{
		auto* Isolate = Context->GetIsolate();
		auto MessageStringv8 = v8::String::NewFromTwoByte( Isolate, Message16.GetArray(), v8::NewStringType::kNormal, Message16.GetSize() );
		//.ToLocalChecked();
		
		auto This = Context->Global();
		//result = channel_->function_.Get(data->isolate())->Call(context, This, 1, &MessageStringv8 );
	};
	mContainer.QueueScoped(Job);
	 */
/*
	v8::MicrotasksScope microtasks_scope(data->isolate(),
										 v8::MicrotasksScope::kRunMicrotasks);
	v8::HandleScope handle_scope(data->isolate());
	v8::Local<v8::Context> context =
	data->GetContext(channel_->context_group_id_);
	v8::Context::Scope context_scope(context);
	v8::Local<v8::Value> message = ToV8String(data->isolate(), message_);
	v8::MaybeLocal<v8::Value> result;
	result = channel_->function_.Get(data->isolate())
	->Call(context, context->Global(), 1, &message);
	allId,
	std::unique_ptr<v8_inspector::StringBuffer> message) override {
		task_runner_->Append(
							 new SendMessageTask(this, ToVector(message->string())));
*/
}

void TV8Channel::sendNotification(std::unique_ptr<StringBuffer> message)
{
	sendResponse( -1, std::move(message) );
}

void TV8Channel::flushProtocolNotifications()
{
	std::Debug << "flushProtocolNotifications" << std::endl;
	/*
	 f (disposed_)
	 return;
	 for (size_t i = 0; i < agents_.size(); i++)
	 agents_[i]->FlushPendingProtocolNotifications();
	 if (!notification_queue_.size())
	 return;
	 v8_session_state_json_.Set(ToCoreString(v8_session_->stateJSON()));
	 for (size_t i = 0; i < notification_queue_.size(); ++i) {
	 client_->SendProtocolNotification(session_id_,
	 notification_queue_[i]->Serialize(),
	 session_state_.TakeUpdates());
	 }
	 notification_queue_.clear();
	 */
}


