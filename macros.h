#ifndef __MACROS_H_LOADED
#define __MACROS_H_LOADED 1

#include	<descrip.h>

#define INIT_SDESC(dsc, len, ptr)       {(dsc).dsc$b_dtype = DSC$K_DTYPE_T;\
                        (dsc).dsc$b_class = DSC$K_CLASS_S; (dsc).dsc$w_length = (short) (len);\
                        (dsc).dsc$a_pointer = (char *) (ptr);}

#define INIT_DDESC(dsc) {(dsc).dsc$b_dtype = DSC$K_DTYPE_T;\
                        (dsc).dsc$b_class = DSC$K_CLASS_D;(dsc).dsc$w_length = 0;\
                        (dsc).dsc$a_pointer = NULL;}

#define $MIN(x,y)       ((x > y)?y:x)
#define $MAX(x,y)       ((x < y)?y:x)

#define	RETRY_COUNT	(-1)

#define TRACE		if(trace_flag)_trace(__MODULE__,__func__,__LINE__,

#define $ASCIc(a)	sizeof(a)-1,(a)

#endif /* __MACROS_H_LOADED */
