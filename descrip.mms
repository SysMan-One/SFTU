linkflags =
!/debu/trace
cflags	=/list/include=([]) /arch=host/opt=(tune=host,lev=5)/REENTRANCY/nowar
!/debu/noop


srcs	= sftu_cld.cld, sftu.c, sftu_msg.msg
objs	= sftu_cld.obj, sftu.obj,  sftu_msg.obj

incs	= sftudef.h, sftu_msg.h
sdls	= sftudef.sdl

all		: sftu.exe

sftu.exe	: $(objs)
	link $(linkflags)/exe=$(mms$target)  $(objs)

sftu_cld.obj	: sftu_cld.cld
	set command/object $(mms$source)
	
sftu.obj	:  sftu.c, $(incs)
	cc $(cflags) $(mms$source)

sftudef.h	: sftudef.sdl
	sdl/vms/c_dev/language=(cc=$(mms$target)) $(mms$source_name).sdl


.msg.obj	: $(srcs)
	message/sdl	$(mms$source)


!.msg.h		: $(srcs)
sftu_msg.h	: sftu_msg.msg
	message/sdl	$(mms$source)
	sdl/vms/c_dev/language=(cc=$(mms$target)) $(mms$source_name).sdl



.sdl.h		: $(sdls)
	sdl/vms/c_dev/language=(cc=$(mms$target).h) $(mms$source_name).sdl

	
!*********************************************************************
clean   :
  @ delete/log *.obj;*,*.lis;*,*.exe;*,*.map;*
!*************
