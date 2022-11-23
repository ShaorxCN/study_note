#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(blog_os::test_runner)] // 指定自定义测试框架的test_runner
#![reexport_test_harness_main = "test_main"] // 自定义测试框架默认是生成一个main函来调用 test_runner  这里重定义了调用名 然后_start里调用test_main
use blog_os::{
    memory::{self, translation_addr, BootInfoFrameAllocator},
    println,
};
use bootloader::{entry_point, BootInfo};
use core::panic::PanicInfo;
use x86_64::{
    structures::paging::{FrameAllocator, Page, PageTable, Translate},
    VirtAddr,
};

// 在底层定义了_start入口点 所以下面的#[no_mangle]和命名不重要了
// 并且对签名进行了检查
entry_point!(kernel_main);

// no_mangle 禁用名称重整  确保编译后的函数名称还是_start  _start多个系统的默认入口
// #[no_mangle]
// pub extern "C" fn _start(boot_info:&'static BootInfo) -> ! {

pub fn kernel_main(boot_info: &'static BootInfo) -> ! {
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

    // 地址位之前执行异常中展示的写操作地址 所以必定是code page
    // let ptr = 0x2031d3 as *mut u32;
    // unsafe{let x = *ptr;}
    // println!("read works");

    // unsafe{*ptr = 42;}
    // println!("write works");

    // use x86_64::registers::control::Cr3; // Cr3 存储存储四级表地址
    // let (level_4_page_table, _) = Cr3::read();
    // println!(
    //     "level 4 page table at:{:?}",
    //     level_4_page_table.start_address()
    // );

    // use blog_os::memory::active_level_4_table;
    // use x86_64::VirtAddr;

    // let phy_mem_offset = VirtAddr::new(boot_info.physical_memory_offset);
    // // 获取实际的l4虚拟地址
    // let l4_table = unsafe{active_level_4_table(phy_mem_offset)};

    // for (i,entry) in l4_table.iter().enumerate(){
    //     // 遍历可用条目
    //     if !entry.is_unused(){
    //         println!("L4 Entry {}:{:?}",i,entry);
    //         // 遍历l3
    //         let phys = entry.frame().unwrap().start_address();
    //         let virt  = phys.as_u64()+boot_info.physical_memory_offset;
    //         let ptr = VirtAddr::new(virt).as_mut_ptr();
    //         let l3_table:&PageTable = unsafe{&*ptr};

    //         for(i,entry) in l3_table.iter().enumerate(){
    //             if !entry.is_unused(){
    //                 println!("L3 Entry {}:{:?}",i,entry);
    //             }
    //         }
    //     }
    // }

    let phys_mem_offset = VirtAddr::new(boot_info.physical_memory_offset);
    let mut mapper = unsafe { memory::init(phys_mem_offset) };
    let address = [
        // VGA 直接映射  0xb8000
        0xb8000,
        // 某代码页
        0x201008,
        // 某栈页
        0x0100_0020_1a10,
        // 0 huge page
        boot_info.physical_memory_offset,
    ];

    for &address in &address {
        let virt = VirtAddr::new(address);
        // let phys = unsafe{translation_addr(virt, phys_mem_offset)};
        // 使用x86_64提供的功能转换
        let phys = mapper.translate_addr(virt);
        println!("{:?} -> {:?}", virt, phys);
    }

    // let mut frame_allocator = memory::EmptyFrameAllocator;
    let mut frame_allocator = unsafe { BootInfoFrameAllocator::init(&boot_info.memory_map) };

    // bootloader 会将自身映射到虚拟空间的第一个兆字节 且为为映射状态 这样说明l1已经存在 那么就不需要申请新的帧创建l1
    // 返回None的EmptyFrameAllocator也就没关系
    let page = Page::containing_address(VirtAddr::new(0));
    memory::create_example_mapping(page, &mut mapper, &mut frame_allocator);

    let page_ptr: *mut u64 = page.start_address().as_mut_ptr();
    unsafe {
        page_ptr.offset(400).write_volatile(0x_f021_f077_f065_f04e);
    }

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
