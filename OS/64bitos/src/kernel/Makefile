head.o:head.S
# -E表示仅预处理 不执行编译汇编或者链接 预处理 编译生成汇编代码一类  汇编生成obj
	gcc -E head.S > head.s    
	as --64 -o head.o head.s