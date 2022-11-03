#![no_std]
#![no_main]

mod vga_buffer;
use core::panic::PanicInfo;
// ！返回 发散型函数 表示没有返回 后面如果有代码也不会执行 有的话编译也会提示unreachable
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    println!("{}", _info);
    loop {}
}

// no_mangle 禁用名称重整  确保编译后的函数名称还是_start  _start多个系统的默认入口
#[no_mangle]
pub extern "C" fn _start() -> ! {
    println!("Hello World,evan{}", "!");
    panic!("Some panic message");
}
