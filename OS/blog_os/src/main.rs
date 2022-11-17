#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(blog_os::test_runner)] // 指定自定义测试框架的test_runner
#![reexport_test_harness_main = "test_main"] // 自定义测试框架默认是生成一个main函来调用 test_runner  这里重定义了调用名 然后_start里调用test_main
use blog_os::println;
use core::panic::PanicInfo;

// no_mangle 禁用名称重整  确保编译后的函数名称还是_start  _start多个系统的默认入口
#[no_mangle]
pub extern "C" fn _start() -> ! {
    println!("Hello World,evan{}", "!");
    blog_os::init();
    // x86_64::instructions::interrupts::int3();

    // unsafe {
    //     *(0xdeadbeef as *mut u64) = 42;
    // };
    // #[allow(unconditional_recursion)]
    // fn stack_overflow() {
    //     stack_overflow();
    // }

    // stack_overflow();

    #[cfg(test)]
    test_main();

    println!("it did not crash!");
    blog_os::hlt_loop();
}

// ！返回 发散型函数 表示没有返回 后面如果有代码也不会执行 有的话编译也会提示unreachable
#[cfg(not(test))]
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    println!("{}", _info);
    blog_os::hlt_loop();
}

// 指定test的时候使用串口通信输出到宿主机器 而不是qemu
#[cfg(test)]
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    blog_os::test_panic_handler(info)
}
