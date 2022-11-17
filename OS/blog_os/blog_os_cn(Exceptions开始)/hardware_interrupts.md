<h1>Hardware Interrupts (硬件中断)</h1>

In this post, we set up the programmable interrupt controller to correctly forward hardware interrupts to the CPU. To handle these interrupts, we add new entries to our interrupt descriptor table, just like we did for our exception handlers. We will learn how to get periodic timer interrupts and how to get input from the keyboard.

这篇文章中，我们将会设置一个可编程中断控制器，以此正确的将硬件中断转发给CPU。为了处理这些中断，我们也会向中断描述符表中添加一些新的表项，就和我们之前写的异常处理程序一样。我们也将学习如何获取周期性运作的计时器中断以及如何从键盘获取输入。

This blog is openly developed on GitHub. If you have any problems or questions, please open an issue there. You can also leave comments at the bottom. The complete source code for this post can be found in the post-07 branch.

这篇博客是在[GitHub](https://github.com/phil-opp/blog_os)上公开开发的。如果你有任何问题或疑问，请在那里开一个问题。你也可以在底部留下评论。
本帖的完整源代码可以在[post-07](https://github.com/phil-opp/blog_os/tree/post-07)分支中找到。

<h2>Overview(概述)</h2>

Interrupts provide a way to notify the CPU from attached hardware devices. So instead of letting the kernel periodically check the keyboard for new characters (a process called [polling](https://en.wikipedia.org/wiki/Polling_(computer_science))), the keyboard can notify the kernel of each keypress. This is much more efficient because the kernel only needs to act when something happened. It also allows faster reaction times since the kernel can react immediately and not only at the next poll.

中断为连接到CPU上的硬件设置提供了一种通知CPU的方法。这样CPU不必定期检查键盘是否有新的字符输入(一种称为[轮询](https://en.wikipedia.org/wiki/Polling_(computer_science)的过程),而是键盘将每次按键事件通知给内核。这样会很高效，因为内核只需要在事件发生的时候去做反应就可以了。这样也能大大缩短事件的反应事件，因为这样内核可以立刻反应而不是需要等待下次轮询的时候。

Connecting all hardware devices directly to the CPU is not possible. Instead, a separate interrupt controller aggregates the interrupts from all devices and then notifies the CPU:

将所有的硬件设备直接连接到CPU是不可行的。因此，一个独立的中断控制器用来汇总设备的中断事件，再通知CPU:


```
                                    ____________             _____
               Timer ------------> |            |           |     |
               Keyboard ---------> | Interrupt  |---------> | CPU |
               Other Hardware ---> | Controller |           |_____|
               Etc. -------------> |____________|
```

Most interrupt controllers are programmable, which means they support different priority levels for interrupts. For example, this allows to give timer interrupts a higher priority than keyboard interrupts to ensure accurate timekeeping.

大部分的中断控制器是可编程的,这就代表他们可以支持为中断设置优先级。举个例子,允许将计时器设置为比键盘更高的优先级以保证计时的准确性。

Unlike exceptions, hardware interrupts occur asynchronously. This means they are completely independent from the executed code and can occur at any time. Thus, we suddenly have a form of concurrency in our kernel with all the potential concurrency-related bugs. Rust’s strict ownership model helps us here because it forbids mutable global state. However, deadlocks are still possible, as we will see later in this post.

和异常不一样的是，硬件中断时异步发生的。这就代表他们和当前执行的代码完全无关并且随时可能发生。因此，我们内核中忽然出现了并发的情况并且出现了潜在的并发bug。好在Rust严格的所有权模型帮助在这里帮了大忙，因为他禁止了全局可变状态。然而，死锁还是可能发生，我们可以在后文中看到。

<h2>The 8259 PIC (8259 可编程中断控制器)</h2>

The [Intel 8259](https://en.wikipedia.org/wiki/Intel_8259) is a programmable interrupt controller (PIC) introduced in 1976. It has long been replaced by the newer [APIC](https://en.wikipedia.org/wiki/Intel_APIC_Architecture), but its interface is still supported on current systems for backwards compatibility reasons. The 8259 PIC is significantly easier to set up than the APIC, so we will use it to introduce ourselves to interrupts before we switch to the APIC in a later post.

英特尔的[8259](https://en.wikipedia.org/wiki/Intel_8259)是1976年推出的可编程中断控制器。他虽然很早就被更新的[APIC](https://en.wikipedia.org/wiki/Intel_APIC_Architecture)替代了，但是因为向后兼容的原因，现在的系统中仍然支持他的接口。8259 PIC的配置比APIC要容易得多，因此在后续文章中切换到APIC之前，我们仍将使用8529 PIC来引入中断。

The 8259 has eight interrupt lines and several lines for communicating with the CPU. The typical systems back then were equipped with two instances of the 8259 PIC, one primary and one secondary PIC, connected to one of the interrupt lines of the primary:

8259有8条中断线和几条用于与CPU通讯的线。当年的典型系统会配备两个8259 PIC实例，一个主控制器和一个通过中断线连接在主控上的从控制器：


```                     ____________                          ____________
Real Time Clock --> |            |   Timer -------------> |            |
ACPI -------------> |            |   Keyboard-----------> |            |      _____
Available --------> | Secondary  |----------------------> | Primary    |     |     |
Available --------> | Interrupt  |   Serial Port 2 -----> | Interrupt  |---> | CPU |
Mouse ------------> | Controller |   Serial Port 1 -----> | Controller |     |_____|
Co-Processor -----> |            |   Parallel Port 2/3 -> |            |
Primary ATA ------> |            |   Floppy disk -------> |            |
Secondary ATA ----> |____________|   Parallel Port 1----> |____________|

```

This graphic shows the typical assignment of interrupt lines. We see that most of the 15 lines have a fixed mapping, e.g., line 4 of the secondary PIC is assigned to the mouse.

上图显示了一个典型的中断线分配布局。可以看到15条线中的大多数都有固定的映射，例如 次PIC的4号线分配给了鼠标。(主上的Timer是序号0)

Each controller can be configured through two [I/O ports](https://os.phil-opp.com/testing/#i-o-ports), one “command” port and one “data” port. For the primary controller, these ports are `0x20` (command) and `0x21` (data). For the secondary controller, they are `0xa0` (command) and `0xa1` (data). For more information on how the PICs can be configured, see the [article on osdev.org](https://wiki.osdev.org/8259_PIC).

每个控制器可以通过两个[I/O端口](https://os.phil-opp.com/testing/#i-o-ports)——一个“命令”端口和一个“数据”端口——进行配置。对于主控制器，这些端口是`0x20`（命令）和`0x21`（数据）。对于从控制器，它们是`0xa0`（命令）和`0xa1`（数据）。有关如何配置PIC的更多信息，请参见[osdev.org上的文章](https://wiki.osdev.org/8259_PIC)。


<h3>Implementation(实现)</h3>

The default configuration of the PICs is not usable because it sends interrupt vector numbers in the range of 0–15 to the CPU. These numbers are already occupied by CPU exceptions. For example, number 8 corresponds to a double fault. To fix this overlapping issue, we need to remap the PIC interrupts to different numbers. The actual range doesn’t matter as long as it does not overlap with the exceptions, but typically the range of 32–47 is chosen, because these are the first free numbers after the 32 exception slots.

PIC的默认配置不可用，因为它会将范围为0到15的中断向量编号发送到CPU。而这些编号已被CPU异常占用，例如，编号8对应双重故障。为了解决这个占用问题，我们需要将PIC中断重新映射到不同的编号。实际范围并不重要，只要它不与已存在的异常重叠即可，但是我们通常会选择编号32到47，因为这些是32个异常占用后的第一个段空闲数字。

The configuration happens by writing special values to the command and data ports of the PICs. Fortunately, there is already a crate called [pic8259](https://docs.rs/pic8259/0.10.1/pic8259/), so we don’t need to write the initialization sequence ourselves. However, if you are interested in how it works, check out [its source code](https://docs.rs/crate/pic8259/0.10.1/source/src/lib.rs). It’s fairly small and well documented.

我们可以通过向PIC的命令和数据端口写入特殊值来使配置生效。幸运的是，已经有一个名为[pic8259_simple](https://docs.rs/pic8259/0.10.1/pic8259/)的crate，因此我们不需要自己编写初始化过程。如果你对它的工作方式感兴趣，请查看它的[源代码](https://docs.rs/crate/pic8259/0.10.1/source/src/lib.rs)，该crate很小并且文档齐全。

To add the crate as a dependency, we add the following to our project:

要将crate添加为依赖，我们需要将以下内容添加到项目中：

```toml
# in Cargo.toml

[dependencies]
pic8259 = "0.10.1"
```


The main abstraction provided by the crate is the [ChainedPics](https://docs.rs/pic8259/0.10.1/pic8259/struct.ChainedPics.html) struct that represents the primary/secondary PIC layout we saw above. It is designed to be used in the following way:

该crate提供的主要抽象为结构体[ChainedPics](https://docs.rs/pic8259/0.10.1/pic8259/struct.ChainedPics.html)。该结构体代表了我们在上面介绍的主/次PIC布局。它的用法如下：

```rust
// in src/interrupts.rs

use pic8259::ChainedPics;
use spin;

pub const PIC_1_OFFSET: u8 = 32;
pub const PIC_2_OFFSET: u8 = PIC_1_OFFSET + 8;

pub static PICS: spin::Mutex<ChainedPics> =
    spin::Mutex::new(unsafe { ChainedPics::new(PIC_1_OFFSET, PIC_2_OFFSET) });
```

As noted above, we’re setting the offsets for the PICs to the range 32–47. By wrapping the `ChainedPics` struct in a `Mutex`, we can get safe mutable access (through the [lock method](https://docs.rs/spin/0.5.2/spin/struct.Mutex.html#method.lock)), which we need in the next step. The `ChainedPics::new` function is unsafe because wrong offsets could cause undefined behavior.

像上面这样将PIC的偏移量设置为32-47。通过将`ChainedPics`结构体放置于`Mutex`中，我们就能够（通过[lock](https://docs.rs/spin/0.5.2/spin/struct.Mutex.html#method.lock)方法）获得安全的写访问权限，这是下一步所必需的。`ChainedPics::new`函数被标记为非安全的，因为提供错误的偏移量将可能导致未定义的行为。


We can now initialize the 8259 PIC in our init function:

现在，我们可以在`init`函数中初始化8259 PIC了：

```rust
// in src/lib.rs

pub fn init() {
    gdt::init();
    interrupts::init_idt();
    unsafe { interrupts::PICS.lock().initialize() }; // new
}
```

We use the [initialize](https://docs.rs/pic8259/0.10.1/pic8259/struct.ChainedPics.html#method.initialize) function to perform the PIC initialization. Like the `ChainedPics::new` function, this function is also unsafe because it can cause undefined behavior if the PIC is misconfigured.

我们使用[initialize](https://docs.rs/pic8259_simple/0.2.0/pic8259_simple/struct.ChainedPics.html#method.initialize)函数来执行PIC初始化。与ChainedPics::new函数一样，该函数也是非安全的，因为如果PIC配置错误，使用它也将可能导致未定义的行为。

If all goes well, we should continue to see the “It did not crash” message when executing `cargo run`.

如果一切顺利，执行`cargo run`时，我们应该继续看到“It not not crash”消息

<h2>Enabling Interrupts(启用中断)</h2>

Until now, nothing happened because interrupts are still disabled in the CPU configuration. This means that the CPU does not listen to the interrupt controller at all, so no interrupts can reach the CPU. Let’s change that:

到目前为止，什么都没发生，因为在CPU配置中依然禁用着中断。这意味着CPU根本不侦听中断控制器，也就没有中断可以到达CPU。让我们更改一下配置：

```rust
// in src/lib.rs

pub fn init() {
    gdt::init();
    interrupts::init_idt();
    unsafe { interrupts::PICS.lock().initialize() };
    x86_64::instructions::interrupts::enable();     // new
}
```

The `interrupts::enable` function of the `x86_64` crate executes the special `sti` instruction (“set interrupts”) to enable external interrupts. When we try cargo `run now`, we see that a double fault occurs:

`x86_64`crate的`interrupts::enable`函数执行特殊的`sti`指令（即“设置中断”）以启用外部中断。当我们现在尝试`cargo run`时，将看到发生双重故障：

<img src="./img/qemu-hardware-timer-double-fault.png">

The reason for this double fault is that the hardware timer (the [Intel 8253](https://en.wikipedia.org/wiki/Intel_8253), to be exact) is enabled by default, so we start receiving timer interrupts as soon as we enable interrupts. Since we didn’t define a handler function for it yet, our double fault handler is invoked.

发生此双重故障是因为硬件计时器在默认情况下为启用状态（确切地说是Intel 8253），因此一旦启用中断，我们便开始接收计时器中断。由于尚未为计时器定义处理函数，因此双重故障处理程序将会被调用。


<h2>Handling Timer Interrupts(处理计时器中断)</h2>


As we see from the graphic above, the timer uses line 0 of the primary PIC. This means that it arrives at the CPU as interrupt 32 (0 + offset 32). Instead of hardcoding index 32, we store it in an `InterruptIndex` enum:

从上图可以看出，定时器使用主PIC的0号线。这意味着它将作为中断32（0+偏移量32）到达CPU。我们不对索引32进行硬编码，而是将其存放在`InterruptIndex`枚举中：

```rust
// in src/interrupts.rs

#[derive(Debug, Clone, Copy)]
#[repr(u8)]
pub enum InterruptIndex {
    Timer = PIC_1_OFFSET,
}

impl InterruptIndex {
    fn as_u8(self) -> u8 {
        self as u8
    }

    fn as_usize(self) -> usize {
        usize::from(self.as_u8())
    }
}
```

The enum is a [C-like enum](https://doc.rust-lang.org/reference/items/enumerations.html#custom-discriminant-values-for-fieldless-enumerations) so that we can directly specify the index for each variant. The `repr(u8)` attribute specifies that each variant is represented as a `u8`. We will add more variants for other interrupts in the future.

该枚举是一个C风格枚举，因此我们可以直接为每个成员变量指定索引。`repr(u8)`属性指定每个变体都表示为`u8`。将来我们还会添加更多中断变量。

Now we can add a handler function for the timer interrupt:

现在我们可以为计时器中断添加一个处理函数：

```rust
// in src/interrupts.rs

use crate::print;

lazy_static! {
    static ref IDT: InterruptDescriptorTable = {
        let mut idt = InterruptDescriptorTable::new();
        idt.breakpoint.set_handler_fn(breakpoint_handler);
        […]
        idt[InterruptIndex::Timer.as_usize()]
            .set_handler_fn(timer_interrupt_handler); // new

        idt
    };
}

extern "x86-interrupt" fn timer_interrupt_handler(
    _stack_frame: InterruptStackFrame)
{
    print!(".");
}
```

Our `timer_interrupt_handler` has the same signature as our exception handlers, because the CPU reacts identically to exceptions and external interrupts (the only difference is that some exceptions push an error code). The [InterruptDescriptorTable](https://docs.rs/x86_64/0.14.2/x86_64/structures/idt/struct.InterruptDescriptorTable.html) struct implements the [IndexMut](https://doc.rust-lang.org/core/ops/trait.IndexMut.html) trait, so we can access individual entries through array indexing syntax.

`timer_interrupt_handler`的函数签名与之前的异常处理函数相同，因为CPU对异常和外部中断的反应相同（唯一的区别是某些异常会推送错误码）。[InterruptDescriptorTable](https://docs.rs/x86_64/0.14.2/x86_64/structures/idt/struct.InterruptDescriptorTable.html)结构体实现了[IndexMut](https://doc.rust-lang.org/core/ops/trait.IndexMut.html) trait，因此我们可以使用数组索引语法访问各个条目。


In our timer interrupt handler, we print a dot to the screen. As the timer interrupt happens periodically, we would expect to see a dot appearing on each timer tick. However, when we run it, we see that only a single dot is printed:

在计时器中断处理程序中，我们在屏幕上打印了一个点。由于计时器中断是周期性发生的，因此我们希望每个计时器周期都出现一个点。但是，当我们运行它时，我们看到只打印了一个点：

<img src="./img/qemu-single-dot-printed.png">

<h3>End of Interrupt (中断结束)</h3>

The reason is that the PIC expects an explicit “end of interrupt” (EOI) signal from our interrupt handler. This signal tells the controller that the interrupt was processed and that the system is ready to receive the next interrupt. So the PIC thinks we’re still busy processing the first timer interrupt and waits patiently for the EOI signal before sending the next one.

原因是PIC期望从我们的中断处理程序中得到一个明确的 "中断结束"（EOI）信号。该信号告诉控制器该中断已被处理，同时系统已经准备好接收下一个中断。因此，PIC认为我们仍在忙于处理第一个计时器中断，并在耐心等待EOI信号，然后才发送下一个中断。

To send the EOI, we use our static `PICS` struct again:

要发送EOI，我们需要再次使用静态`PICS`结构体：

```rust
// in src/interrupts.rs

extern "x86-interrupt" fn timer_interrupt_handler(
    _stack_frame: InterruptStackFrame)
{
    print!(".");

    unsafe {
        PICS.lock()
            .notify_end_of_interrupt(InterruptIndex::Timer.as_u8());
    }
}
```

The `notify_end_of_interrupt` figures out whether the primary or secondary PIC sent the interrupt and then uses the `command` and `data` ports to send an EOI signal to the respective controllers. If the secondary PIC sent the interrupt, both PICs need to be notified because the secondary PIC is connected to an input line of the primary PIC.

`notify_end_of_interrupt`会推断出是主PIC还是从PIC发送了中断，然后使用`command`和`data`端口将EOI信号发送到各控制器。如果是从PIC发送了中断，则需要通知两个PIC，因为从PIC通过输入线连接在主PIC上。

We need to be careful to use the correct interrupt vector number, otherwise we could accidentally delete an important unsent interrupt or cause our system to hang. This is the reason that the function is unsafe.

我们需要小心的使用正确的中断向量编号，否则可能会意外删除重要的未发送中断或导致系统挂起。这也是为什么该函数别标记为了非安全。

When we now execute `cargo run` we see dots periodically appearing on the screen:

现在，当我们执行`cargo run`时，我们会看到点定期出现在屏幕上：

<img src="./img/qemu-hardware-timer-dots.gif">

<h3>Configuring the Timer(配置计时器)</h3>

The hardware timer that we use is called the Programmable Interval Timer, or PIT, for short. Like the name says, it is possible to configure the interval between two interrupts. We won’t go into details here because we will switch to the [APIC timer](https://wiki.osdev.org/APIC_timer) soon, but the OSDev wiki has an extensive article about the [configuring the PIT](https://wiki.osdev.org/Programmable_Interval_Timer).

我们使用的硬件计时器叫做可编程间隔计时器，也简称为PIT。顾名思义，我们可以配置两个中断之间的间隔。这里不做详细介绍，因为后文将很快切换到[APIC计时器](https://wiki.osdev.org/APIC_timer)，但是OSDev Wiki上有大量有关[配置PIT](https://wiki.osdev.org/Programmable_Interval_Timer)的文章。


<h2>Deadlocks(死锁)</h2>

We now have a form of concurrency in our kernel: The timer interrupts occur asynchronously, so they can interrupt our `_start` function at any time. Fortunately, Rust’s ownership system prevents many types of concurrency-related bugs at compile time. One notable exception is deadlocks. Deadlocks occur if a thread tries to acquire a lock that will never become free. Thus, the thread hangs indefinitely.

现在，我们的内核中具有了一种并发形式：定时器中断会异步的发生，因此它们可以随时中断我们的`_start`函数。幸运的是，Rust的所有权系统可以在编译时就能够防止很多与并发相关的bug。不过，死锁是一个值得注意的例外。如果线程试图获取永远不会释放的锁，则会发生死锁。此时，线程会无限期地挂起。

We can already provoke a deadlock in our kernel. Remember, our `println` macro calls the `vga_buffer::_print` function, which [locks a global WRITER](https://os.phil-opp.com/vga-text-mode/#spinlocks) using a spinlock:

我们现在就可以在内核中诱发死锁。记住，我们的`println`宏调用`vga_buffer::__print`函数，而该函数会用自旋锁锁定全局变量[WRITER](https://os.phil-opp.com/vga-text-mode/#spinlocks)：


```rust
// in src/vga_buffer.rs

[…]

#[doc(hidden)]
pub fn _print(args: fmt::Arguments) {
    use core::fmt::Write;
    WRITER.lock().write_fmt(args).unwrap();
}
```


It locks the `WRITER`, calls `write_fmt` on it, and implicitly unlocks it at the end of the function. Now imagine that an interrupt occurs while the `WRITER` is locked and the interrupt handler tries to print something too:

该函数先锁定`WRITER`再调用其`write_fmt`，并会在函数末尾隐式将`WRITER`解锁。现在想象一下，在`WRITER`锁定时发生了中断，并且中断处理程序也尝试打印一些内容：


| Timestep | _start             | interrupt_handler                           |
| -------- | ------------------ | ------------------------------------------- |
| 0        | calls println!     |
| 1        | print locks WRITER |
| 2        |                    | interrupt occurs, handler begins to run     |
| 3        |                    | calls println!                              |
| 4        |                    | print tries to lock WRITER (already locked) |
| 5        |                    | print tries to lock WRITER (already locked) |
| …        |                    | …                                           |
| never    | unlock WRITER      |




The `WRITER` is locked, so the interrupt handler waits until it becomes free. But this never happens, because the `_start` function only continues to run after the interrupt handler returns. Thus, the entire system hangs.

`WRITER`被锁定，因此中断处理程序会等待锁释放。但这永远不会发生，因为`_start`函数仅在中断处理程序返回后才继续运行。因此，整个系统挂起。

<h3>Provoking a Deadlock(引发一次死锁)</h3>

We can easily provoke such a deadlock in our kernel by printing something in the loop at the end of our `_start` function:

通过在_start函数末尾的loop循环中打印一些内容，我们就可以轻松地在内核中引发这种死锁：

```rust
// in src/main.rs

#[no_mangle]
pub extern "C" fn _start() -> ! {
    […]
    loop {
        use blog_os::print;
        print!("-");        // new
    }
}

```

When we run it in QEMU, we get an output of the form:

当我们在QEMU中运行他，我们可以看到如下形式的输出:

<img src="./img/qemu-deadlock.png">


We see that only a limited number of hyphens are printed until the first timer interrupt occurs. Then the system hangs because the timer interrupt handler deadlocks when it tries to print a dot. This is the reason that we see no dots in the above output.

我们看到只有有限的连字符被打印，当第一次定时器中断发生时便停止打印。之后系统挂起，因为计时器中断处理程序在尝试打印点时会死锁。这就是我们在上面的输出中看不到任何点的原因。

The actual number of hyphens varies between runs because the timer interrupt occurs asynchronously. This non-determinism is what makes concurrency-related bugs so difficult to debug.

每次运行打印的连字符数量会有所不同，因为计时器中断是异步发生的。正是这种不确定性使得与并发相关的bug难以调试。

<h3>Fixing the Deadlock(修复死锁)</h3>

To avoid this deadlock, we can disable interrupts as long as the `Mutex` is locked:

为了避免发生这种死锁，只要`Mutex`处于锁定状态，我们就禁用中断：

```rust
// in src/vga_buffer.rs

/// Prints the given formatted string to the VGA text buffer
/// through the global `WRITER` instance.
#[doc(hidden)]
pub fn _print(args: fmt::Arguments) {
    use core::fmt::Write;
    use x86_64::instructions::interrupts;   // new

    interrupts::without_interrupts(|| {     // new
        WRITER.lock().write_fmt(args).unwrap();
    });
}
```

The [without_interrupts](https://docs.rs/x86_64/0.14.2/x86_64/instructions/interrupts/fn.without_interrupts.html) function takes a [closure](https://doc.rust-lang.org/book/ch13-01-closures.html) and executes it in an interrupt-free environment. We use it to ensure that no interrupt can occur as long as the `Mutex` is locked. When we run our kernel now, we see that it keeps running without hanging. (We still don’t notice any dots, but this is because they’re scrolling by too fast. Try to slow down the printing, e.g., by putting a `for _ in 0..10000 {}` inside the loop.)

[without_interrupts](https://docs.rs/x86_64/0.14.2/x86_64/instructions/interrupts/fn.without_interrupts.html)函数将获取一个闭包并在无中断的环境中执行该闭包。我们使用它来确保只要互斥锁被锁定，就不会发生中断。现在，当我们运行内核时，我们看到它一直在运行而不会挂起。（我们仍然没有看到任何点，这是因为打印滚动的速度太快。请尝试减慢打印速度，例如，将`for _ in 0..10000 {}`放置在loop中。）


We can apply the same change to our serial printing function to ensure that no deadlocks occur with it either:

我们可以对串行打印功能应用相同的更改，以确保不会发生死锁：

```rust
// in src/serial.rs

#[doc(hidden)]
pub fn _print(args: ::core::fmt::Arguments) {
    use core::fmt::Write;
    use x86_64::instructions::interrupts;       // new

    interrupts::without_interrupts(|| {         // new
        SERIAL1
            .lock()
            .write_fmt(args)
            .expect("Printing to serial failed");
    });
}
```

Note that disabling interrupts shouldn’t be a general solution. The problem is that it increases the worst-case interrupt latency, i.e., the time until the system reacts to an interrupt. Therefore, interrupts should only be disabled for a very short time.

请注意，禁用中断并不应该作为通用的解决方案。因为这样做会增加最坏情况下的中断响应时间，即系统对中断做出反应的时间。因此，只应该在很短的时间内禁用中断。

<h2>Fixing a Race Condition(修复竞争情况)</h2>

If you run `cargo test`, you might see the `test_println_output` test failing:

现在如果执行`cargo test`，可能会看到`test_println_output`测试失败：


```
> cargo test --lib
[…]
Running 4 tests
test_breakpoint_exception...[ok]
test_println... [ok]
test_println_many... [ok]
test_println_output... [failed]

Error: panicked at 'assertion failed: `(left == right)`
  left: `'.'`,
 right: `'S'`', src/vga_buffer.rs:205:9
```

The reason is a race condition between the test and our timer handler. Remember, the test looks like this:

原因是测试与我们的计时器处理程序之间存在竞争条件。回忆一下测试看起来像这样：


```rust
// in src/vga_buffer.rs

#[test_case]
fn test_println_output() {
    let s = "Some test string that fits on a single line";
    println!("{}", s);
    for (i, c) in s.chars().enumerate() {
        let screen_char = WRITER.lock().buffer.chars[BUFFER_HEIGHT - 2][i].read();
        assert_eq!(char::from(screen_char.ascii_character), c);
    }
}
```

The test prints a string to the VGA buffer and then checks the output by manually iterating over the `buffer_char`s array. The race condition occurs because the timer interrupt handler might run between the `println` and the reading of the screen characters. Note that this isn’t a dangerous data race, which Rust completely prevents at compile time. See the [Rustonomicon](https://doc.rust-lang.org/nomicon/races.html) for details.

该测试将一个字符串打印到VGA缓冲区，然后通过手动迭代`buffer_chars`数组来检查输出。由于计时器中断处理程序可能在`println`之后，读取屏幕字符之前运行（中断处理函数会输出一个.），因此发生竞争状态。请注意，这不是危险的数据竞争，Rust在编译时完全避免了这种竞争。有关详细信息，请参见[Rustonomicon](https://doc.rust-lang.org/nomicon/races.html)。


To fix this, we need to keep the `WRITER` locked for the complete duration of the test, so that the timer handler can’t write a `.` to the screen in between. The fixed test looks like this:

要解决此问题，我们需要在测试的整个过程中保持`WRITER`处于锁定状态，以使计时器处理程序无法在“打印行为”和“读取行为”之间将`.`打印到屏幕上。修复的测试如下所示：


```rust
// in src/vga_buffer.rs

#[test_case]
fn test_println_output() {
    use core::fmt::Write;
    use x86_64::instructions::interrupts;

    let s = "Some test string that fits on a single line";
    interrupts::without_interrupts(|| {
        let mut writer = WRITER.lock();
        writeln!(writer, "\n{}", s).expect("writeln failed");
        for (i, c) in s.chars().enumerate() {
            let screen_char = writer.buffer.chars[BUFFER_HEIGHT - 2][i].read();
            assert_eq!(char::from(screen_char.ascii_character), c);
        }
    });
}
```

We performed the following changes:

- We keep the writer locked for the complete test by using the `lock()` method explicitly. Instead of `println`, we use the [writeln](https://doc.rust-lang.org/core/macro.writeln.html) macro that allows printing to an already locked writer.
- To avoid another deadlock, we disable interrupts for the test’s duration. Otherwise, the test might get interrupted while the writer is still locked.
- Since the timer interrupt handler can still run before the test, we print an additional newline `\n` before printing the string `s`. This way, we avoid test failure when the timer handler has already printed some `.` characters to the current line.


我们进行了以下改进：

- 显式调用`lock()`方法，使`WRITER`在整个测试过程中保持锁定状态。代替`println`，我们使用`writeln`宏，该宏允许打印到已经锁定的写入器。
- 为了避免再次出现死锁，我们在测试期间禁用中断。否则，在`WRITER`仍处于锁定状态时，测试可能会中断。
- 由于计时器中断处理程序仍旧可能在测试之前运行，因此在打印字符串`s`之前，我们还要打印一个换行符`\n`。这样，即使计时器中断处理程序已经打印出`.`，我们仍然可以避免测试失败。

With the above changes, `cargo test` now deterministically succeeds again.

通过上述更改，现在`cargo test`可以再次运行成功了。


This was a very harmless race condition that only caused a test failure. As you can imagine, other race conditions can be much more difficult to debug due to their non-deterministic nature. Luckily, Rust prevents us from data races, which are the most serious class of race conditions since they can cause all kinds of undefined behavior, including system crashes and silent memory corruptions.

这是一个无害的竞争条件，仅可能会导致测试失败。你可以想象，其他竞争条件会由于其不确定性而更加难以调试。幸运的是，Rust帮我们阻止了最严重的竞争条件——数据竞争，该竞争条件会导致各种不确定的行为，包括系统崩溃和静默的内存数据损坏。


## The `hlt` Instruction(`hlt`指令)


Until now, we used a simple empty loop statement at the end of our `_start` and `panic` functions. This causes the CPU to spin endlessly, and thus works as expected. But it is also very inefficient, because the CPU continues to run at full speed even though there’s no work to do. You can see this problem in your task manager when you run your kernel: The QEMU process needs close to 100% CPU the whole time.

到目前为止，我们在`_start`和`panic`函数的末尾使用了一个简单的空循环语句。这将使得CPU一直在工作，虽然这是代码预期的效果，但这也是非常低效的，因为即使没有任何工作，CPU仍将继续满负荷运行。运行内核时，您可以在任务管理器中观察到此现象：QEMU进程始终需要近100％的CPU使用率。

What we really want to do is to halt the CPU until the next interrupt arrives. This allows the CPU to enter a sleep state in which it consumes much less energy. The [hlt instruction](https://en.wikipedia.org/wiki/HLT_(x86_instruction)) does exactly that. Let’s use this instruction to create an energy-efficient endless loop:

我们真正想做的是停止CPU，直到收到下一个中断信号。这期间允许CPU进入睡眠状态以降低消耗。[hlt指令](https://en.wikipedia.org/wiki/HLT_(x86_instruction))正是这样的功能。我们使用该指令创建一个节能的无限循环：


```rust
// in src/lib.rs

pub fn hlt_loop() -> ! {
    loop {
        x86_64::instructions::hlt();
    }
}
```

The `instructions::hlt` function is just a [thin wrapper](https://github.com/rust-osdev/x86_64/blob/5e8e218381c5205f5777cb50da3ecac5d7e3b1ab/src/instructions/mod.rs#L16-L22) around the assembly instruction. It is safe because there’s no way it can compromise memory safety.

`instructions::hlt`函数只是[简单封装](https://github.com/rust-osdev/x86_64/blob/5e8e218381c5205f5777cb50da3ecac5d7e3b1ab/src/instructions/mod.rs#L16-L22)了汇编指令。不过这是安全的，因为这个操作并不会损害内存安全。


We can now use this hlt_loop instead of the endless loops in our _start and panic functions:

现在，我们可以使用此`hlt_loop`代替`_start`和`panic`函数中的无限循环：

```rust
// in src/main.rs

#[no_mangle]
pub extern "C" fn _start() -> ! {
    […]

    println!("It did not crash!");
    blog_os::hlt_loop();            // new
}


#[cfg(not(test))]
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("{}", info);
    blog_os::hlt_loop();            // new
}
```


Let’s update our `lib.rs` as well:

我们把`lib.rs`也更新一下：


```rust
// in src/lib.rs

/// Entry point for `cargo test`
#[cfg(test)]
#[no_mangle]
pub extern "C" fn _start() -> ! {
    init();
    test_main();
    hlt_loop();         // new
}

pub fn test_panic_handler(info: &PanicInfo) -> ! {
    serial_println!("[failed]\n");
    serial_println!("Error: {}\n", info);
    exit_qemu(QemuExitCode::Failed);
    hlt_loop();         // new
}

```

When we run our kernel now in QEMU, we see a much lower CPU usage.

当我们现在在QEMU中运行我们的内核时，我们看到CPU的使用率大大降低。



</h2>Keyboard Input(键盘输入)</h2>

Now that we are able to handle interrupts from external devices, we are finally able to add support for keyboard input. This will allow us to interact with our kernel for the first time.

现在我们已经能够处理来自外部设备的中断，我们终于能够增加对键盘输入的支持。这将使我们能够第一次与我们的内核进行交互。

>Note that we only describe how to handle [PS/2](https://en.wikipedia.org/wiki/PS/2_port) keyboards here, not USB keyboards. However, the mainboard emulates USB keyboards as PS/2 devices to support older software, so we can safely ignore USB keyboards until we have USB support in our kernel.

>注意，我们在这里只描述了如何处理[PS/2](https://en.wikipedia.org/wiki/PS/2_port)键盘，而不是USB键盘。然而，主板将USB键盘模拟成PS/2设备以支持旧的软件，所以我们可以安全地忽略USB键盘，直到我们的内核支持USB。


Like the hardware timer, the keyboard controller is already enabled by default. So when you press a key, the keyboard controller sends an interrupt to the PIC, which forwards it to the CPU. The CPU looks for a handler function in the IDT, but the corresponding entry is empty. Therefore, a double fault occurs.

与硬件计时器一样，键盘控制器默认已经启用。因此，当你按下一个键时，键盘控制器会向PIC发送一个中断，PIC将其转发给CPU。CPU在IDT中寻找一个处理函数，但相应的条目是空的。因此，产生了一个双重故障。


So let’s add a handler function for the keyboard interrupt. It’s quite similar to how we defined the handler for the timer interrupt; it just uses a different interrupt number:

让我们为键盘中断添加一个处理函数。这与我们为计时器中断定义的处理程序非常相似；它只是使用了一个不同的中断号。

```rust
// in src/interrupts.rs

#[derive(Debug, Clone, Copy)]
#[repr(u8)]
pub enum InterruptIndex {
    Timer = PIC_1_OFFSET,
    Keyboard, // new
}

lazy_static! {
    static ref IDT: InterruptDescriptorTable = {
        let mut idt = InterruptDescriptorTable::new();
        idt.breakpoint.set_handler_fn(breakpoint_handler);
        […]
        // new
        idt[InterruptIndex::Keyboard.as_usize()]
            .set_handler_fn(keyboard_interrupt_handler);

        idt
    };
}

extern "x86-interrupt" fn keyboard_interrupt_handler(
    _stack_frame: InterruptStackFrame)
{
    print!("k");

    unsafe {
        PICS.lock()
            .notify_end_of_interrupt(InterruptIndex::Keyboard.as_u8());
    }
}
```

As we see from the graphic above, the `keyboard` uses line 1 of the primary PIC. This means that it arrives at the CPU as interrupt 33 (1 + offset 32). We add this index as a new Keyboard variant to the `InterruptIndex` enum. We don’t need to specify the value explicitly, since it defaults to the previous value plus one, which is also 33. In the interrupt handler, we print a `k` and send the end of interrupt signal to the interrupt controller.

正如我们从上面的图形中看到的，"键盘 "使用主PIC的1号线。这意味着它作为中断33（1+偏移32）到达CPU。我们把这个索引作为一个新的键盘变量添加到`InterruptIndex`枚举中。我们不需要明确指定这个值，因为它默认为前一个值加1，也就是33。 在中断处理程序中，我们打印一个`k`并向中断控制器发送中断结束信号。

We now see that a `k` appears on the screen when we press a key. However, this only works for the first key we press. Even if we continue to press keys, no more ks appear on the screen. This is because the keyboard controller won’t send another interrupt until we have read the so-called scancode of the pressed key.

我们现在看到，当我们按下一个键时，屏幕上会出现一个`K`。然而，这只对我们按的第一个键有效。即使我们继续按键，屏幕上也不会再出现`k`。这是因为键盘控制器不会发送另一个中断，直到我们读取了被按下的键的scancode。


<h3>Reading the Scancodes(读取Scancodes)</h3>

To find out which key was pressed, we need to query the keyboard controller. We do this by reading from the data port of the PS/2 controller, which is the [I/O port](https://os.phil-opp.com/testing/#i-o-ports) with the number `0x60`:

要知道哪个键被按下，我们需要查询键盘控制器。我们通过读取PS/2控制器的数据端口，即编号为`0x60`的[I/O端口](https://os.phil-opp.com/testing/#i-o-ports)来做到这一点。

```rust
// in src/interrupts.rs

extern "x86-interrupt" fn keyboard_interrupt_handler(
    _stack_frame: InterruptStackFrame)
{
    use x86_64::instructions::port::Port;

    let mut port = Port::new(0x60);
    let scancode: u8 = unsafe { port.read() };
    print!("{}", scancode);

    unsafe {
        PICS.lock()
            .notify_end_of_interrupt(InterruptIndex::Keyboard.as_u8());
    }
}
```

We use the [Port](https://docs.rs/x86_64/0.14.2/x86_64/instructions/port/struct.Port.html) type of the `x86_64` crate to read a byte from the keyboard’s data port. This byte is called the [scancode](https://en.wikipedia.org/wiki/Scancode) and it represents the key press/release. We don’t do anything with the scancode yet, other than print it to the screen:

我们使用`x86_64`包的[端口](https://docs.rs/x86_64/0.14.2/x86_64/instructions/port/struct.Port.html)类型，从键盘的数据端口读取一个字节。这个字节被称为`scancode`，它代表按键的按下/释放。除了把它打印到屏幕上之外，我们还没有对scancode做任何处理。

<img src="./img/qemu-printing-scancodes.gif">

The above image shows me slowly typing “123”. We see that adjacent keys have adjacent scancodes and that pressing a key causes a different scancode than releasing it. But how do we translate the scancodes to the actual key actions exactly?

上面的图片显示了我慢慢地输入 "123"。我们看到，相邻的键有相邻的代码，按下一个键与松开一个键所引起的代码是不同的。但是，我们如何将这些代码准确地转化为实际对应的案件呢？

<h3>Interpreting the Scancodes(转义Scancodes)</h3>

There are three different standards for the mapping between scancodes and keys, the so-called scancode sets. All three go back to the keyboards of early IBM computers: the [IBM XT](https://en.wikipedia.org/wiki/IBM_Personal_Computer_XT), the [IBM 3270 PC](https://en.wikipedia.org/wiki/IBM_3270_PC), and the [IBM AT](https://en.wikipedia.org/wiki/IBM_Personal_Computer/AT). Later computers fortunately did not continue the trend of defining new scancode sets, but rather emulated the existing sets and extended them. Today, most keyboards can be configured to emulate any of the three sets.

对于编码和按键之间的映射有三种不同的标准，即所谓的编码集。这三种标准都可以追溯到早期IBM计算机的键盘：[IBM XT](https://en.wikipedia.org/wiki/IBM_Personal_Computer_XT)、[IBM 3270 PC](https://en.wikipedia.org/wiki/IBM_3270_PC)和[IBM AT](https://en.wikipedia.org/wiki/IBM_Personal_Computer/AT)。幸运的是，后来的计算机并没有继续定义新的字符集，而是模仿现有的字符集并加以扩展。今天，大多数键盘可以被配置为模拟这三组中的任何一组。


By default, PS/2 keyboards emulate scancode set 1 (“XT”). In this set, the lower 7 bits of a scancode byte define the key, and the most significant bit defines whether it’s a press (“0”) or a release (“1”). Keys that were not present on the original [IBM XT](https://en.wikipedia.org/wiki/IBM_Personal_Computer_XT) keyboard, such as the enter key on the keypad, generate two scancodes in succession: a `0xe0` escape byte and then a byte representing the key. For a list of all set 1 scancodes and their corresponding keys, check out the [OSDev Wiki](https://wiki.osdev.org/Keyboard#Scan_Code_Set_1).

默认情况下，PS/2键盘模拟的是scancode set 1（"XT"）。在这套编码中，扫描码字节的低7位定义了按键，最高位定义了它是按下（"0"）还是释放（"1"）。原有的IBM XT键盘上没有的键，如小键盘上的回车键，会连续产生两个scancode：一个`0xe0`转义字节，然后是代表该键的字节。有关集合1中所有扫描码及其对应键的表，请查看[OSDev Wiki](https://wiki.osdev.org/Keyboard#Scan_Code_Set_1)。

To translate the scancodes to keys, we can use a match statement:

要将scancodes转换为键，我们可以使用match语句:


```rust
// in src/interrupts.rs

extern "x86-interrupt" fn keyboard_interrupt_handler(
    _stack_frame: InterruptStackFrame)
{
    use x86_64::instructions::port::Port;

    let mut port = Port::new(0x60);
    let scancode: u8 = unsafe { port.read() };

    // new
    let key = match scancode {
        0x02 => Some('1'),
        0x03 => Some('2'),
        0x04 => Some('3'),
        0x05 => Some('4'),
        0x06 => Some('5'),
        0x07 => Some('6'),
        0x08 => Some('7'),
        0x09 => Some('8'),
        0x0a => Some('9'),
        0x0b => Some('0'),
        _ => None,
    };
    if let Some(key) = key {
        print!("{}", key);
    }

    unsafe {
        PICS.lock()
            .notify_end_of_interrupt(InterruptIndex::Keyboard.as_u8());
    }
}
```

The above code translates keypresses of the number keys 0-9 and ignores all other keys. It uses a `match` statement to assign a character or None to each scancode. It then uses `if let` to destructure the optional `key`. By using the same variable name key in the pattern, we shadow the previous declaration, which is a common pattern for destructuring `Option` types in Rust.

上面的代码将翻译数字键0-9，并忽略其他键。它使用`match`语句为每个扫描码分配一个字符或一个None。然后使用`if let`语句解构变量key中的字符。通过在模式中使用相同变量名key，我们可以遮蔽先前的声明，这是Rust中解构`Option`类型的常见写法。


Now we can write numbers:

现在我们可以打印数字了：

<img src="./img/qemu-printing-numbers.gif">

Translating the other keys works in the same way. Fortunately, there is a crate named [pc-keyboard](https://docs.rs/pc-keyboard/0.5.0/pc_keyboard/) for translating scancodes of scancode sets 1 and 2, so we don’t have to implement this ourselves. To use the crate, we add it to our `Cargo.toml` and import it in our `lib.rs`:

翻译其他键的方法相同。 幸运的是，有一个名为[pc-keyboard](https://docs.rs/pc-keyboard/0.5.0/pc_keyboard/)的crate可用于翻译集1和集2的扫描码，因此我们不必自己实现此功能。要使用crate，请将其添加到`Cargo.toml`中，然后将其导入`lib.rs`中：


```toml
# in Cargo.toml

[dependencies]
pc-keyboard = "0.5.0"
```


Now we can use this crate to rewrite our keyboard_interrupt_handler:

现在，我们可以使用此crate重写我们的`keyboard_interrupt_handler`：

```rust
// in/src/interrupts.rs

extern "x86-interrupt" fn keyboard_interrupt_handler(
    _stack_frame: InterruptStackFrame)
{
    use pc_keyboard::{layouts, DecodedKey, HandleControl, Keyboard, ScancodeSet1};
    use spin::Mutex;
    use x86_64::instructions::port::Port;

    lazy_static! {
        static ref KEYBOARD: Mutex<Keyboard<layouts::Us104Key, ScancodeSet1>> =
            Mutex::new(Keyboard::new(layouts::Us104Key, ScancodeSet1,
                HandleControl::Ignore)
            );
    }

    let mut keyboard = KEYBOARD.lock();
    let mut port = Port::new(0x60);

    let scancode: u8 = unsafe { port.read() };
    if let Ok(Some(key_event)) = keyboard.add_byte(scancode) {
        if let Some(key) = keyboard.process_keyevent(key_event) {
            match key {
                DecodedKey::Unicode(character) => print!("{}", character),
                DecodedKey::RawKey(key) => print!("{:?}", key),
            }
        }
    }

    unsafe {
        PICS.lock()
            .notify_end_of_interrupt(InterruptIndex::Keyboard.as_u8());
    }
}
```

We use the `lazy_static` macro to create a static `Keyboard` object protected by a `Mutex`. We initialize the Keyboard with a US keyboard layout and the scancode set 1. The [HandleControl](https://docs.rs/pc-keyboard/0.5.0/pc_keyboard/enum.HandleControl.html) parameter allows to map `ctrl+[a-z]` to the Unicode characters `U+0001` through `U+001A`. We don’t want to do that, so we use the `Ignore` option to handle the `ctrl` like normal keys.

通过`lazy_static`宏创建一个由`Mutex`保护的静态`Keyboard`对象。使用美式键盘布局和扫描码集1初始化`Keyboard`。[HandleControl](https://docs.rs/pc-keyboard/0.5.0/pc_keyboard/enum.HandleControl.html)参数允许将`ctrl+[a-z]`映射到`U+0001`至`U+001A`的Unicode字符上。我们并不想这样做，因此使用Ignore选项来像处理普通键一样处理`ctrl`。

On each interrupt, we lock the Mutex, read the scancode from the keyboard controller, and pass it to the [add_byte](https://docs.rs/pc-keyboard/0.5.0/pc_keyboard/struct.Keyboard.html#method.add_byte) method, which translates the scancode into an `Option<KeyEvent>`. The [KeyEvent](https://docs.rs/pc-keyboard/0.5.0/pc_keyboard/struct.KeyEvent.html) contains the key which caused the event and whether it was a press or release event.

对于每个中断，我们锁定Mutex，从键盘控制器读取scancode，并将其传递给[add_byte](https://docs.rs/pc-keyboard/0.5.0/pc_keyboard/struct.Keyboard.html#method.add_byte)方法，该方法将scancode转换为`Option<KeyEvent>`。[KeyEvent](https://docs.rs/pc-keyboard/0.5.0/pc_keyboard/struct.KeyEvent.html)包含触发事件的按键，以及究竟是按下事件还是释放事件。


To interpret this key event, we pass it to the [process_keyevent](https://docs.rs/pc-keyboard/0.5.0/pc_keyboard/struct.Keyboard.html#method.process_keyevent) method, which translates the key event to a character, if possible. For example, it translates a press event of the `A` key to either `a` lowercase a character or an uppercase `A` character, depending on whether the shift key was pressed.

为了解释这个按键事件，我们把它传递给[process_keyevent](https://docs.rs/pc-keyboard/0.5.0/pc_keyboard/struct.Keyboard.html#method.process_keyevent)方法，如果没问题的话，该方法会把按键事件翻译成一个字符。例如，它将`A`键的按下事件转化为小写的`a`字符或大写的`A`字符，这取决于是否按下了shift键。

With this modified interrupt handler, we can now write text:

使用修改后的中断处理程序，我们已经能够输入文本了：

<img src="./img/qemu-typing.gif">


<h3>Configuring the Keyboard(键盘配置)</h3>

It’s possible to configure some aspects of a `PS/2` keyboard, for example, which scancode set it should use. We won’t cover it here because this post is already long enough, but the OSDev Wiki has an overview of possible [configuration commands](https://wiki.osdev.org/PS/2_Keyboard#Commands).


我们可以配置`PS/2`键盘的某些功能，例如应使用哪个扫描码集。我们不会在这里介绍它，因为这篇文章已经足够长了，但是OSDev Wiki概述了可能的[配置命令](https://wiki.osdev.org/PS/2_Keyboard#Commands)。


<h2>Summary(总结)</h2>

This post explained how to enable and handle external interrupts. We learned about the 8259 PIC and its primary/secondary layout, the remapping of the interrupt numbers, and the “end of interrupt” signal. We implemented handlers for the hardware timer and the keyboard and learned about the `hlt` instruction, which halts the CPU until the next interrupt.

本文解释了如何启用和处理外部设备中断。我们了解了8259 PIC及其主/从布局、中断号的重新映射以及发送"中断结束"信号。我们为硬件计时器和键盘中断实现了处理程序，并了解了`hlt`指令，该指令能够将CPU暂停，直到下一个中断。



Now we are able to interact with our kernel and have some fundamental building blocks for creating a small shell or simple games.

现在，我们可以与内核进行交互，并且初步编写出了一些基础模块，可用于创建小型shell或简单游戏。