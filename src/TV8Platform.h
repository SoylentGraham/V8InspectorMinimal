#pragma once
#include "libplatform/libplatform.h"


//	seems node AND chromium have the same issue I have
//	https://github.com/nodejs/node/commit/b1e26128f317a6f5a5808a0a727e98f80f088b84
//	https://cs.chromium.org/chromium/src/gin/v8_platform.cc?type=cs&q=CallDelayedOnWorkerThread&sq=package:chromium&g=0&l=368
//	so forced to implement their own platforms and task scheduling.
//	Ibon didnt.... but still, I can't seem to work around it
//	gr: as I cannot derive from the default platform (linker can't find typeinfo!)...
//		...I create a proxy to the default.
class TV8Platform : public v8::Platform
{
	using Isolate = v8::Isolate;
	using Task = v8::Task;
	using IdleTask = v8::IdleTask;
	using PageAllocator = v8::PageAllocator;
	using TaskRunner = v8::TaskRunner;
	using TracingController = v8::TracingController;
	
public:
	TV8Platform() :
		mpPlatform	( v8::platform::CreateDefaultPlatform() ),
		mPlatform	( *mpPlatform )
	{
		
	}
	
	virtual PageAllocator* GetPageAllocator() override {	return mPlatform.GetPageAllocator();	}
	virtual void OnCriticalMemoryPressure() override {	mPlatform.OnCriticalMemoryPressure();	}
	virtual bool OnCriticalMemoryPressure(size_t length) override {	return mPlatform.OnCriticalMemoryPressure(length);	}
	virtual int NumberOfWorkerThreads() override{	return mPlatform.NumberOfWorkerThreads();	}
	virtual std::shared_ptr<TaskRunner> GetForegroundTaskRunner(Isolate* isolate) override{	return mPlatform.GetForegroundTaskRunner(isolate);	}
	virtual void CallOnWorkerThread(std::unique_ptr<Task> task) override{	mPlatform.CallOnWorkerThread( std::move(task) );	}
	virtual void CallBlockingTaskOnWorkerThread(std::unique_ptr<Task> task) override	{	mPlatform.CallBlockingTaskOnWorkerThread( std::move(task) );	}
	//virtual void CallDelayedOnWorkerThread(std::unique_ptr<Task> task,double delay_in_seconds) override	{	mPlatform.CallDelayedOnWorkerThread( std::move(task),delay_in_seconds);	}
	virtual void CallOnForegroundThread(Isolate* isolate, Task* task) override	{	mPlatform.CallOnForegroundThread(isolate, task);	}
	virtual void CallDelayedOnForegroundThread(Isolate* isolate, Task* task, double delay_in_seconds) override	{	mPlatform.CallDelayedOnForegroundThread( isolate, task, delay_in_seconds );	}
	virtual void CallIdleOnForegroundThread(Isolate* isolate, IdleTask* task) override	{	mPlatform.CallIdleOnForegroundThread(isolate, task);	}
	virtual bool IdleTasksEnabled(Isolate* isolate) override	{	return mPlatform.IdleTasksEnabled(isolate);	}
	virtual double MonotonicallyIncreasingTime() override	{	return mPlatform.MonotonicallyIncreasingTime();	}
	virtual double CurrentClockTimeMillis() override	{	return mPlatform.CurrentClockTimeMillis();	}
	virtual StackTracePrinter GetStackTracePrinter() override	{	return mPlatform.GetStackTracePrinter();	}
	virtual TracingController* GetTracingController() override	{	return mPlatform.GetTracingController();	}
	
	//	here's the bodged fix to avoid the assert. Seems to work!
	virtual void CallDelayedOnWorkerThread(std::unique_ptr<Task> task,double delay_in_seconds) override
	{
		delay_in_seconds = 0;
		mPlatform.CallDelayedOnWorkerThread( std::move(task),delay_in_seconds);
	}
	
protected:
	std::shared_ptr<v8::Platform>	mpPlatform;
	v8::Platform&	mPlatform;
	

};



