#pragma once

//	no websocket, just some pre made commands captured from chrome dev tools
//#define USE_PREBAKED_COMMANDS

#if !defined(USE_PREBAKED_COMMANDS)
#define ENABLE_WEBSOCKET
#endif

#if defined(USE_PREBAKED_COMMANDS)
std::vector<std::string> PrebakedCommands
{
	"Message={\"id\":1,\"method\":\"Profiler.enable\"}",
	"Message={\"id\":2,\"method\":\"Runtime.enable\"}"
	/*
	 Message={"id":1,"method":"Profiler.enable"}
	 Message={"id":2,"method":"Runtime.enable"}
	 Message={"id":3,"method":"Debugger.enable"}
	 Message={"id":4,"method":"Debugger.setPauseOnExceptions","params":{"state":"all"}}
	 Message={"id":5,"method":"Debugger.setAsyncCallStackDepth","params":{"maxDepth":32}}
	 Message={"id":6,"method":"Debugger.setBlackboxPatterns","params":{"patterns":["/main\\.js\\b"]}}
	 Message={"id":7,"method":"Runtime.runIfWaitingForDebugger"}
	 Message={"id":8,"method":"Debugger.setSkipAllPauses","params":{"skip":false}}
	 Message={"id":9,"method":"Debugger.pause"}
	 Message={"id":10,"method":"Runtime.evaluate","params":{"expression":"this","objectGroup":"completion","includeCommandLineAPI":true,"silent":true,"contextId":1,"returnByValue":false,"generatePreview":false,"userGesture":false,"awaitPromise":false,"throwOnSideEffect":false}}
	 Message={"id":11,"method":"Runtime.evaluate","params":{"expression":"(async function(){ await 1; })()","contextId":1,"throwOnSideEffect":true}}
	 Message={"id":12,"method":"Runtime.evaluate","params":{"expression":"(Log)","objectGroup":"argumentsHint","includeCommandLineAPI":true,"silent":true,"contextId":1,"returnByValue":false,"generatePreview":false,"userGesture":false,"awaitPromise":false,"throwOnSideEffect":false}}
	 Message={"id":13,"method":"Runtime.compileScript","params":{"expression":"Log(\"he\");","sourceURL":"","persistScript":false,"executionContextId":1}}
	 Message={"id":14,"method":"Runtime.compileScript","params":{"expression":"Log(\"he\")","sourceURL":"","persistScript":false,"executionContextId":1}}
	 Message={"id":15,"method":"Runtime.compileScript","params":{"expression":"Log(\"he\")","sourceURL":"","persistScript":false,"executionContextId":1}}
	 Message={"id":16,"method":"Runtime.compileScript","params":{"expression":"Log(\"he\")","sourceURL":"","persistScript":false,"executionContextId":1}}
	 Message={"id":17,"method":"Debugger.getScriptSource","params":{"scriptId":"10"}}
	 Message={"id":18,"method":"Runtime.callFunctionOn","params":{"objectId":"{\"injectedScriptId\":1,\"id\":3}","functionDeclaration":"function getCompletions(type){let object;if(type==='string')\nobject=new String('');else if(type==='number')\nobject=new Number(0);else if(type==='bigint')\nobject=Object(BigInt(0));else if(type==='boolean')\nobject=new Boolean(false);else\nobject=this;const result=[];try{for(let o=object;o;o=Object.getPrototypeOf(o)){if((type==='array'||type==='typedarray')&&o===object&&o.length>9999)\ncontinue;const group={items:[],__proto__:null};try{if(typeof o==='object'&&o.constructor&&o.constructor.name)\ngroup.title=o.constructor.name;}catch(ee){}\nresult[result.length]=group;const names=Object.getOwnPropertyNames(o);const isArray=Array.isArray(o);for(let i=0;i<names.length&&group.items.length<10000;++i){if(isArray&&/^[0-9]/.test(names[i]))\ncontinue;group.items[group.items.length]=names[i];}}}catch(e){}\nreturn result;}","arguments":[{}],"silent":true,"returnByValue":true}}
	 Message={"id":19,"method":"Runtime.evaluate","params":{"expression":"D","includeCommandLineAPI":true,"contextId":1,"generatePreview":true,"userGesture":true,"awaitPromise":false,"throwOnSideEffect":true,"timeout":500}}
	 */
};
#else
#include "TApiWebsocket.h"
#endif



class TV8MessageChannel
{
public:
	TV8MessageChannel(std::function<void(const std::string&)> OnMessage) :
		mOnMessage	( OnMessage )
	{
	}
	virtual ~TV8MessageChannel()	{}
	
	virtual void	SendResponse(const std::string& Message)=0;
	
protected:
	void	OnMessage(const std::string& Message)
	{
		mOnMessage( Message );
	}
	
	std::function<void(const std::string&)>	mOnMessage;
};


#if defined(ENABLE_WEBSOCKET)
class TWebsocketMessageChannel : public TV8MessageChannel
{
public:
	TWebsocketMessageChannel(uint16_t ListenPort,std::function<void(const std::string&)> OnMessage);
	
	virtual void	SendResponse(const std::string& Message) override
	{
		mWebsocketServer->Send( mWebsocketClient, Message );
	}
	
	std::shared_ptr<TWebsocketServer>	mWebsocketServer;
	SoyRef								mWebsocketClient;	//	hack; record last client who we just send responses back to
};
#endif


class TPredefinedMessageChannel : public TV8MessageChannel
{
public:
	TPredefinedMessageChannel(const std::vector<std::string>& Messages,std::function<void(const std::string&)> OnMessage);
	
	virtual void	SendResponse(const std::string& Message) override
	{
	}
};

