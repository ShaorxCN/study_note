; q 后缀代表四字操作 也就是64bit  l代表双字
; 这里是AT&T 汇编

0000000000001125 <test>:
    1125:	55                   	push   %rbp
    1126:	48 89 e5             	mov    %rsp,%rbp          ;这里和intel的相反 是将rsp的内容复制到rbp中
    1129:	c7 45 fc 00 00 00 00 	movl   $0x0,-0x4(%rbp)
    1130:	83 45 fc 02          	addl   $0x2,-0x4(%rbp)
    1134:	8b 45 fc             	mov    -0x4(%rbp),%eax
    1137:	5d                   	pop    %rbp
    1138:	c3                   	retq   

0000000000001139 <main>:
    1139:	55                   	push   %rbp
    113a:	48 89 e5             	mov    %rsp,%rbp
    113d:	b8 00 00 00 00       	mov    $0x0,%eax
    1142:	e8 de ff ff ff       	callq  1125 <test>
    1147:	b8 00 00 00 00       	mov    $0x0,%eax
    114c:	5d                   	pop    %rbp
    114d:	c3                   	retq   
    114e:	66 90                	xchg   %ax,%ax

