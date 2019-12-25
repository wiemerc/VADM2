.intel_syntax noprefix

.code64
jmp	l_data
l_code:
int 3
mov	rdi, 1		# file descriptor
pop	rsi	        # pointer to buffer
mov	rdx, 13		# buffer size
mov	rax, 1		# sys_write
syscall

# return value of system calls is in RAX, in case of an error the value is negative with the
# absolute value being the error number. The next 5 lines compute that.
mov	rdi, rax
mov	rbx, rax
sar	rbx, 63
xor	rdi, rbx
sub	rdi, rbx	# RDI = error code
mov	rax, 60		# sys_exit
syscall

l_data:
call	l_code
msg:	.asciz "hello, world\n"
