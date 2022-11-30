<h1>Heap Allocation(内存堆分配)</h1>

This post adds support for heap allocation to our kernel. First, it gives an introduction to dynamic memory and shows how the borrow checker prevents common allocation errors. It then implements the basic allocation interface of Rust, creates a heap memory region, and sets up an allocator crate. At the end of this post, all the allocation and collection types of the built-in `alloc` crate will be available to our kernel.

本文将为内核添加对堆分配的支持。首先介绍动态内存的基础知识，并展示Rust借用检查器如何防止常见的分配错误。然后将实现Rust的基础分配器接口，创建一个堆内存区域，并添加分配器crate。在本文末尾，Rust内置`alloc` crate中的所有分配和集合类型都将在内核中可用。

This blog is openly developed on GitHub. If you have any problems or questions, please open an issue there. You can also leave comments at the bottom. The complete source code for this post can be found in the post-10 branch.

这篇博客是在[GitHub](https://github.com/phil-opp/blog_os)上公开开发的。如果你有任何问题或疑问，请在那里开一个问题。你也可以在底部留下评论。
本帖的完整源代码可以在[post-10](https://github.com/phil-opp/blog_os/tree/post-10)分支中找到。

<h2>Local and Static Variables(局部变量和静态变量)</h2>

We currently use two types of variables in our kernel: local variables and static variables. Local variables are stored on the call stack and are only valid until the surrounding function returns. Static variables are stored at a fixed memory location and always live for the complete lifetime of the program.

目前在我们的内核中使用两种类型的变量：局部变量和静态变量。 局部变量存储在调用栈中，只在定义该变量的函数返回之前有效。 静态变量存储在内存的一个固定位置，在程序的整个生命周期内都是有效的。

<h3>Local Variables(局部变量)</h3>

Local variables are stored on the [call stack](https://en.wikipedia.org/wiki/Call_stack), which is a [stack data structure](https://en.wikipedia.org/wiki/Stack_(abstract_data_type)) that supports `push` and `pop` operations. On each function entry, the parameters, the return address, and the local variables of the called function are pushed by the compiler:

局部变量存储在[调用栈](https://en.wikipedia.org/wiki/Call_stack)上。 这是一个称为[栈的数据结构](https://en.wikipedia.org/wiki/Stack_(abstract_data_type))，支持`push`和`pop`指令。 每次进入一个函数时，被调用函数的参数、返回地址和局部变量都会被编译器入栈：


<img src="./img/call-stack.svg">




The above example shows the call stack after the `outer` function called the `inner` function. We see that the call stack contains the local variables of `outer` first. On the inner call, the parameter `1` and the return address for the function were pushed. Then control was transferred to `inner`, which pushed its local variables.

上面的例子显示了`outer`函数调用`inner`函数后的调用栈。 可以看到，调用堆栈中首先有 `outer` 的局部变量.当 `inner` 被调用时，参数 `1` 和这个函数的返回地址被入栈。 然后控制权转移到`inner`函数，并继续将其局部变量入栈。

After the `inner` function returns, its part of the call stack is popped again and only the local variables of `outer` remain:

`inner`函数返回后，其位于调用栈中的相关部分弹栈，此时调用栈只仅剩下`outer`函数相关的局部变量：

<img src="./img/call-stack-return.svg">


We see that the local variables of inner only live until the function returns. The Rust compiler enforces these lifetimes and throws an error when we use a value for too long, for example when we try to return a reference to a local variable:

我们看到`inner`的局部变量仅在函数返回之前有效。Rust编译器会强制执行这些生命周期，当我们持有变量时间过长时，比如尝试返回对局部变量的引用，编译器会抛出一个错误：

```rust
fn inner(i: usize) -> &'static u32 {
    let z = [1, 2, 3];
    &z[i]
}
```

[run the example on the playground](https://play.rust-lang.org/?version=stable&mode=debug&edition=2018&gist=6186a0f3a54f468e1de8894996d12819)

While returning a reference makes no sense in this example, there are cases where we want a variable to live longer than the function. We already saw such a case in our kernel when we tried to [load an interrupt descriptor table](https://os.phil-opp.com/cpu-exceptions/#loading-the-idt) and had to use a `static` variable to extend the lifetime.

虽然本例中返回引用并无意义，但在某些情况下，我们希望变量比其所在函数的生命周期更长。我们已经在编写内核的过程中遇到了这种情况，比如在尝试[加载中断描述符表](https://os.phil-opp.com/cpu-exceptions/#loading-the-idt)时就不得不使用`static`变量来延长其生命周期。

<h3>Static Variables(静态变量)</h3>

Static variables are stored at a fixed memory location separate from the stack. This memory location is assigned at compile time by the linker and encoded in the executable. Statics live for the complete runtime of the program, so they have the `'static` lifetime and can always be referenced from local variables:


静态变量存储在栈以外的固定内存位置中。此存储位置由链接器在编译时分配并编码在可执行文件中。静态变量在程序的完整运行时内始终有效，因此它们具有`'static'`生命周期，并且始终可以被局部变量引用：


<img src="./img/call-stack-static.svg">



When the `inner` function returns in the above example, its part of the call stack is destroyed. The static variables live in a separate memory range that is never destroyed, so the `&Z[1]` reference is still valid after the return.

上图中的`inner`函数返回时，其在调用栈中的相关部分被析构。而静态变量位于一个独立的内存范围中，并不会被析构，因此`&Z[1]`的引用在函数返回后仍然有效。

Apart from the `'static'` lifetime, static variables also have the useful property that their location is known at compile time, so that no reference is needed for accessing them. We utilized that property for our `println` macro: By using a [static Writer](https://os.phil-opp.com/vga-text-mode/#a-global-interface) internally, there is no `&mut Writer` reference needed to invoke the macro, which is very useful in [exception handlers](https://os.phil-opp.com/cpu-exceptions/#implementation), where we don’t have access to any additional variables.

除了`'static'`生命周期以外，静态变量还有另一个有用的属性，即其内存位置在编译时就已确定，因此不需要引用就能够对其进行访问。我们利用该属性实现了`println`宏：通过在内部使用[static Writer](https://os.phil-opp.com/vga-text-mode/#a-global-interface)，即便不使用`&mut Writer`引用也能够调用该宏。这在[异常处理程序](https://os.phil-opp.com/cpu-exceptions/#implementation)中非常有用，因为在其中我们无法访问任何其他变量。

However, this property of static variables brings a crucial drawback: they are read-only by default. Rust enforces this because a [data race](https://doc.rust-lang.org/nomicon/races.html) would occur if, e.g., two threads modified a static variable at the same time. The only way to modify a static variable is to encapsulate it in a [Mutex](https://docs.rs/spin/0.5.2/spin/struct.Mutex.html) type, which ensures that only a single `&mut` reference exists at any point in time. We already used a `Mutex` for our [static VGA buffer Writer](https://os.phil-opp.com/vga-text-mode/#spinlocks).



但是，静态变量的这个属性也带来了一个致命缺点：它们默认是只读的。Rust强制此规则是为了避免[数据竞争](https://doc.rust-lang.org/nomicon/races.html)，比如当两个线程同时修改一个静态变量时。修改静态变量的唯一方法是将其封装在[Mutex](https://docs.rs/spin/0.5.2/spin/struct.Mutex.html)类型中，从而确保在任何时刻中仅存在一个`&mut`引用。我们已经将`Mutex`用于[静态VGA缓冲区Writer](https://os.phil-opp.com/vga-text-mode/#spinlocks)。


<h2>Dynamic Memory(动态内存)</h2>

Local and static variables are already very powerful together and enable most use cases. However, we saw that they both have their limitations:

- Local variables only live until the end of the surrounding function or block. This is because they live on the call stack and are destroyed after the surrounding function returns.
- Static variables always live for the complete runtime of the program, so there is no way to reclaim and reuse their memory when they’re no longer needed. Also, they have unclear ownership semantics and are accessible from all functions, so they need to be protected by a `Mutex` when we want to modify them.


结合使用局部变量和静态变量已经能够实现非常强大的功能，足以应付大多数用例了。但是，它们仍然具有一定的局限性：

- 局部变量仅在其所在的函数或代码块结束前有效。这是因为它们存在于调用栈中，并在上下文函数返回后被销毁。
- 静态变量在程序的运行周期内始终有效，因此无法在不再需要它们时回收和重用它们的内存。此外，它们的所有权语义不明确，且能够被任意函数访问，所以我们才会在需要修改时使用`Mutex`对其进行保护。

Another limitation of local and static variables is that they have a fixed size. So they can’t store a collection that dynamically grows when more elements are added. (There are proposals for [unsized rvalues](https://github.com/rust-lang/rust/issues/48055) in Rust that would allow dynamically sized local variables, but they only work in some specific cases.)

局部变量和静态变量的另一个局限性，就是它们的大小固定，因此，遇到需要添加更多元素的情况时，它们将无法存储这些动态增长的集合。（Rust中有一些关于[unsized rvalues](https://github.com/rust-lang/rust/issues/48055)的提案，以允许局部变量具有动态大小，但它们仅在某些特定情况下有效。）

To circumvent these drawbacks, programming languages often support a third memory region for storing variables called the **heap**. The heap supports dynamic memory allocation at runtime through two functions called `allocate` and `deallocate`. It works in the following way: The `allocate` function returns a free chunk of memory of the specified size that can be used to store a variable. This variable then lives until it is freed by calling the `deallocate` function with a reference to the variable.

为了规避这些缺点，编程语言通常支持第三个用于存储变量的内存区域，称为**堆**。堆支持在运行时通过两个名为`allocate`和`deallocate`的函数进行动态内存分配。它以如下方式工作。allocate函数返回一个指定大小的自由内存块，可用于存储一个变量。然后，这个变量将一直存在，直到在其引用上调用`deallocate`函数将其释放为止。

Let’s go through an example:

让我们来看一个例子：

<img src="./img/call-stack-heap.svg">



Here the `inner` function uses heap memory instead of static variables for storing `z`. It first allocates a memory block of the required size, which returns a `*mut u32`  [raw pointer](https://doc.rust-lang.org/book/ch19-01-unsafe-rust.html#dereferencing-a-raw-pointer). It then uses the [ptr::write](https://doc.rust-lang.org/core/ptr/fn.write.html) method to write the array `[1,2,3]` to it. In the last step, it uses the [offset](https://doc.rust-lang.org/std/primitive.pointer.html#method.offset) function to calculate a pointer to the i-th element and then returns it. (Note that we omitted some required casts and unsafe blocks in this example function for brevity.)

在这里，`inner`函数使用堆内存而不是静态变量来存储`z`。首先按照所需大小分配内存块，然后返回`*mut u32` [裸指针](https://doc.rust-lang.org/book/ch19-01-unsafe-rust.html#dereferencing-a-raw-pointer)。然后使用[ptr::write](https://doc.rust-lang.org/core/ptr/fn.write.html)方法写入数组`[1,2,3]`。最后使用[offset](https://doc.rust-lang.org/std/primitive.pointer.html#method.offset)函数计算指向第i个元素的指针，并将其返回。（请注意，为简洁起见，在此示例函数中，我们省略了一些必需的强制转换和unsafe块。）

The allocated memory lives until it is explicitly freed through a call to `deallocate`. Thus, the returned pointer is still valid even after `inner` returned and its part of the call stack was destroyed. The advantage of using heap memory compared to static memory is that the memory can be reused after it is freed, which we do through the `deallocate` call in `outer`. After that call, the situation looks like this:

分配的内存将会一直有效，直到通过调用`dealloc`显式释放为止。因此，即使在`inner`返回且其相关部分的调用栈被销毁之后，返回的指针依然有效。与静态内存相比，使用堆内存的优势在于内存可以在释放后重用，这是通过`oute`r的`deallocate`调用实现的。调用结束后的情形如下：

<img src="./img/call-stack-heap-freed.svg">


We see that the `z[1]` slot is free again and can be reused for the next `allocate` call. However, we also see that `z[0]` and `z[2]` are never freed because we never deallocate them. Such a bug is called a memory leak and is often the cause of excessive memory consumption of programs (just imagine what happens when we call `inner` repeatedly in a loop). This might seem bad, but there are much more dangerous types of bugs that can happen with dynamic allocation.

可以看到`z[1]`所在堆中的位置又空闲了，可以重新用于下一次`allocate`调用。但是，还可以看到`z[0]`和`z[2]`还未释放，因为我们从未释放过它们。这种错误称为内存泄漏，也通常是导致程序过度消耗内存的原因（试想一下，当我们在循环中重复调用`inner`时会发生什么）。这看起来很不好，然而动态分配可能会导致更多危险的错误。

<h3>Common Errors(常见错误)</h3>

Apart from memory leaks, which are unfortunate but don’t make the program vulnerable to attackers, there are two common types of bugs with more severe consequences:

- When we accidentally continue to use a variable after calling `deallocate` on it, we have a so-called **use-after-free** vulnerability. Such a bug causes undefined behavior and can often be exploited by attackers to execute arbitrary code.
- When we accidentally free a variable twice, we have a **double-free** vulnerability. This is problematic because it might free a different allocation that was allocated in the same spot after the first `deallocate` call. Thus, it can lead to a use-after-free vulnerability again.
  
内存泄漏虽然会导致过度消耗内存，但并不会使程序更容易受到攻击。除此之外，还有两种常见的错误类型，其后果更为严重：

- 当我们在调用`deallocate`后意外地继续使用变量时，便产生了一个称作**释放后使用**的漏洞。这种漏洞会导致未定义的行为，此外，攻击者也经常通过该漏洞来执行任意代码。
- 当我们不小心两次释放变量时，便产生了一个**双重释放**漏洞。这是有问题的，因为它这可能会释放第一次调用`deallocate`后在该位置重新分配的变量。因此，它可能再次导致释放后使用漏洞。


These types of vulnerabilities are commonly known, so one might expect that people have learned how to avoid them by now. But no, such vulnerabilities are still regularly found, for example this [use-after-free vulnerability in Linux](https://securityboulevard.com/2019/02/linux-use-after-free-vulnerability-found-in-linux-2-6-through-4-20-11/) (2019), that allowed arbitrary code execution. A web search like `use-after-free linux {current year}` will probably always yield results. This shows that even the best programmers are not always able to correctly handle dynamic memory in complex projects.

这些种类的漏洞是很常见的，看起来可以期望人们现在已经学会了如何规避它们。现实很遗憾，这些漏洞仍然经常出现，例如[Linux（2019年）的这个use-after-free漏洞](https://securityboulevard.com/2019/02/linux-use-after-free-vulnerability-found-in-linux-2-6-through-4-20-11/),它允许执行任意代码。这也表明即使是最优秀的程序员也不一定总是能够正确处理复杂项目中的动态内存。

To avoid these issues, many languages, such as Java or Python, manage dynamic memory automatically using a technique called [garbage collection](https://en.wikipedia.org/wiki/Garbage_collection_(computer_science)). The idea is that the programmer never invokes `deallocate` manually. Instead, the program is regularly paused and scanned for unused heap variables, which are then automatically deallocated. Thus, the above vulnerabilities can never occur. The drawbacks are the performance overhead of the regular scan and the probably long pause times.

为了规避这些漏洞，许多语言——例如Java或Python——都使用称为[垃圾回收](https://en.wikipedia.org/wiki/Garbage_collection_(computer_science))的技术自动管理动态内存。其意图是使程序员不再需要手动调用`deallocate`，转而使程序定期暂停并扫描未使用的堆变量，然后将它们自动释放。于是，上述漏洞也就不会再出现了。不过，其缺点是常规扫描有一定的性能开销，以及暂停的时间可能会较长。

Rust takes a different approach to the problem: It uses a concept called [ownership](https://doc.rust-lang.org/book/ch04-01-what-is-ownership.html) that is able to check the correctness of dynamic memory operations at compile time. Thus, no garbage collection is needed to avoid the mentioned vulnerabilities, which means that there is no performance overhead. Another advantage of this approach is that the programmer still has fine-grained control over the use of dynamic memory, just like with C or C++.

针对此问题Rust采用了不同的解决方案：它使用一种称为[所有权](https://doc.rust-lang.org/book/ch04-01-what-is-ownership.html)的概念，能够在编译时检查动态内存操作的正确性。因此，Rust不需要垃圾收集也能避免上述漏洞，这意味着没有性能开销。该方案的另一个优点是，程序员仍然可以像使用C或C++一样对动态内存进行细粒度的控制。

