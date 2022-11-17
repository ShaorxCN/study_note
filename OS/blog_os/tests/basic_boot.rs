#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(blog_os::test_runner)]
#![reexport_test_harness_main = "test_main"]

use blog_os::{println, serial_print, serial_println};
use core::panic::PanicInfo;

#[test_case]
fn test_println() {
    serial_print!("test_println_in basic... ");
    println!("test_println output");
    serial_println!("[ok]");
}

#[no_mangle]
pub extern "C" fn _start() -> ! {
    test_main();
    loop {}
}

fn test_runner(_tests: &[&dyn Fn()]) {
    unimplemented!();
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    blog_os::test_panic_handler(info)
}
