package main

import "fmt"

func main() {
	escape()
	fmt.Printf("%064b\n", 2)
	fmt.Printf("%064b\n", ^2) // 存储的是补码  所以源码是补码减1再取反  0...010 取反 1...101 恢复源码 减一1...100 取反(符号位不变) 10...011 = -3
	fmt.Println(^2)
	return
}

func escape() *int {
	a := 1
	return &a
}
