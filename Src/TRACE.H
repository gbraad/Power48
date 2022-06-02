
//#define TRACE_ENABLED 

#ifndef TRACE_ENABLED

	#define TRACE1(d)
	#define TRACE2(d,e)
	#define TRACE3(d,e,f)
	#define TRACE4(d,e,f,g)
	#define NOT_FUNCTION_CALL
	
#else

	#define TRACE1(d) Q->traceNumber=d;TracePrint_arm(Q);
	#define TRACE2(d,e) Q->traceNumber=d;Q->traceVal=(UInt32)e;TracePrint_arm(Q);
	#define TRACE3(d,e,f) Q->traceNumber=d;Q->traceVal=(UInt32)e;Q->traceVal2=(UInt32)f;TracePrint_arm(Q);
	#define TRACE4(d,e,f,g) Q->traceNumber=d;Q->traceVal=(UInt32)e;Q->traceVal2=(UInt32)f;Q->traceVal3=(UInt32)g;TracePrint_arm(Q);
	extern void TracePrint(void);
	extern void TraceOpen(void);
	extern void TraceClose(void);
	#define NOT_FUNCTION_CALL Q->poporpush=0;

#endif

