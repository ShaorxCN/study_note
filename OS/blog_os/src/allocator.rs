use alloc::alloc::{GlobalAlloc, Layout};
use core::ptr::null_mut;
use linked_list_allocator::LockedHeap;
use x86_64::{
    structures::paging::{
        mapper::MapToError, FrameAllocator, Mapper, Page, PageTableFlags, Size4KiB,
    },
    VirtAddr,
};

use spin::Mutex;

// use bump::BumpAllocator;

// use linked_list::LinkedListAllocator;

use fixed_size_block::FixedSizeBlockAllocator;

// 线性分配器
pub mod bump;

// 链表分配器
pub mod linked_list;

// 块分配器
pub mod fixed_size_block;

#[global_allocator]
static ALLOCATOR: Locked<FixedSizeBlockAllocator> = Locked::new(FixedSizeBlockAllocator::new());
// static ALLOCATOR: Locked<BumpAllocator> = Locked::new(BumpAllocator::new());

// pub struct Dummy;

// unsafe impl  GlobalAlloc for Dummy{
//     unsafe fn alloc(&self,_layout:Layout)->*mut u8{
//         null_mut()
//     }

//     unsafe fn dealloc(&self,_ptr:*mut u8,_layout:Layout){
//         panic!("dealloc should be never called");
//     }
// }

pub const HEAP_START: usize = 0x_4444_4444_0000;
pub const HEAP_SIZE: usize = 100 * 1024; // 100k

pub fn init_heap(
    mapper: &mut impl Mapper<Size4KiB>,
    frame_allocator: &mut impl FrameAllocator<Size4KiB>,
) -> Result<(), MapToError<Size4KiB>> {
    // 映射
    let page_range = {
        let heap_start = VirtAddr::new(HEAP_START as u64);
        let heap_end = heap_start + HEAP_SIZE - 1u64;
        let heap_start_page = Page::containing_address(heap_start);
        let heap_end_page = Page::containing_address(heap_end);

        Page::range_inclusive(heap_start_page, heap_end_page)
    };

    for page in page_range {
        let frame = frame_allocator
            .allocate_frame()
            .ok_or(MapToError::FrameAllocationFailed)?;

        let flags = PageTableFlags::PRESENT | PageTableFlags::WRITABLE;

        unsafe { mapper.map_to(page, frame, flags, frame_allocator)?.flush() };
    }

    unsafe { ALLOCATOR.lock().init(HEAP_START, HEAP_SIZE) }

    Ok(())
}

pub struct Locked<A> {
    inner: spin::Mutex<A>,
}

impl<A> Locked<A> {
    pub const fn new(inner: A) -> Self {
        Locked {
            inner: spin::Mutex::new(inner),
        }
    }

    pub fn lock(&self) -> spin::MutexGuard<A> {
        self.inner.lock()
    }
}

fn align_up(addr: usize, align: usize) -> usize {
    // let remainder =  addr%align;

    // if remainder == 0{
    //     addr
    // }else{
    //     addr - remainder+align
    // }
    // 要求align是2的幂
    // 因为是2的幂 所以只有1位是1 0b000100000 减一就是低位全变成1
    // 取反 那就是只有低于原先数值位的是0  其他都是1
    // 按位and 也就是向下对齐 也就是所有低于原值的位都变为0 然后 因为是addr+align-1 所以肯定是大于align的 且必然是倍数对齐(低位都是0 也就是没有余数)
    (addr + align - 1) & !(align - 1)
}
