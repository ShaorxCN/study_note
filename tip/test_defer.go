package main

import (
	"fmt"
	"time"
)

func func_test_defer2() {
	fmt.Println("test defer 2")
}

func test_defer() {
	var a int

	var b int = 2

	defer func() {
		fmt.Println("defer 1", a)
	}()

	defer func_test_defer2()

	fmt.Println("test_defer end", b)
}

func main() {
	test_defer()
	time.Sleep(5 * time.Second)
	fmt.Println("end")
	return
}
