#include <iostream>
#include <node.h>
#include <v8.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>        
#include <fcntl.h>           
#include <sys/mman.h>

enum {
    pjsip_dll_init=0,
	pjsip_dll_add_account,
	pjsip_dll_make_call,
	pjsip_dll_answer_call,
	pjsip_dll_release_call,
	pjsip_dll_poll_for_events,
	pjsip_dll_get_params,
	pjsip_dll_shutdown,
	PJSIP_DLL_EVENTS_END
};

#define MAX_STR 80
typedef struct CallbackParams_s {
    char displayName[MAX_STR];
	char userName[80];
	char sipIdentity[80];
	char sipProxyAddress[80];
	char password[80];
	char sipurl[80];
	char calleeURI[80];

    unsigned int releaseCallId;
    unsigned int answerCallId;
    unsigned int answerCode;
    unsigned int timeout;
    unsigned int rate_percent;

    unsigned int tcpOnly;
    unsigned int sipPort;
	unsigned int cmd;
	unsigned int res;
} CallbackParams;

enum {
	CMD_REGISTER=0,
	CMD_CALL,
	RES_SUCCESS,
	RES_CONFIRMED,
	CMD_NONE
};

extern "C" int DLLInvokeCallback(int cbNum, CallbackParams *cb);
pid_t child;
int cmd=CMD_NONE;
using namespace v8;

CallbackParams cb;
CallbackParams *cbParams;
int sId;
int account_added = 0;
std::string domain;

Handle<Value> MyInit(const Arguments &args) {
  HandleScope scope;

	std::string str("Ashish Lal");
	strcpy(cb.displayName,str.c_str());

	cb.sipPort = 5070;
	cb.timeout = 200;
    if(!DLLInvokeCallback(pjsip_dll_init, &cb)) {
		return scope.Close(String::New("Success"));
	}
	return scope.Close(String::New("Failed"));
}

Handle<Value> Register(const Arguments& args) {
  HandleScope scope;

  std::cout << "inside Register.." << std::endl;
  if (args.Length() < 3) {
    ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    return scope.Close(Undefined());
  }

   if (!args[0]->IsString() || !args[1]->IsString() || !args[2]->IsString()) {
	    ThrowException(Exception::TypeError(String::New("Wrong arguments")));
	    return scope.Close(Undefined());
    }

    v8::String::Utf8Value param1(args[0]->ToString());
	domain = std::string(*param1);

    v8::String::Utf8Value param2(args[1]->ToString());
	std::string uname = std::string(*param2);

    v8::String::Utf8Value param3(args[2]->ToString());
	std::string pass = std::string(*param3);

    v8::String::Utf8Value param4(args[3]->ToString());
	std::string port = std::string(*param4);

	std::string str("Ashish Lal");
	std::cout << "cbParams = " << cbParams << std::endl;
	cbParams = (CallbackParams *) mmap (NULL, sizeof(CallbackParams)+512,
	        PROT_READ|PROT_WRITE, MAP_SHARED, sId, 0);
	strcpy(cbParams->displayName, str.c_str());;
	std::cout << "1.... " << std::endl;

	std::string sipcolon("sip:");
	std::string sipat("@");
	strcpy(cbParams->userName,uname.c_str());
	strcpy(cbParams->password, pass.c_str());
	std::string sipIdentity = sipcolon + uname + sipat + domain;
	strcpy(cbParams->sipIdentity, sipIdentity.c_str());
	std::string sipurl = sipcolon + uname + sipat + domain;
	strcpy(cbParams->sipurl, sipurl.c_str());
	std::string sipProxyAddress = sipcolon + domain;
	strcpy(cbParams->sipProxyAddress, sipProxyAddress.c_str());
	cbParams->sipPort = 5070;
	std::cout << "setting cmd to CMD_REGISTER..." << std::endl;
	cbParams->cmd = CMD_REGISTER;
	while(cbParams->res != RES_SUCCESS) {
		struct timespec tim, tim2;
		tim.tv_sec = 0;
		tim.tv_nsec = 200;
	  	if(nanosleep(&tim , &tim2) < 0 ) {
			return scope.Close(String::New("failed"));
		}
	}
	return scope.Close(String::New("REGISTERED"));
}

Handle<Value> Call(const Arguments& args) {
	HandleScope scope;
    v8::String::Utf8Value param1(args[0]->ToString());
	std::string callee = std::string(*param1);

	std::string sipcolon("sip:");
	std::string sipat("@");
	std::string calleeURI = sipcolon + callee + sipat + domain;
	strcpy(cbParams->calleeURI, calleeURI.c_str());
	cbParams->cmd = CMD_CALL;
	while(cbParams->res != RES_CONFIRMED) {
		struct timespec tim, tim2;
		tim.tv_sec = 0;
		tim.tv_nsec = 200;
	  	if(nanosleep(&tim , &tim2) < 0 ) {
			return scope.Close(String::New("failed"));
		}
	}
	return scope.Close(String::New("CONFIRMED"));
}

Handle<Value> Poll(const Arguments& args) {
	HandleScope scope;
	cb.timeout = 200;
	/* if(account_added == 1) */
		DLLInvokeCallback(pjsip_dll_poll_for_events,&cb); 
	return scope.Close(String::New("Success"));
}

void Dialer_Confirmed() {
	cbParams->res = RES_CONFIRMED;
}

void Init(Handle<Object> exports)
{
	/* We create a shared memory object */
	sId = shm_open("MySharedData", O_RDWR|O_CREAT|O_TRUNC, 0666);

	/* We allocate memory to shared object */         
    ftruncate (sId, sizeof(CallbackParams)+512);

    /* we attach the allocated object to our process */
	cbParams = (CallbackParams *) mmap (NULL, sizeof(CallbackParams)+512, 
		PROT_READ|PROT_WRITE, MAP_SHARED, sId, 0);
	cbParams->cmd = CMD_NONE;
	std::cout << "cbParams = " << cbParams << std::endl;
  	exports->Set(String::NewSymbol("init"),
  		FunctionTemplate::New(MyInit)->GetFunction());
  	exports->Set(String::NewSymbol("register"),
  		FunctionTemplate::New(Register)->GetFunction());
  	exports->Set(String::NewSymbol("call"),
  		FunctionTemplate::New(Call)->GetFunction());
  	exports->Set(String::NewSymbol("poll"),
  		FunctionTemplate::New(Poll)->GetFunction());

	child = fork();
	if(child == 0) {
		std::string str("Ashish Lal");
		strcpy(cb.displayName,str.c_str());
		cb.sipPort = 5070;
		cb.timeout = 200;
		if(!DLLInvokeCallback(pjsip_dll_init, &cb)) {
		      /* nothing */
			  std::cout << "init succeeded.." << std::endl;
		}

		for(;;) {
			cb.timeout = 200;
			DLLInvokeCallback(pjsip_dll_poll_for_events,&cb); 
			switch(cbParams->cmd) {
				case CMD_REGISTER:
					{
					std::cout << "registering.." << std::endl;
					int res = DLLInvokeCallback(pjsip_dll_add_account,cbParams);
				    if(res == 0) {
					    std::cout << "setting result to success" << std::endl;
					    cbParams->res = RES_SUCCESS;
					}
					}
					cbParams->cmd = CMD_NONE;
					break;
				case CMD_CALL:
					DLLInvokeCallback(pjsip_dll_make_call,cbParams);
					cbParams->cmd = CMD_NONE;
					break;
				default:
					break;
			}
		}
	}
}

NODE_MODULE(dialer, Init)
