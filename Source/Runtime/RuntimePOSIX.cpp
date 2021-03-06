#ifndef _WIN32

#include "Core/Core.h"
#include "RuntimePrivate.h"

#include <signal.h>
#include <setjmp.h>
#include <sys/resource.h>

#ifdef __linux__
#include <execinfo.h>
#include <dlfcn.h>
#endif

namespace RuntimePlatform
{
	using namespace Runtime;

	THREAD_LOCAL jmp_buf setjmpEnv;
	THREAD_LOCAL Exception::Cause exceptionCause = Exception::Cause::Unknown;
	THREAD_LOCAL Exception* exception = nullptr;

	enum { signalStackNumBytes = SIGSTKSZ };
	THREAD_LOCAL uint8* signalStack = nullptr;
	THREAD_LOCAL uint8* stackMinAddr = nullptr;
	THREAD_LOCAL size_t stackSize = 0;

	void initSignalStack()
	{
		if(!signalStack)
		{
			signalStack = new uint8[signalStackNumBytes];
			stack_t signalStackInfo;
			signalStackInfo.ss_size = signalStackNumBytes;
			signalStackInfo.ss_sp = signalStack;
			signalStackInfo.ss_flags = 0;
			if(sigaltstack(&signalStackInfo,nullptr) < 0)
			{
				throw;
			}

			struct rlimit stackLimit;
			getrlimit(RLIMIT_STACK,&stackLimit);
			stackSize = stackLimit.rlim_cur;

			stackMinAddr = (uint8*)&signalStackInfo - stackSize;
		}
	}

	void signalHandler(int signalNumber,siginfo_t* signalInfo,void*)
	{
		// Derive the exception cause the from signal that was received.
		exceptionCause = Exception::Cause::Unknown;
		switch(signalNumber)
		{
		case SIGFPE:
			switch(signalInfo->si_code)
			{
				case FPE_INTDIV: exceptionCause = Exception::Cause::IntegerDivideByZeroOrIntegerOverflow; break;
			};
			break;
		case SIGSEGV:
		case SIGBUS:
			exceptionCause = signalInfo->si_addr > stackMinAddr - 16384 && signalInfo->si_addr < stackMinAddr + 16384
				? Exception::Cause::StackOverflow
				: Exception::Cause::AccessViolation;
			break;
		};

		// Jump back to the setjmp in catchRuntimeExceptions.
		siglongjmp(setjmpEnv,1);
	}

	Value catchRuntimeExceptions(const std::function<Value()>& thunk)
	{
		initSignalStack();

		Runtime::Value result;

		struct sigaction oldSignalActionSEGV;
		struct sigaction oldSignalActionBUS;
		struct sigaction oldSignalActionFPE;
		
		// Use setjmp to allow signals to jump
		if(!sigsetjmp(setjmpEnv,1))
		{
			// Set a signal handler for the signals we want to intercept.
			struct sigaction signalAction;
			signalAction.sa_sigaction = signalHandler;
			sigemptyset(&signalAction.sa_mask);
			signalAction.sa_flags = SA_SIGINFO | SA_ONSTACK;
			sigaction(SIGSEGV,&signalAction,&oldSignalActionSEGV);
			sigaction(SIGBUS,&signalAction,&oldSignalActionBUS);
			sigaction(SIGFPE,&signalAction,&oldSignalActionFPE);

			// Call the thunk.
			result = thunk();
		}
		else if(exception) { result = Value(exception); }
		else { result = Value(new Exception {exceptionCause}); }

		// Reset the signal state.
		exceptionCause = Exception::Cause::Unknown;
		exception = nullptr;
		sigaction(SIGSEGV,&oldSignalActionSEGV,nullptr);
		sigaction(SIGBUS,&oldSignalActionBUS,nullptr);
		sigaction(SIGFPE,&oldSignalActionFPE,nullptr);

		return result;
	}

	void raiseException(Runtime::Exception* inException)
	{
		exception = inException;
		longjmp(setjmpEnv,1);
	}

	bool describeInstructionPointer(uintptr_t ip,std::string& outDescription)
	{
		#ifdef __linux__
			// Look up static symbol information for the address.
			Dl_info symbolInfo;
			if(dladdr((void*)ip,&symbolInfo) && symbolInfo.dli_sname)
			{
				outDescription = symbolInfo.dli_sname;
				return true;
			}
		#endif
		return false;
	}

	Runtime::ExecutionContext captureExecutionContext()
	{
		#ifdef __linux__
			// Unwind the callstack.
			enum { maxCallStackSize = 512 };
			void* callstackAddresses[maxCallStackSize];
			auto numCallStackEntries = backtrace(callstackAddresses,maxCallStackSize);

			// Copy the return pointers into the stack frames of the resulting ExecutionContext.
			Runtime::ExecutionContext result;
			for(intptr index = 0;index < numCallStackEntries;++index)
			{
				result.stackFrames.push_back({(uintptr)callstackAddresses[index],0});
			}
			return result;
		#else
			return Runtime::ExecutionContext();
		#endif
	}
}

#endif