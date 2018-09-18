#include "v8Minimal.h"
#include "SoyV8.h"
#include <SoyFilesystem.h>

#include "TV8Inspector.h"
#include "TV8Platform.h"
#include "TV8MessageChannel.h"
#include "include/v8-inspector.h"

using namespace v8;
using namespace v8_inspector;


std::shared_ptr<v8::Platform>	gPlatform;
std::shared_ptr<V8Inspector>	gInspector;

class InspectorClientImpl : public v8_inspector::V8InspectorClient
{
public:
	InspectorClientImpl(Local<v8::Context> Context,std::function<void()> RunMessageLoop,std::function<void()> StopMessageLoop) :
		mContext			( Context ),
		mRunMessageLoop		( RunMessageLoop ),
		mStopMessageLoop	( StopMessageLoop )
	{
	}
	virtual ~InspectorClientImpl()
	{
	}
	
private:
	
	virtual v8::Local<v8::Context> ensureDefaultContextInGroup(int context_group_id) override
	{
		return mContext;
	}
	/*
	virtual double currentTimeMS() override
	{
		return 1.0;
	}*/
	virtual void runMessageLoopOnPause(int context_group_id) override
	{
		mRunMessageLoop();
	}
	virtual void quitMessageLoopOnPause() override
	{
		mStopMessageLoop();
	}
/*
	std::unique_ptr<v8_inspector::V8Inspector> inspector_;
	std::unique_ptr<v8_inspector::V8InspectorSession> session_;
	std::unique_ptr<v8_inspector::V8Inspector::Channel> channel_;
*/
	//v8::Isolate* isolate_;
	//v8::Global<v8::Context> context_;
	Local<v8::Context>		mContext;
	std::function<void()>	mRunMessageLoop;
	std::function<void()>	mStopMessageLoop;

	//DISALLOW_COPY_AND_ASSIGN(InspectorClientImpl);
};

auto TestScriptFilename = "SourceFile.js";
auto TestScript = R"HELLOMUM(

function Log(str)
{
	console.log(str);
}

Log('Hello!');
//throw 'xxx';

)HELLOMUM";

void RunHelloWorld(Local<Context> context)
{
	auto* isolate = context->GetIsolate();
	v8::HandleScope handle_scope(isolate);

	// Create a string containing the JavaScript source code.
	v8::Local<v8::String> source =
	v8::String::NewFromUtf8(isolate, TestScript,v8::NewStringType::kNormal).ToLocalChecked();
		
	// Compile the source code.
	auto OriginStr = v8::GetString(*isolate, TestScriptFilename );
	auto OriginRow = v8::Integer::New( isolate, 0 );
	auto OriginCol = v8::Integer::New( isolate, 0 );
	auto Cors = v8::Boolean::New( isolate, true );
	static int ScriptIdCounter = 99;
	auto ScriptId = v8::Integer::New( isolate, ScriptIdCounter++ );
	auto OriginUrl = v8::GetString(*isolate, std::string("file://")+TestScriptFilename );
	
	static std::shared_ptr<v8::ScriptOrigin> HelloWorldOrigin;
	HelloWorldOrigin.reset( new v8::ScriptOrigin( OriginStr, OriginRow, OriginCol, Cors, ScriptId, OriginUrl ) );
	auto* pOrigin = HelloWorldOrigin.get();
/*
	v8::ScriptOrigin HelloWorldOrigin(
							v8::String::NewFromUtf8(isolate, "file://abcd.js"),
							v8::Integer::New(isolate, 0),
							v8::Integer::New(isolate, 0)
							);
	auto* pOrigin = &HelloWorldOrigin;
 */
	//v8::ScriptOrigin* pOrigin = nullptr;
	static v8::Local<v8::Script> script =
	v8::Script::Compile(context, source, pOrigin ).ToLocalChecked();
		
	// Run the script to get the result.
	auto result = script->Run(context);
	/*
	//v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();
	
	// Convert the result to an UTF8 string and print it.
	v8::String::Utf8Value utf8(isolate, result);
	printf("%s\n", *utf8);
	 */
}

void v8min_main_helloworld(v8::Isolate* isolate,const std::string& mRootDirectory)
{
	
	// Create a new context.
	/*
	v8::Local<v8::ObjectTemplate> global_template = v8::ObjectTemplate::New(isolate);
	
	v8::Local<v8::Context> context = v8::Context::New(isolate, nullptr, global_template);
	Local<ObjectTemplate> gl = ObjectTemplate::New(isolate);
	global_template->Set(String::NewFromUtf8(isolate, "gl"), gl);
	
*/
	v8::HandleScope handle_scope(isolate);
	v8::Local<v8::Context> context = v8::Context::New(isolate);

	
	


	Array<std::string> Messages;
	std::mutex MessagesLock;
	auto OnMessage = [&](const std::string& Message)
	{
		MessagesLock.lock();
		Messages.PushBack(Message);
		MessagesLock.unlock();
	};
	
		
	std::shared_ptr<V8InspectorClient>	mInspectorClient;
	std::shared_ptr<V8Inspector::Channel>	mChannel;
	std::shared_ptr<V8InspectorSession>		mSession;
	
#if defined(ENABLE_WEBSOCKET)
	std::shared_ptr<TV8MessageChannel>	mMessageSource( new TWebsocketMessageChannel(8008,OnMessage) );
#else
	std::shared_ptr<TV8MessageChannel>	mMessageSource( new TPredefinedMessageChannel(PrebakedCommands,OnMessage) );
#endif
	
	
	bool RunOnlyMessagesInLoop = false;
	
	auto RunMessageLoop = [&]()
	{
		std::Debug << "RunMessageLoop" << std::endl;
		RunOnlyMessagesInLoop = true;
	};
	auto QuitMessageLoop = [&]()
	{
		std::Debug << "QuitMessageLoop" << std::endl;
		RunOnlyMessagesInLoop = false;
	};

	auto DoSendResponse = [&](const std::string& Message)
	{
		mMessageSource->SendResponse(Message);
	};
	
	{
		// Enter the context for compiling and running the hello world script.
		v8::Context::Scope context_scope(context);
	
		auto ContextGroupId = 1;
		mInspectorClient.reset( new InspectorClientImpl(context,RunMessageLoop,QuitMessageLoop) );
		gInspector = V8Inspector::create( isolate, mInspectorClient.get() );
		TV8StringViewContainer ContextName("TheContextName");
		V8ContextInfo ContextInfo( context, ContextGroupId, ContextName.GetStringView() );
		gInspector->contextCreated(ContextInfo);

		// create a v8 channel.
		// ChannelImpl : public v8_inspector::V8Inspector::Channel
		mChannel.reset( new TV8Channel(DoSendResponse) );
		
		TV8StringViewContainer State("{}");
		
		// Create a debugging session by connecting the V8Inspector
		// instance to the channel
		mSession = gInspector->connect( ContextGroupId, mChannel.get(), State.GetStringView() );
	}
	
	
	
	auto* Session = mSession.get();
	auto SendMessageToSession = [=](const std::string& Message)
	{
		Array<uint8_t> MessageBuffer;
		std::Debug << "Message=" << Message << std::endl;
		Soy::StringToArray( Message, GetArrayBridge(MessageBuffer) );
		v8_inspector::StringView MessageString( MessageBuffer.GetArray(), MessageBuffer.GetSize() );
		Session->dispatchProtocolMessage(MessageString);
	};
	
	{
		v8::Context::Scope context_scope(context);
		RunHelloWorld(context);
	}
	
	while ( true )
	{
		if ( !RunOnlyMessagesInLoop )
		{
			while (v8::platform::PumpMessageLoop(gPlatform.get(), isolate))
			{
	
			}
		}
		
		//	process inspector messages
		{
			std::lock_guard<std::mutex> Lock( MessagesLock );
			while ( Messages.GetSize() > 0 )
			{
				// Enter the context for compiling and running the hello world script.
				v8::HandleScope handle_scope(isolate);
				v8::Context::Scope context_scope(context);

				auto Message = Messages.PopAt(0);
				SendMessageToSession(Message);
			}
		}
		
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	
}



void MessageHandler(v8::Local<v8::Message> message, v8::Local<v8::Value> exception) {
	
	v8::Isolate *isolate_ = v8::Isolate::GetCurrent();
	v8::Local<v8::Context> context = isolate_->GetEnteredContext();
	if (context.IsEmpty())
		return;
	v8_inspector::V8Inspector *inspector = gInspector.get();
	if ( inspector == nullptr )
		return;
	
	v8::Local<v8::StackTrace> stack = message->GetStackTrace();
	
	if ( !stack.IsEmpty() )
	{
		int StackSize = stack->GetFrameCount();
		std::Debug << "exception: stack size x" << StackSize << std::endl;
		for ( int i=0;	i<StackSize;	i++ )
		{
			auto Frame = stack->GetFrame(isolate_,i);
			auto ScriptName = v8::EnumString(*isolate_,Frame->GetScriptName());
			auto FuncName = v8::EnumString(*isolate_,Frame->GetFunctionName());
			std::Debug << "stack #" << i << " " << ScriptName << ", " << FuncName << "()" << std::endl;
		}
	}
	
	auto ScriptOrigin = message->GetScriptOrigin();
	
	/*
	int script_id = static_cast<int>(message->GetScriptOrigin().ScriptID()->Value());
	if (!stack.IsEmpty() && stack->GetFrameCount() > 0) {
		int top_script_id = stack->GetFrame(0)->GetScriptId();
		if (top_script_id == script_id) script_id = 0;
	}
	int line_number = message->GetLineNumber(context).FromMaybe(0);
	int column_number = 0;
	if (message->GetStartColumn(context).IsJust())
		column_number = message->GetStartColumn(context).FromJust() + 1;
	
	v8_inspector::StringView detailed_message;
	v8::internal::Vector<uint16_t> message_text_string = ToVector(message->Get());
	v8_inspector::StringView message_text(message_text_string.start(),
										  (size_t) message_text_string.length());
	v8::internal::Vector<uint16_t> url_string;
	if (message->GetScriptOrigin().ResourceName()->IsString()) {
		url_string = ToVector(message->GetScriptOrigin().ResourceName().As<v8::String>());
	}
	v8_inspector::StringView url(url_string.start(), (size_t) url_string.length());
	
	inspector->exceptionThrown(context, message_text, exception, detailed_message,
							   url, (unsigned int) line_number,
							   (unsigned int) column_number,
							   inspector->createStackTrace(stack), script_id);
*/
}




void v8min_main(const std::string& mRootDirectory)
{
	std::string IcuPath = mRootDirectory + "../v8Runtime/icudtl.dat";
	std::string NativesBlobPath = mRootDirectory + "../v8Runtime/natives_blob.bin";
	std::string SnapshotBlobPath = mRootDirectory + "../v8Runtime/snapshot_blob.bin";
	
	if ( !V8::InitializeICUDefaultLocation( nullptr, IcuPath.c_str() ) )
		throw Soy::AssertException("Failed to load ICU");
	V8::InitializeExternalStartupData( NativesBlobPath.c_str(), SnapshotBlobPath.c_str() );
	
	//std::unique_ptr<v8::Platform> platform = v8::platform::CreateDefaultPlatform();
	//gPlatform.reset( v8::platform::CreateDefaultPlatform() );
	gPlatform.reset( new TV8Platform() );
	V8::InitializePlatform( gPlatform.get() );
	V8::Initialize();
	
	// Create a new Isolate and make it the current one.
	v8::Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	v8::Isolate* isolate = v8::Isolate::New(create_params);
	isolate->AddMessageListener(MessageHandler);
	
	{
		v8::Locker locker(isolate);
		isolate->Enter();
		v8::Isolate::Scope isolate_scope(isolate);
		//v8::HandleScope handle_scope(isolate);

		v8min_main_helloworld( isolate, mRootDirectory );
	}
	
	// Dispose the isolate and tear down V8.
	isolate->Dispose();
	v8::V8::Dispose();
	v8::V8::ShutdownPlatform();
	delete create_params.array_buffer_allocator;
}

#if defined(TARGET_WINDOWS)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc,const char* argv[])
#endif
{
	auto RootDir = ::Platform::GetAppResourcesDirectory();
	//RootDir += "Data_dlib/";
	//RootDir += "Data_Posenet/";
	RootDir += "Data_HelloWorld/";
	v8min_main(RootDir);
}

