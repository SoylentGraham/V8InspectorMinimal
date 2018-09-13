#include "SoyV8.h"
#include "TV8Inspector.h"

#include <SoyDebug.h>
#include <SoyFilesystem.h>


//	normally I hate using namespace;'s...
using namespace v8;


bool ReportDefinedReturns = false;




V8Exception::V8Exception(v8::TryCatch& TryCatch,v8::Isolate& Isolate,const std::string& Context) :
	mError	( Context )
{
	//	get the exception from v8
	auto Exception = TryCatch.Exception();

	if ( Exception.IsEmpty() )
	{
		mError += "<Empty Exception>";
		return;
	}

	//	get the description
	String::Utf8Value ExceptionStr( &Isolate, Exception);
	auto ExceptionCStr = *ExceptionStr;
	if ( ExceptionCStr == nullptr )
	{
		mError += ": <null> possibly not an exception";
	}
	else
	{
		mError += ": ";
		mError += ExceptionCStr;
	}
	
	//	get stack trace
	auto StackTrace = v8::Exception::GetStackTrace( Exception );
	if ( StackTrace.IsEmpty() )
	{
		mError += "\n<missing stacktrace>";
	}
	else
	{
		for ( int fi=0;	fi<StackTrace->GetFrameCount();	fi++ )
		{
			auto Frame = StackTrace->GetFrame( &Isolate, fi );
			String::Utf8Value FuncName( &Isolate, Frame->GetFunctionName() );
			mError += "\n";
			mError += "in ";
			mError += *FuncName;
		}
	}
	

}


void* TV8Allocator::Allocate(size_t length)
{
	auto* Bytes = static_cast<uint8_t*>( AllocateUninitialized(length) );
	
	for ( auto i=0;	i<length;	i++ )
		Bytes[i] = 0;
	
	return Bytes;
}

void* TV8Allocator::AllocateUninitialized(size_t length)
{
	return mHeap.AllocRaw(length);
}

void TV8Allocator::Free(void* data, size_t length)
{
	mHeap.FreeRaw(data, length);
}




Local<Function> v8::GetFunction(Local<Context> ContextHandle,Local<Object> This,const std::string& FunctionName)
{
	auto* Isolate = ContextHandle->GetIsolate();
	auto FuncNameKey = v8::GetString( *Isolate, FunctionName );
	
	//  get the global object for this name
	auto FunctionHandle = This->Get( ContextHandle, FuncNameKey).ToLocalChecked();
	
	auto Func = v8::SafeCast<v8::Function>( FunctionHandle );
	return Func;
}





std::string v8::GetTypeName(v8::Local<v8::Value> Handle)
{
	if ( Handle->IsUndefined() )	return "Undefined";
	if ( Handle->IsNull() )			return "Null";
	if ( Handle->IsFunction() )		return "Function";
	if ( Handle->IsString())		return "String";
	if ( Handle->IsMap() )			return "Map";
	if ( Handle->IsDate())			return "Date";
	if ( Handle->IsArray() )		return "Array";
	if ( Handle->IsBoolean() )		return "Boolean";
	if ( Handle->IsNumber() )		return "Number";
	if ( Handle->IsPromise() )		return "Promise";
	if ( Handle->IsFloat32Array() )	return "Float32Array";
	if ( Handle->IsFloat64Array() )	return "Float64Array";
	if ( Handle->IsUint8ClampedArray() )	return "Uint8ClampedArray";
	if ( Handle->IsInt8Array() )	return "Int8Array";
	if ( Handle->IsInt16Array() )	return "Int16Array";
	if ( Handle->IsInt32Array() )	return "Int32Array";
	if ( Handle->IsUint8Array() )	return "Uint8Array";
	if ( Handle->IsUint16Array() )	return "Uint16Array";
	if ( Handle->IsUint32Array() )	return "Uint32Array";
	
	if ( Handle->IsObject() )		return "Object";

	return "Unknown type";
}


//	uint8_t -> uint8clampedarray
Local<v8::Value> v8::GetTypedArray(v8::Isolate& Isolate,ArrayBridge<uint8_t>&& Values)
{
	std::stringstream TimerName;
	TimerName << "v8::GetTypedArray( " << Values.GetSize() << " )";
	Soy::TScopeTimerPrint Timer(TimerName.str().c_str(), 10 );
	
	auto Size = Values.GetDataSize();
	auto Rgba8Buffer = v8::ArrayBuffer::New( &Isolate, Size );
	auto Rgba8BufferContents = Rgba8Buffer->GetContents();
	auto Rgba8DataArray = GetRemoteArray( static_cast<uint8_t*>( Rgba8BufferContents.Data() ), Rgba8BufferContents.ByteLength() );
	Rgba8DataArray.Copy( Values );
	
	auto Rgba8 = v8::Uint8ClampedArray::New( Rgba8Buffer, 0, Rgba8Buffer->ByteLength() );
	return Rgba8;
}


//	uint8_t -> uint8clampedarray
void v8::CopyToTypedArray(v8::Isolate& Isolate,ArrayBridge<uint8_t>&& Values,Local<v8::Value> ArrayHandle)
{
	std::stringstream TimerName;
	TimerName << "v8::CopyToTypedArray( " << Values.GetSize() << " )";
	Soy::TScopeTimerPrint Timer(TimerName.str().c_str(), 10 );

	auto u8ArrayHandle = v8::SafeCast<v8::Uint8ClampedArray>( ArrayHandle );
	auto Buffer = u8ArrayHandle->Buffer();
	auto BufferContents = Buffer->GetContents();
	auto u8DataArray = GetRemoteArray( static_cast<uint8_t*>( BufferContents.Data() ), BufferContents.ByteLength() );
	u8DataArray.Copy( Values );
	/*
	auto Size = Values.GetDataSize();
	auto Rgba8Buffer = v8::ArrayBuffer::New( &Isolate, Size );
	auto Rgba8BufferContents = Rgba8Buffer->GetContents();
	auto Rgba8DataArray = GetRemoteArray( static_cast<uint8_t*>( Rgba8BufferContents.Data() ), Rgba8BufferContents.ByteLength() );
	Rgba8DataArray.Copy( Values );
	auto Rgba8 = v8::Uint8ClampedArray::New( Rgba8Buffer, 0, Rgba8Buffer->ByteLength() );
	return Rgba8;
	 */
}


void v8::EnumArray(Local<Array> ArrayHandle,std::function<void(size_t,Local<Value>)> EnumElement)
{
	for ( auto i=0;	i<ArrayHandle->Length();	i++ )
	{
		auto ElementHandle = ArrayHandle->Get(i);
		EnumElement( i, ElementHandle );
	}
}


void v8::EnumArray(Local<Value> ValueHandle,ArrayBridge<float>&& FloatArray,const std::string& Context)
{
	EnumArray( ValueHandle, FloatArray, Context );
}

void v8::EnumArray(Local<Value> ValueHandle,ArrayBridge<int>&& IntArray,const std::string& Context)
{
	EnumArray( ValueHandle, IntArray, Context );
}




void v8::EnumArray(v8::Local<v8::Value> ValueHandle,ArrayBridge<float>& FloatArray,const std::string& Context)
{
	if ( ValueHandle->IsNumber() )
	{
		auto ValueFloat = Local<Number>::Cast( ValueHandle );
		FloatArray.PushBack( ValueFloat->Value() );
	}
	else if ( ValueHandle->IsArray() )
	{
		//	we recursively expand arrays
		//	really we should only allow one level deep and check against the uniform (to allow arrays of vec4)
		auto ValueArray = Local<v8::Array>::Cast( ValueHandle );
		for ( auto i=0;	i<ValueArray->Length();	i++ )
		{
			auto ElementHandle = ValueArray->Get(i);
			EnumArray( ElementHandle, FloatArray, Context );
		}
	}
	else if ( ValueHandle->IsFloat32Array() )
	{
		EnumArray<Float32Array>( ValueHandle, GetArrayBridge(FloatArray) );
	}
	else
	{
		std::stringstream Error;
		Error << "Unhandled element type(" << v8::GetTypeName(ValueHandle) << ") in EnumArray<float>. Context: " << Context;
		throw Soy::AssertException(Error.str());
	}
}


void v8::EnumArray(v8::Local<v8::Value> ValueHandle,ArrayBridge<int>& IntArray,const std::string& Context)
{
	if ( ValueHandle->IsNumber() )
	{
		auto ValueFloat = Local<Number>::Cast( ValueHandle );
		IntArray.PushBack( ValueFloat->Value() );
	}
	else if ( ValueHandle->IsArray() )
	{
		//	we recursively expand arrays
		//	really we should only allow one level deep and check against the uniform (to allow arrays of vec4)
		auto ValueArray = Local<v8::Array>::Cast( ValueHandle );
		for ( auto i=0;	i<ValueArray->Length();	i++ )
		{
			auto ElementHandle = ValueArray->Get(i);
			EnumArray( ElementHandle, IntArray, Context );
		}
	}
	else if ( ValueHandle->IsInt32Array() )
	{
		::Array<int32_t> Ints;
		EnumArray<Int32Array>( ValueHandle, GetArrayBridge(Ints) );
		IntArray.PushBackArray(Ints);
	}
	else if ( ValueHandle->IsUint32Array() )
	{
		::Array<uint32_t> Ints;
		EnumArray<Uint32Array>( ValueHandle, GetArrayBridge(Ints) );
		IntArray.PushBackArray(Ints);
	}
	else if ( ValueHandle->IsInt16Array() )
	{
		::Array<int16_t> Ints;
		EnumArray<Int16Array>( ValueHandle, GetArrayBridge(Ints) );
		IntArray.PushBackArray(Ints);
	}
	else if ( ValueHandle->IsUint16Array() )
	{
		::Array<uint16_t> Ints;
		EnumArray<Uint16Array>( ValueHandle, GetArrayBridge(Ints) );
		IntArray.PushBackArray(Ints);
	}
	else if ( ValueHandle->IsInt8Array() )
	{
		::Array<int8_t> Ints;
		EnumArray<Int8Array>( ValueHandle, GetArrayBridge(Ints) );
		IntArray.PushBackArray(Ints);
	}
	else if ( ValueHandle->IsUint8Array() )
	{
		::Array<uint8_t> Ints;
		EnumArray<Uint8Array>( ValueHandle, GetArrayBridge(Ints) );
		IntArray.PushBackArray(Ints);
	}
	else if ( ValueHandle->IsUint8ClampedArray() )
	{
		::Array<uint8_t> Ints;
		EnumArray<Uint8ClampedArray>( ValueHandle, GetArrayBridge(Ints) );
		IntArray.PushBackArray(Ints);
	}
	else
	{
		std::stringstream Error;
		Error << "Unhandled element type(" << v8::GetTypeName(ValueHandle) << ") in EnumArray<int>. Context: " << Context;
		throw Soy::AssertException(Error.str());
	}
}



std::string v8::EnumString(v8::Isolate& Isolate,Local<Value> StringHandle)
{
	//	do we want the string representation, or only if it IS a string?
	auto StringValue = v8::SafeCast<String>(StringHandle);
	String::Utf8Value ExceptionStr( &Isolate, StringValue );
	
	const auto* ExceptionCStr = *ExceptionStr;
	if ( ExceptionCStr == nullptr )
		ExceptionCStr = "<null>";
	
	std::string NewStr( ExceptionCStr );
	return NewStr;
}


Local<Value> v8::GetString(v8::Isolate& Isolate,const std::string& Str)
{
	auto* CStr = Str.c_str();
	return GetString(Isolate,CStr);
}

Local<Value> v8::GetString(v8::Isolate& Isolate,const char* CStr)
{
	if ( CStr == nullptr )
		CStr = "";

	auto StringHandle = String::NewFromUtf8( &Isolate, CStr );
	auto StringValue = Local<Value>::Cast( StringHandle );
	return StringValue;
}


v8::Local<v8::Array> v8::GetArray(v8::Isolate& Isolate,size_t ElementCount,std::function<Local<Value>(size_t)> GetElement)
{
	auto ArrayHandle = Array::New( &Isolate );
	for ( auto i=0;	i<ElementCount;	i++ )
	{
		auto ValueHandle = GetElement(i);
		ArrayHandle->Set( i, ValueHandle );
	}
	return ArrayHandle;
}
