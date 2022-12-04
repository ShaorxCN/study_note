# Async/Await

In this post, we explore cooperative multitasking and the async/await feature of Rust. We take a detailed look at how async/await works in Rust, including the design of the `Future` trait, the state machine transformation, and pinning. We then add basic support for async/await to our kernel by creating an asynchronous keyboard task and a basic executor.

在这篇文章中，我们将探讨合作式多任务系统和Rust的async/await功能。我们将会详细研究async/await在Rust中的工作原理，包括`Future` trait的设计、状态机的转换和`pinning`。然后，我们通过创建一个异步键盘任务和一个基础的执行器来为我们的内核添加对async/await的基本支持。

This blog is openly developed on[GitHub](https://github.com/phil-opp/blog_os). If you have any problems or questions, please open an issue there. You can also leave comments at the bottom. The complete source code for this post can be found in the [post-12](https://github.com/phil-opp/blog_os/tree/post-12) branch.

这篇博客是在[GitHub](https://github.com/phil-opp/blog_os)上公开开发的。如果你有任何问题或疑问，请在那里开一个问题。你也可以在底部留下评论。本帖的完整源代码可以在[post-12](https://github.com/phil-opp/blog_os/tree/post-12)分支中找到。

## Multitasking(多任务)

One of the fundamental features of most operating systems is [multitasking](https://en.wikipedia.org/wiki/Computer_multitasking), which is the ability to execute multiple tasks concurrently. For example, you probably have other programs open while looking at this post, such as a text editor or a terminal window. Even if you have only a single browser window open, there are probably various background tasks for managing your desktop windows, checking for updates, or indexing files.

大多数操作系统的基本功能之一是[多任务](https://en.wikipedia.org/wiki/Computer_multitasking)，即能够同时执行多个任务。例如，你在看这篇文章时可能还打开了其他程序，如文本编辑器或终端窗口。即使你只打开了一个浏览器窗口，也可能有各种后台任务来管理你的桌面窗口，检查更新，或索引文件。

While it seems like all tasks run in parallel, only a single task can be executed on a CPU core at a time. To create the illusion that the tasks run in parallel, the operating system rapidly switches between active tasks so that each one can make a bit of progress. Since computers are fast, we don’t notice these switches most of the time.

虽然看起来所有的任务都是并行运行的，但每次只有一个任务可以在一个CPU核上执行。为了制造任务并行运行的假象，操作系统在活动任务之间快速切换，以便每个任务都能取得一点进展。由于计算机的速度很快，我们在大多数时候并没有注意到这些切换。

While single-core CPUs can only execute a single task at a time, multi-core CPUs can run multiple tasks in a truly parallel way. For example, a CPU with 8 cores can run 8 tasks at the same time. We will explain how to setup multi-core CPUs in a future post. For this post, we will focus on single-core CPUs for simplicity. (It’s worth noting that all multi-core CPUs start with only a single active core, so we can treat them as single-core CPUs for now.)

虽然单核CPU一次只能执行一个任务，但多核CPU可以以真正的并行方式运行多个任务。例如，一个有8个核心的CPU可以同时运行8个任务。我们将在以后的文章中解释如何设置多核CPU。在这篇文章中，为了简单起见，我们将专注于单核CPU。(值得注意的是，所有多核CPU开始时只有一个活跃的内核，所以我们现在可以把它们当作单核CPU)。


There are two forms of multitasking: Cooperative multitasking requires tasks to regularly give up control of the CPU so that other tasks can make progress. Preemptive multitasking uses operating system functionality to switch threads at arbitrary points in time by forcibly pausing them. In the following we will explore the two forms of multitasking in more detail and discuss their respective advantages and drawbacks.

有两种形式的多任务。合作式多任务要求任务定期放弃对CPU的控制，以便其他任务能够取得进展。抢占式多任务使用操作系统的功能，在任意时间点通过强制暂停来切换线程。在下文中，我们将更详细地探讨这两种形式的多任务处理，并讨论它们各自的优点和缺点。

### Preemptive Multitasking(抢占式多任务)

The idea behind preemptive multitasking is that the operating system controls when to switch tasks. For that, it utilizes the fact that it regains control of the CPU on each interrupt. This makes it possible to switch tasks whenever new input is available to the system. For example, it would be possible to switch tasks when the mouse is moved or a network packet arrives. The operating system can also determine the exact time that a task is allowed to run by configuring a hardware timer to send an interrupt after that time.

抢占式多任务背后的理念是，操作系统控制何时切换任务。为此，它利用了在每次中断时重新获得CPU控制权的事实。这使得每当系统有新的输入时就有可能切换任务。例如，当鼠标被移动或一个网络数据包到达时，就有可能切换任务。操作系统还可以通过配置一个硬件定时器来确定一个任务被允许运行的确切时间，以便在该时间之后发送一个中断。

The following graphic illustrates the task switching process on a hardware interrupt:

下图说明了硬件中断上的任务切换过程：

<img src="./img/regain-control-on-interrupt.svg">

In the first row, the CPU is executing task `A1` of program `A`. All other tasks are paused. In the second row, a hardware interrupt arrives at the CPU. As described in the[ Hardware Interrupts](https://os.phil-opp.com/hardware-interrupts/) post, the CPU immediately stops the execution of task `A1` and jumps to the interrupt handler defined in the interrupt descriptor table (IDT). Through this interrupt handler, the operating system now has control of the CPU again, which allows it to switch to task `B1` instead of continuing task `A1`.

在第一行，CPU正在执行程序`A`的任务`A1`，所有其他任务都暂停了。在第二行中，一个硬件中断到达了CPU。如[硬件中断](https://os.phil-opp.com/hardware-interrupts/)一文所述，CPU立即停止执行任务`A1`，并跳转到中断描述符表（IDT）中定义的中断处理程序。通过这个中断处理程序，操作系统现在又有了对CPU的控制权，这使得它可以切换到任务`B1`，而不是继续任务`A1`。

#### Saving State(保存状态)

Since tasks are interrupted at arbitrary points in time, they might be in the middle of some calculations. In order to be able to resume them later, the operating system must backup the whole state of the task, including its [call stack](https://en.wikipedia.org/wiki/Call_stack) and the values of all CPU registers. This process is called a [context switch](https://en.wikipedia.org/wiki/Context_switch).

由于任务是在任意时间点中断的，它们可能处于某些计算的过程中。为了以后能够恢复它们，操作系统必须备份任务的整个状态，包括其[调用栈](https://en.wikipedia.org/wiki/Call_stack)和所有CPU寄存器的值。这个过程被称为[上下文切换](https://en.wikipedia.org/wiki/Context_switch)。


As the call stack can be very large, the operating system typically sets up a separate call stack for each task instead of backing up the call stack content on each task switch. Such a task with its own stack is called a [thread of execution](https://en.wikipedia.org/wiki/Thread_(computing)) or thread for short. By using a separate stack for each task, only the register contents need to be saved on a context switch (including the program counter and stack pointer). This approach minimizes the performance overhead of a context switch, which is very important since context switches often occur up to 100 times per second.

由于调用堆栈可能非常大，操作系统通常为每个任务设置一个单独的调用栈，而不是在每个任务切换时备份调用栈内容。这种有自己栈的任务被称为[执行线程](https://en.wikipedia.org/wiki/Thread_(computing))或简称为线程。通过为每个任务使用单独的堆栈，在上下文切换时只需要保存寄存器内容（包括程序计数器和堆栈指针）。这种方法最大限度地减少了上下文切换的性能开销，这一点非常重要，因为上下文切换经常每秒发生100次。


#### Discussion(讨论)

The main advantage of preemptive multitasking is that the operating system can fully control the allowed execution time of a task. This way, it can guarantee that each task gets a fair share of the CPU time, without the need to trust the tasks to cooperate. This is especially important when running third-party tasks or when multiple users share a system.

抢占式多任务的主要优点是，操作系统可以完全控制任务允许执行的时间。这样，它可以保证每个任务得到公平的CPU时间份额，而不需要相信任务之间的合作。当运行第三方任务或多个用户共享一个系统时，这一点尤其重要。

The disadvantage of preemption is that each task requires its own stack. Compared to a shared stack, this results in higher memory usage per task and often limits the number of tasks in the system. Another disadvantage is that the operating system always has to save the complete CPU register state on each task switch, even if the task only used a small subset of the registers.

抢占的缺点是，每个任务都需要自己的栈。与共享栈相比，这导致每个任务的内存占用率较高，并且常常限制系统中的任务数量。另一个缺点是，操作系统总是要在每次任务切换时保存完整的CPU寄存器状态，即使该任务只使用了寄存器的一小部分。


Preemptive multitasking and threads are fundamental components of an operating system because they make it possible to run untrusted userspace programs. We will discuss these concepts in full detail in future posts. For this post, however, we will focus on cooperative multitasking, which also provides useful capabilities for our kernel.

抢占式多任务和线程是操作系统的基本组成部分，因为它们使运行不受信任的用户空间程序成为可能。我们将在未来的文章中详细讨论这些概念。然而，在这篇文章中，我们将重点讨论合作式多任务，它也为我们的内核提供了有用的功能。

### Cooperative Multitasking(合作式多任务)

Instead of forcibly pausing running tasks at arbitrary points in time, cooperative multitasking lets each task run until it voluntarily gives up control of the CPU. This allows tasks to pause themselves at convenient points in time, for example, when they need to wait for an I/O operation anyway.

合作式多任务不是在任意时间点上强行暂停正在运行的任务，而是让每个任务运行到它自愿放弃对CPU的控制。这允许任务在方便的时间点暂停自己，例如，当他们需要等待I/O操作时。


Cooperative multitasking is often used at the language level, like in the form of [coroutines](https://en.wikipedia.org/wiki/Coroutine) or [async/await](https://rust-lang.github.io/async-book/01_getting_started/04_async_await_primer.html). The idea is that either the programmer or the compiler inserts [yield](https://en.wikipedia.org/wiki/Yield_(multithreading)) operations into the program, which give up control of the CPU and allow other tasks to run. For example, a yield could be inserted after each iteration of a complex loop.

合作式多任务经常在语言层面上使用，比如以[coroutines](https://en.wikipedia.org/wiki/Coroutine)或[async/await](https://rust-lang.github.io/async-book/01_getting_started/04_async_await_primer.html)的形式。其原理是，程序员或编译器在程序中插入[yield](https://en.wikipedia.org/wiki/Yield_(multithreading))操作，放弃对CPU的控制，允许其他任务运行。例如，可以在一个复杂的循环的每个迭代之后插入一个yield操作。

It is common to combine cooperative multitasking with [asynchronous operations](https://en.wikipedia.org/wiki/Asynchronous_I/O). Instead of waiting until an operation is finished and preventing other tasks from running during this time, asynchronous operations return a “not ready” status if the operation is not finished yet. In this case, the waiting task can execute a yield operation to let other tasks run.

将合作式多任务与[异步操作](https://en.wikipedia.org/wiki/Asynchronous_I/O)结合起来是很常见的。与抢占式的一直等待直到操作完成并在期间阻止其他任务的执行，在协作式中若操作尚未完成，则异步操作将返回一个“未就绪”状态。在这种情况下，等待的任务可以执行一个yield操作以允许其他任务运行。

#### Saving State(状态保存)

Since tasks define their pause points themselves, they don’t need the operating system to save their state. Instead, they can save exactly the state they need for continuation before they pause themselves, which often results in better performance. For example, a task that just finished a complex computation might only need to backup the final result of the computation since it does not need the intermediate results anymore.

由于任务自己定义它们的暂停点，它们不需要操作系统来保存它们的状态。相反，它们可以在自己暂停之前准确地保存它们需要继续的状态，这往往会带来更好的性能。例如，一个刚刚完成复杂计算的任务可能只需要备份计算的最终结果，因为它不再需要中间的结果了。

Language-supported implementations of cooperative tasks are often even able to backup the required parts of the call stack before pausing. As an example, Rust’s async/await implementation stores all local variables that are still needed in an automatically generated struct (see below). By backing up the relevant parts of the call stack before pausing, all tasks can share a single call stack, which results in much lower memory consumption per task. This makes it possible to create an almost arbitrary number of cooperative tasks without running out of memory.

语言支持的合作任务的实现通常甚至能够在暂停前备份调用栈的必要部分。作为一个例子，Rust的async/await实现将所有仍然需要的局部变量存储在一个自动生成的结构中（见下文）。通过在暂停前备份调用栈的相关部分，所有的任务都可以共享一个调用栈，这使得每个任务的内存消耗大大降低。这使得创建几乎任意数量的合作任务而不耗尽内存成为可能。


#### Discussion(讨论)

The drawback of cooperative multitasking is that an uncooperative task can potentially run for an unlimited amount of time. Thus, a malicious or buggy task can prevent other tasks from running and slow down or even block the whole system. For this reason, cooperative multitasking should only be used when all tasks are known to cooperate. As a counterexample, it’s not a good idea to make the operating system rely on the cooperation of arbitrary user-level programs.

合作式多任务的缺点是，一个不合作的任务有可能运行无限长的时间。因此，一个恶意的或有错误的任务可以阻止其他任务的运行，使整个系统变慢甚至堵塞。出于这个原因，合作式多任务只应在所有任务都已知会合作的情况下使用。作为一个反例，让操作系统依赖任意的用户级程序的合作并不是一个好主意。

However, the strong performance and memory benefits of cooperative multitasking make it a good approach for usage within a program, especially in combination with asynchronous operations. Since an operating system kernel is a performance-critical program that interacts with asynchronous hardware, cooperative multitasking seems like a good approach for implementing concurrency.

然而，合作多任务的强大性能和内存优势使其成为程序内使用的好方法，特别是与异步操作相结合。由于操作系统内核是一个与异步硬件交互的性能关键程序，合作式多任务似乎是一个实现并发的好方法。

## Async/Await in Rust(Rust中的Async/Await)

The Rust language provides first-class support for cooperative multitasking in the form of async/await. Before we can explore what async/await is and how it works, we need to understand how futures and asynchronous programming work in Rust.

Rust语言以async/await的形式为合作式多任务提供了一流的支持。在我们探索什么是async/await以及它是如何工作的之前，我们需要了解futures和异步编程在Rust中是如何工作的。

### Futures

A future represents a value that might not be available yet. This could be, for example, an integer that is computed by another task or a file that is downloaded from the network. Instead of waiting until the value is available, futures make it possible to continue execution until the value is needed.

Future代表了一个可能还没有就绪的值。例如，这可能是一个由另一个任务计算的整数或一个从网络上下载的文件。Future并不需要一直要等待到该值可用，它可以继续执行其他代码直到需要用到该值为止。

#### Example(示例)

The concept of futures is best illustrated with a small example:

用一个小例子来说明futures的概念：

<img src="./img/async-example.svg">

This sequence diagram shows a `main` function that reads a file from the file system and then calls a function `foo`. This process is repeated two times: once with a synchronous `read_file` call and once with an asynchronous `async_read_file` call.

这个序列图显示了一个主函数，它从文件系统中读取一个文件，然后调用一个函数`foo`。这个过程重复了两次：一次是同步的`read_file`调用，一次是异步的`async_read_file`调用。

With the synchronous call, the `main` function needs to wait until the file is loaded from the file system. Only then can it call the `foo` function, which requires it to again wait for the result.

在同步调用中，主函数需要等待，直到文件从文件系统中加载。只有这样，它才能调用`foo`函数，这需要它再次等待结果。

With the asynchronous `async_read_file` call, the file system directly returns a future and loads the file asynchronously in the background. This allows the `main` function to call `foo` much earlier, which then runs in parallel with the file load. In this example, the file load even finishes before `foo` returns, so `main` can directly work with the file without further waiting after `foo` returns.


通过异步的`async_read_file`调用，文件系统直接返回一个future，并在后台异步加载文件。这使得主函数可以更早地调用`foo`，然后与文件加载并行运行。在这个例子中，文件加载甚至在`foo`返回之前就完成了，所以`main`可以直接处理文件，而无需在`foo`返回后继续等待。


#### Futures in Rust(Rust中的Futures)

In Rust, futures are represented by the [Future](https://doc.rust-lang.org/nightly/core/future/trait.Future.html) trait, which looks like this:

在Rust中，futures由[Future](https://doc.rust-lang.org/nightly/core/future/trait.Future.html) trait表示，其定义如下：

```rust
pub trait Future {
    type Output;
    fn poll(self: Pin<&mut Self>, cx: &mut Context) -> Poll<Self::Output>;
}
```

The [associated type](https://doc.rust-lang.org/book/ch19-03-advanced-traits.html#specifying-placeholder-types-in-trait-definitions-with-associated-types) `Output` specifies the type of the asynchronous value. For example, the `async_read_file` function in the diagram above would return a `Future` instance with `Output` set to `File`.

[关联类型](https://doc.rust-lang.org/book/ch19-03-advanced-traits.html#specifying-placeholder-types-in-trait-definitions-with-associated-types) `Output`指定异步返回值的类型。例如，上图中的`async_read_file`函数将返回一个`Output`设置为`File`的`Future`实例。

The [poll](https://doc.rust-lang.org/nightly/core/future/trait.Future.html#tymethod.poll) method allows to check if the value is already available. It returns a [Poll](https://doc.rust-lang.org/nightly/core/future/trait.Future.html#tymethod.poll) enum, which looks like this:

调用[poll](https://doc.rust-lang.org/nightly/core/future/trait.Future.html#tymethod.poll)方法可以检查该值是否已经可用。它将返回一个枚举[Poll](https://doc.rust-lang.org/nightly/core/future/trait.Future.html#tymethod.poll)，如下所示：

```rust
pub enum Poll<T> {
    Ready(T),
    Pending,
}
```


When the value is already available (e.g. the file was fully read from disk), it is returned wrapped in the `Ready` variant. Otherwise, the `Pending` variant is returned, which signals to the caller that the value is not yet available.

如果该值已经可用（例如，已从磁盘中读取完整的文件），则将其封装在`Ready`变量中返回。否则，将返回`Pending`变量，以通知调用方该值尚不可用。


The `poll` method takes two arguments: `self: Pin<&mut Self>` and `cx: &mut Context`. The former behaves similarly to a normal `&mut self` reference, except that the `Self` value is [pinned](https://doc.rust-lang.org/nightly/core/pin/index.html) to its memory location. Understanding Pin and why it is needed is difficult without understanding how async/await works first. We will therefore explain it later in this post.

`poll`方法采用两个参数：`self: Pin<&mut Self>`和`cx: &mut Context`上下文。前者的行为类似于普通的`&mut self`引用，不同之处在于，`Self`值[pinned](https://doc.rust-lang.org/nightly/core/pin/index.html)在一个固定的内存位置上。如果不先了解async/await的工作原理，就很难理解Pin以及为什么需要Pin。因此，我们将在后文中进行详细解释。

The purpose of the `cx: &mut Context` parameter is to pass a [Waker](https://doc.rust-lang.org/nightly/core/task/struct.Waker.html) instance to the asynchronous task, e.g., the file system load. This `Waker` allows the asynchronous task to signal that it (or a part of it) is finished, e.g., that the file was loaded from disk. Since the main task knows that it will be notified when the `Future` is ready, it does not need to call `poll` over and over again. We will explain this process in more detail later in this post when we implement our own waker type.

`cx: &mut Context`参数用于将一个[Waker](https://doc.rust-lang.org/nightly/core/task/struct.Waker.html)实例传递给异步任务，例如加载文件系统。这个`Waker`允许异步任务发信号通知任务（或部分任务）已完成，例如该文件已从磁盘加载。由于主任务知道将在`Future`就绪时将会收到通知，因此就不需要一遍又一遍地调用`poll`了。我们会在后文中实现自己的waker类型，届时将更加详细地说明此过程。

### Working with Futures(使用Futures)

We now know how futures are defined and understand the basic idea behind the poll method. However, we still don’t know how to effectively work with futures. The problem is that futures represent the results of asynchronous tasks, which might not be available yet. In practice, however, we often need these values directly for further calculations. So the question is: How can we efficiently retrieve the value of a future when we need it?

我们现在知道了futures是如何定义的，也理解了poll方法背后的基本理念。然而，我们仍然不知道如何有效地使用futures。问题是，futures代表了异步任务的结果，而这些结果可能还未可用。然而，在实践中，我们经常需要这些值直接用于进一步的计算。因此，问题是：当我们需要时，我们如何有效地检索一个future的值？

#### Waiting on Futures(等待Futures的值)

One possible answer is to wait until a future becomes ready. This could look something like this:

一种可能的答案是一直等待到future就绪为止。该过程看起来像这样：

```rust
let future = async_read_file("foo.txt");
let file_content = loop {
    match future.poll(…) {
        Poll::Ready(value) => break value,
        Poll::Pending => {}, // do nothing
    }
}
```

Here we actively wait for the future by calling `poll` over and over again in a loop. The arguments to `poll` don’t matter here, so we omitted them. While this solution works, it is very inefficient because we keep the CPU busy until the value becomes available.

在上面的代码中，我们通过循环中积极地调用`poll`来等待future完成。在这里`poll`的参数并不重要，因此已被省略。尽管此方法可行，但是效率很低，因为包含`poll`的循环会一直占用CPU直到该值可用为止。

A more efficient approach could be to block the current thread until the future becomes available. This is, of course, only possible if you have threads, so this solution does not work for our kernel, at least not yet. Even on systems where blocking is supported, it is often not desired because it turns an asynchronous task into a synchronous task again, thereby inhibiting the potential performance benefits of parallel tasks.

一种较为高效的方法是阻塞当前线程，直到future可用为止。当然，这只有在有线程支持的情况下才可行，因此并不适用于我们的内核，至少目前还不行。即使在支持阻塞的系统上，通常也不会这么做，因为阻塞会将异步任务再次变为同步任务，从而失去了并行任务的性能优势。

#### Future Combinators(Future组合器)

An alternative to waiting is to use future combinators. Future combinators are methods like `map` that allow chaining and combining futures together, similar to the methods of the [Iterator](https://doc.rust-lang.org/stable/core/iter/trait.Iterator.html) trait. Instead of waiting on the future, these combinators return a future themselves, which applies the mapping operation on `poll`.

另一种等待方式是使用future组合器。Future组合器是类似于`map`的方法，它允许将futures链接并组合在一起，就像在[Iterator](https://doc.rust-lang.org/stable/core/iter/trait.Iterator.html)上做的那样。这些组合器不等待future，而是自己返回future，即为`poll`应用了map操作。

As an example, a simple `string_len` combinator for converting a `Future<Output = String>` to a `Future<Output = usize>` could look like this:

```rust
struct StringLen<F> {
    inner_future: F,
}

impl<F> Future for StringLen<F> where F: Future<Output = String> {
    type Output = usize;

    fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<T> {
        match self.inner_future.poll(cx) {
            Poll::Ready(s) => Poll::Ready(s.len()),
            Poll::Pending => Poll::Pending,
        }
    }
}

fn string_len(string: impl Future<Output = String>)
    -> impl Future<Output = usize>
{
    StringLen {
        inner_future: string,
    }
}

// Usage
fn file_len() -> impl Future<Output = usize> {
    let file_content_future = async_read_file("foo.txt");
    string_len(file_content_future)
}
```

This code does not quite work because it does not handle [pinning](https://doc.rust-lang.org/stable/core/pin/index.html), but it suffices as an example. The basic idea is that the `string_len` function wraps a given `Future` instance into a new `StringLen` struct, which also implements `Future`. When the wrapped future is polled, it polls the inner future. If the value is not ready yet, `Poll::Pending` is returned from the wrapped future too. If the value is ready, the string is extracted from the `Poll::Ready` variant and its length is calculated. Afterwards, it is wrapped in `Poll::Ready` again and returned.

该代码无法正常工作，因为尚未处理[pinning](https://doc.rust-lang.org/stable/core/pin/index.html)，不过作为例子已经足够了。基本思路是`string_len`函数将给定的实现了`Future`trait的实例封装到新的`StringLen`结构体中，而该结构体也实现了`Future`trait。当poll封装的future时，即是poll其内部的future。如果该值尚未就绪，封装的future也将返回`Poll::Pending`。如果该值已就绪，则从`Poll::Ready`变量中获取字符串，并计算其长度。最后再将其封装在`Poll::Ready`中返回。


With this `string_len` function, we can calculate the length of an asynchronous string without waiting for it. Since the function returns a `Future` again, the caller can’t work directly on the returned value, but needs to use combinator functions again. This way, the whole call graph becomes asynchronous and we can efficiently wait for multiple futures at once at some point, e.g., in the main function.

通过`string_len`函数，我们不需要等待一个异步字符串，就可以计算其长度。由于该函数也返回Future，因此调用者无法直接操作返回的值，而需要再次使用组合器函数。这样，整个调用过程就变为异步的了，我们可以高效地在某个时刻一次等待多个future，例如 在main函数上。

Because manually writing combinator functions is difficult, they are often provided by libraries. While the Rust standard library itself provides no combinator methods yet, the semi-official (and `no_std` compatible) [futures](https://docs.rs/futures/0.3.4/futures/) crate does. Its [FutureExt](https://docs.rs/futures/0.3.4/futures/future/trait.FutureExt.html) trait provides high-level combinator methods such as [map](https://docs.rs/futures/0.3.4/futures/future/trait.FutureExt.html#method.map) or [then](https://docs.rs/futures/0.3.4/futures/future/trait.FutureExt.html#method.then), which can be used to manipulate the result with arbitrary closures.

手动编写组合器函数比较困难，所以它们通常由库直接提供。尽管Rust标准库本身还没有提供官方组合器方法，但半官方（兼容`no_std`）的[futures](https://docs.rs/futures/0.3.4/futures/) crate可以。其F[FutureExt](https://docs.rs/futures/0.3.4/futures/future/trait.FutureExt.html) 特性提供了诸如[map](https://docs.rs/futures/0.3.4/futures/future/trait.FutureExt.html#method.map)或[then](https://docs.rs/futures/0.3.4/futures/future/trait.FutureExt.html#method.then)之类的高级组合器方法，可用于任意闭包来操作结果。

#### Advantages(优势)

The big advantage of future combinators is that they keep the operations asynchronous. In combination with asynchronous I/O interfaces, this approach can lead to very high performance. The fact that future combinators are implemented as normal structs with trait implementations allows the compiler to excessively optimize them. For more details, see the [Zero-cost futures in Rust](https://aturon.github.io/blog/2016/08/11/futures/)  post, which announced the addition of futures to the Rust ecosystem.

Future组合器最大的优点是能够使操作保持异步。这种方法和异步I/O接口结合使用时性能非常高。实际上future组合器将被实现为具有trait的普通结构体，以使编译器能够对其做出进一步优化。有关更多详细信息，请参阅[Rust的零成本future](https://aturon.github.io/blog/2016/08/11/futures/)一文，就是这篇文章宣布了在Rust生态系统中添加future。

#### Drawbacks(缺点)

While future combinators make it possible to write very efficient code, they can be difficult to use in some situations because of the type system and the closure-based interface. For example, consider code like this:

尽管future组合器可以编写出非常高效的代码，但由于类型系统和基于闭包的接口的限制，组合器可能会在某些情况下变得难以使用。例如，考虑下面的代码：

```rust
fn example(min_len: usize) -> impl Future<Output = String> {
    async_read_file("foo.txt").then(move |content| {
        if content.len() < min_len {
            Either::Left(async_read_file("bar.txt").map(|s| content + &s))
        } else {
            Either::Right(future::ready(content))
        }
    })
}
```
[(Try it on the playground)](https://play.rust-lang.org/?version=stable&mode=debug&edition=2018&gist=91fc09024eecb2448a85a7ef6a97b8d8)

Here we read the file `foo.txt` and `then` use the then combinator to chain a second future based on the file content. If the content length is smaller than the given `min_len`, we read a different `bar.txt` file and append it to content using the map combinator. Otherwise, we return only the content of `foo.txt`.

代码先读取文件`foo.txt`，然后使用`then`组合器根据文件内容链接第二个future。如果内容长度小于给定的`min_len`，我们将读取另一个`bar.txt`文件，然后使用map组合器将其附加到content后。否则，只返回`foo.txt`的content。

We need to use the [move keyword](https://doc.rust-lang.org/std/keyword.move.html) for the closure passed to `then` because otherwise there would be a lifetime error for `min_len`. The reason for the [Either](https://docs.rs/futures/0.3.4/futures/future/enum.Either.html) wrapper is that `if` and `else` blocks must always have the same type. Since we return different future types in the blocks, we must use the wrapper type to unify them into a single type. The [ready](https://docs.rs/futures/0.3.4/futures/future/fn.ready.html) function wraps a value into a future, which is immediately ready. The function is required here because the `Either` wrapper expects that the wrapped value implements `Future`.

我们需要在传递给`then`的闭包上使用[move关键字]((https://doc.rust-lang.org/std/keyword.move.html))，否则`min_len`变量会产生生命周期错误。使用[Either](https://docs.rs/futures/0.3.4/futures/future/enum.Either.html)封装的原因是让`if`块和`else`块始终具有相同的类型。由于我们在块中返回了不同的future类型，因此必须使用封装类型将它们统一为一个类型。[ready](https://docs.rs/futures/0.3.4/futures/future/fn.ready.html)函数将立刻就绪的值封装到future中。这里需要使用该函数是因为`Either`封装要求值实现`Future`trait。

As you can imagine, this can quickly lead to very complex code for larger projects. It gets especially complicated if borrowing and different lifetimes are involved. For this reason, a lot of work was invested in adding support for async/await to Rust, with the goal of making asynchronous code radically simpler to write.

您可以想象，这种用法很快就会导致大型项目的代码变得非常复杂。尤其是再涉及借用和生命周期，就会变得更加复杂。因此，为了使异步代码从根本上更易于编写，大量的工作用来为Rust添加对async/await的支持。

### The Async/Await Pattern(Async/Await模式)

The idea behind async/await is to let the programmer write code that looks like normal synchronous code, but is turned into asynchronous code by the compiler. It works based on the two keywords `async` and `await`. The `async` keyword can be used in a function signature to turn a synchronous function into an asynchronous function that returns a future:

Async/Await的思路是让程序员以编写看起来像同步代码的方式编写异步代码，只不最后是由编译器将同步代码转换为异步代码。它基于两个关键字`async`和`await`。在函数签名中使用`async`关键字，就可以将同步函数转换为一个返回future的异步函数：

```rust
async fn foo() -> u32 {
    0
}

// the above is roughly translated by the compiler to:
fn foo() -> impl Future<Output = u32> {
    future::ready(0)
}
```

This keyword alone wouldn’t be that useful. However, inside `async` functions, the `await` keyword can be used to retrieve the asynchronous value of a future:

仅使用此关键字并没有那么有用。但是，在异步函数内部，可以使用`await`关键字来取回future的异步值：

```rust
async fn example(min_len: usize) -> String {
    let content = async_read_file("foo.txt").await;
    if content.len() < min_len {
        content + &async_read_file("bar.txt").await
    } else {
        content
    }
}
```

[(Try it on the playground)](https://play.rust-lang.org/?version=stable&mode=debug&edition=2018&gist=d93c28509a1c67661f31ff820281d434)

This function is a direct translation of the `example` function from above that used combinator functions. Using the `.await` operator, we can retrieve the value of a future without needing any closures or `Either` types. As a result, we can write our code like we write normal synchronous code, with the difference that this is still asynchronous code.

将上面使用组合器实现的example函数直接转换为async/await模式：使用`.await`运算符就可以取回future的值，无需使用闭包或`Either`类型。如此，我们就可以像编写普通的同步代码一样编写异步代码。

#### State Machine Transformation(状态转换机)

Behind the scenes, the compiler converts the body of the `async` function into a [state machine](https://en.wikipedia.org/wiki/Finite-state_machine), with each `.await` call representing a different state. For the above `example` function, the compiler creates a state machine with the following four states:

在这种场景中，编译器的作用就是将`async`函数体转换为一个[状态机](https://en.wikipedia.org/wiki/Finite-state_machine)，每个`.await`调用代表一个不同的状态。对于上面的`example`函数，编译器创建具有以下四个状态的状态机：

<img src="./img/async-state-machine-states.svg">


Each state represents a different pause point in the function. The “Start” and “End” states represent the function at the beginning and end of its execution. The “Waiting on foo.txt” state represents that the function is currently waiting for the first `async_read_file` result. Similarly, the “Waiting on bar.txt” state represents the pause point where the function is waiting on the second `async_read_file` result.

不同状态代表该函数的不同暂停点。"Start"和"End"状态代表函数在其执行的开始和结束时的状态。"Waiting on foo.txt"状态表示该函数目前正在等待第一个`async_read_file`的结果。同样的，"Waiting on bar.txt"状态表示函数在等待第二个`async_read_file`的结果的暂停点。

The state machine implements the `Future` trait by making each `poll` call a possible state transition:

状态机通过将每个`poll`调用都变为一个可能的状态转换来实现`Future`trait：

<img src="./img/async-state-machine-basic.svg">

The diagram uses arrows to represent state switches and diamond shapes to represent alternative ways. For example, if the `foo.txt` file is not ready, the path marked with “no” is taken and the “Waiting on foo.txt” state is reached. Otherwise, the “yes” path is taken. The small red diamond without a caption represents the `if content.len() < 100` branch of the `example` function.

图中使用箭头表示状态开关，并使用菱形表示条件路径。例如，如果`foo.txt`文件尚未准备好，则采用标记为"no"的路径，并达到"Waiting on foo.txt"的状态。否则，就采用标记为"yes"路径。没有字的红色小菱形代表`example`函数中`if content.len() < 100`的条件分支。


We see that the first `poll` call starts the function and lets it run until it reaches a future that is not ready yet. If all futures on the path are ready, the function can run till the “End” state, where it returns its result wrapped in `Poll::Ready`. Otherwise, the state machine enters a waiting state and returns `Poll::Pending`. On the next `poll` call, the state machine then starts from the last waiting state and retries the last operation.

我们看到第一个`poll`调用启动了该函数并使它运行，直到遇到一个尚未就绪的future。如果路径上的所有future都已就绪，则该函数可以一直运行到”End“状态，并返回封装在`Poll::Ready`中的结果。否则，状态机将进入等待状态并返回`Poll::Pending`。然后在下一个`poll`调用中，状态机从上一个等待状态开始重试其最后一次操作。

#### Saving State(保存状态)

In order to be able to continue from the last waiting state, the state machine must keep track of the current state internally. In addition, it must save all the variables that it needs to continue execution on the next `poll` call. This is where the compiler can really shine: Since it knows which variables are used when, it can automatically generate structs with exactly the variables that are needed.

为了能够从上一个等待状态中恢复，状态机必须在内部跟踪当前状态。此外，它还必须保存在下一个`poll`调用中恢复执行所需的变量。这就是编译器真正发挥作用的地方：由于编译器知道在何时要使用哪些变量，因此它可以自动生成具有所需变量的结构体。

As an example, the compiler generates structs like the following for the above `example` function:

作为示例，编译器为上面的`example`函数生成类似于下面这样的结构体：

```rust
// The `example` function again so that you don't have to scroll up
async fn example(min_len: usize) -> String {
    let content = async_read_file("foo.txt").await;
    if content.len() < min_len {
        content + &async_read_file("bar.txt").await
    } else {
        content
    }
}

// The compiler-generated state structs:

struct StartState {
    min_len: usize,
}

struct WaitingOnFooTxtState {
    min_len: usize,
    foo_txt_future: impl Future<Output = String>,
}

struct WaitingOnBarTxtState {
    content: String,
    bar_txt_future: impl Future<Output = String>,
}

struct EndState {}
```

In the “start” and “Waiting on foo.txt” states, the `min_len` parameter needs to be stored for the later comparison with `content.len()`. The “Waiting on foo.txt” state additionally stores a `foo_txt_future`, which represents the future returned by the `async_read_file` call. This future needs to be polled again when the state machine continues, so it needs to be saved.


在"Start"和"Waiting on foo.txt"状态下，需要存储`min_len`参数，因为稍后与`content.len()`做比较时需要使用该参数。"Waiting on foo.txt"状态还存储了一个`foo_txt_future`，用来表示`async_read_file`调用返回的future。状态机继续运行时会再次poll该future，因此需要将其保存。


The “Waiting on bar.txt” state contains the `content` variable for the later string concatenation when `bar.txt` is ready. It also stores a `bar_txt_future` that represents the in-progress load of `bar.txt`. The struct does not contain the `min_len` variable because it is no longer needed after the `content.len()` comparison. In the “end” state, no variables are stored because the function has already run to completion.

"Waiting on bar.txt"状态包含`content`变量，是因为在`bar.txt`就绪后需要使用该变量进行字符串连接。该状态还存储了一个`bar_txt_future`，用来表示正在加载中的`bar.txt`。该结构体不包含`min_len`变量，因为在c`ontent.len()`比较之后就不再需要该变量了。在"End"状态下，没有存储任何变量，因为此时函数已经运行完毕。

Keep in mind that this is only an example of the code that the compiler could generate. The struct names and the field layout are implementation details and might be different.

请记住，这只是编译器可能生成的代码的一个示例。结构体名称和字段布局是实现细节，可能会有所不同。

#### The Full State Machine Type(完整的状态机类型)

While the exact compiler-generated code is an implementation detail, it helps in understanding to imagine how the generated state machine could look for the `example` function. We already defined the structs representing the different states and containing the required variables. To create a state machine on top of them, we can combine them into an `enum`:

尽管编译器生成的确切代码是实现细节，但这个示例还是有助于我们理解并想象`example`函数生成的状态机可能的样子。我们已经定义了代表不同状态的结构体，并给出了其中包含的所需变量。为了基于这些结构体创建一个状态机，我们可以将它们组合成一个`枚举`：

```rust
enum ExampleStateMachine {
    Start(StartState),
    WaitingOnFooTxt(WaitingOnFooTxtState),
    WaitingOnBarTxt(WaitingOnBarTxtState),
    End(EndState),
}
```

We define a separate enum variant for each state and add the corresponding state struct to each variant as a field. To implement the state transitions, the compiler generates an implementation of the `Future` trait based on the `example` function:

我们为每个状态定义一个单独的枚举变量，并将对应状态的结构体作为字段添加到每个变量。为了实现状态转换，编译器根据`example`函数实现`Future`trait：

```rust
impl Future for ExampleStateMachine {
    type Output = String; // return type of `example`

    fn poll(self: Pin<&mut Self>, cx: &mut Context) -> Poll<Self::Output> {
        loop {
            match self { // TODO: handle pinning
                ExampleStateMachine::Start(state) => {…}
                ExampleStateMachine::WaitingOnFooTxt(state) => {…}
                ExampleStateMachine::WaitingOnBarTxt(state) => {…}
                ExampleStateMachine::End(state) => {…}
            }
        }
    }
}
```

The `Output` type of the future is `String` because it’s the return type of the `example` function. To implement the `poll` function, we use a `match` statement on the current state inside a `loop`. The idea is that we switch to the next state as long as possible and use an explicit `return Poll::Pending` when we can’t continue.

该Future的`Output`类型为`String`，即`example`函数的返回类型。为了实现`poll`函数，我们在`loop`内的当前状态上使用`match`语句。思路是我们尽可能长时间地切换到下一个状态，并在无法继续时显式的使用`return Poll::Pending`。

For simplicity, we only show simplified code and don’t handle [pinning](https://doc.rust-lang.org/stable/core/pin/index.html), ownership, lifetimes, etc. So this and the following code should be treated as pseudo-code and not used directly. Of course, the real compiler-generated code handles everything correctly, albeit possibly in a different way.

为简单起见，这里仅给出简化的代码，且暂不处理[pinning](https://doc.rust-lang.org/stable/core/pin/index.html)、所有权、生命周期等内容。因此，这里的代码和下面的代码应被看做伪代码，不能直接使用。当然，真正的编译器生成的代码可以正确处理所有内容，尽管可能使用了与我们不同的方式。

To keep the code excerpts small, we present the code for each `match` arm separately. Let’s begin with the `Start` state:

为了使示意的代码更简洁，我们将分别显示每个匹配分支的代码。从”Start”状态开始：

```rust
ExampleStateMachine::Start(state) => {
    // from body of `example`
    let foo_txt_future = async_read_file("foo.txt");
    // `.await` operation
    let state = WaitingOnFooTxtState {
        min_len: state.min_len,
        foo_txt_future,
    };
    *self = ExampleStateMachine::WaitingOnFooTxt(state);
}
```

The state machine is in the `Start` state when it is right at the beginning of the function. In this case, we execute all the code from the body of the `example` function until the first `.await`. To handle the `.await` operation, we change the state of the `self` state machine to `WaitingOnFooTxt`, which includes the construction of the `WaitingOnFooTxtState` struct.

当状态机处于`Start`状态时，其对应位置正是函数体的最开始。在这种情况下，我们将执行`example`函数体中的所有代码，直到遇到第一个`.await`。为了处理`.await`操作，我们将`self`状态机的状态修改为`WaitingOnFooTxt`，并令状态中包含`WaitingOnFooTxtState`结构体。

Since the `match self {…}` statement is executed in a loop, the execution jumps to the `WaitingOnFooTxt` arm next:

由于`match self {…}`语句是在循环中执行的，因此该执行将跳至下一个分支`WaitingOnFooTxt`：

```rust
ExampleStateMachine::WaitingOnFooTxt(state) => {
    match state.foo_txt_future.poll(cx) {
        Poll::Pending => return Poll::Pending,
        Poll::Ready(content) => {
            // from body of `example`
            if content.len() < state.min_len {
                let bar_txt_future = async_read_file("bar.txt");
                // `.await` operation
                let state = WaitingOnBarTxtState {
                    content,
                    bar_txt_future,
                };
                *self = ExampleStateMachine::WaitingOnBarTxt(state);
            } else {
                *self = ExampleStateMachine::End(EndState);
                return Poll::Ready(content);
            }
        }
    }
}
```

In this `match` arm, we first call the `poll` function of the `foo_txt_future`. If it is not ready, we exit the loop and return `Poll::Pending`. Since `self` stays in the `WaitingOnFooTxt` state in this case, the next poll call on the state machine will enter the same `match` arm and retry polling the `foo_txt_future`.

在这一匹配分支中，我们首先调用`foo_txt_future`的`poll`函数。如果尚未就绪，则退出循环并返回`Poll::Pending`。由于在这种情况下`self`仍位于`WaitingOnFooTxt`状态，因此状态机的下一次`poll`调用也将进入相同的匹配分支并重试`foo_txt_future`。

When the `foo_txt_future` is ready, we assign the result to the `content` variable and continue to execute the code of the `example` function: If `content.len()` is smaller than the `min_len` saved in the state struct, the `bar.txt` file is read asynchronously. We again translate the `.await` operation into a state change, this time into the `WaitingOnBarTxt` state. Since we’re executing the `match` inside a loop, the execution directly jumps to the `match` arm for the new state afterward, where the `bar_txt_future` is polled.

当`foo_txt_future`就绪时，我们将结果赋给`content`变量，然后继续执行`example`函数的代码：如果`content.len()`小于状态结构体中保存的`min_len`，则异步读取`bar.txt`文件。我们再次将`.await`操作转换为状态更改，而这次应转换为W`aitingOnBarTxt`状态。由于我们是在循环内执行匹配，因此下一轮循环将直接跳转到新状态的匹配分支，然后在该状态下poll `bar_txt_future`。

In case we enter the `else` branch, no further `.await` operation occurs. We reach the end of the function and return `content` wrapped in `Poll::Ready`. We also change the current state to the `End` state.

如果我们进入`else`分支，则不会进行进一步的`.await`操作。此时已到达函数的结尾，并将`content`封装在`Poll::Ready`中返回。我们还需要将当前状态更改为`End`状态。

The code for the `WaitingOnBarTxt` state looks like this:

`WaitingOnBarTxt`状态的代码如下所示：

```rust
ExampleStateMachine::WaitingOnBarTxt(state) => {
    match state.bar_txt_future.poll(cx) {
        Poll::Pending => return Poll::Pending,
        Poll::Ready(bar_txt) => {
            *self = ExampleStateMachine::End(EndState);
            // from body of `example`
            return Poll::Ready(state.content + &bar_txt);
        }
    }
}
```

Similar to the `WaitingOnFooTxt` state, we start by polling the `bar_txt_future`. If it is still pending, we exit the loop and return `Poll::Pending`. Otherwise, we can perform the last operation of the `example` function: concatenating the `content` variable with the result from the `future`. We update the state machine to the End state and then return the result wrapped in `Poll::Ready`.

与`WaitingOnFooTxt`状态类似，我们从poll `bar_txt_future`开始。如果仍未就绪，则退出循环并返回P`oll::Pending`。否则，我们就执行`example`函数的最后一个操作：用`content`变量与`future`的结果做字符串连接。我们将状态机更新为`End`状态，然后将结果封装在`Poll::Ready`中返回。

Finally, the code for the `End` state looks like this:

最后，`End`状态的代码如下所示：

```rust
ExampleStateMachine::End(_) => {
    panic!("poll called after Poll::Ready was returned");
}
```

Futures should not be polled again after they returned `Poll::Ready`, so we panic if `poll` is called while we are already in the `End` state.

Future返回`Poll::Ready`后就不应再被poll了，因此，当我们已经处于`End`状态时，如果再次调用`poll`，就产生一个panic。

We now know what the compiler-generated state machine and its implementation of the `Future` trait could look like. In practice, the compiler generates code in a different way. (In case you’re interested, the implementation is currently based on [generators](https://doc.rust-lang.org/nightly/unstable-book/language-features/generators.html), but this is only an implementation detail.)

现在我们知道了编译器可能会生成怎样的状态机，以及怎样去给状态机实现`Future` trait。但实际上，编译器会以不同的方式生成代码。（如果您感兴趣的话，该实现目前基于[生成器](https://doc.rust-lang.org/nightly/unstable-book/language-features/generators.html)，不过这只是实现细节。）

The last piece of the puzzle is the generated code for the `example` function itself. Remember, the function header was defined like this:

最后一步是为`example`函数本身生成代码。记住，函数签名是这样定义的：

```rust
async fn example(min_len: usize) -> String
```

Since the complete function body is now implemented by the state machine, the only thing that the function needs to do is to initialize the state machine and return it. The generated code for this could look like this:

由于现在整个函数体是由状态机实现的，因此该函数唯一需要做的就是初始化状态机并将其返回。为此生成的代码如下所示：

```rust
fn example(min_len: usize) -> ExampleStateMachine {
    ExampleStateMachine::Start(StartState {
        min_len,
    })
}
```

The function no longer has an `async` modifier since it now explicitly returns an `ExampleStateMachine` type, which implements the `Future` trait. As expected, the state machine is constructed in the `Start` state and the corresponding state struct is initialized with the `min_len` parameter.

该函数不再使用`async`修饰符，因为它现在显式返回一个实现了`Future` trait的`ExampleStateMachine`类型。如预期的那样，状态机被初始化为`Start`状态，并且使用`min_len`参数初始化了对应的状态结构体。

Note that this function does not start the execution of the state machine. This is a fundamental design decision of futures in Rust: they do nothing until they are polled for the first time.

请注意，此函数并不会直接启动状态机。这是Rust中future的一个基本设计决策：在第一次被poll之前什么也不做。