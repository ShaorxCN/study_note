use conquer_once::spin::OnceCell;
use crossbeam_queue::ArrayQueue;
use crate::println;
use futures_util::stream::Stream;
use core::{pin::Pin, task::{Poll, Context}};
use futures_util::task::AtomicWaker;
use futures_util::stream::StreamExt;
use pc_keyboard::{layouts, DecodedKey, HandleControl, Keyboard, ScancodeSet1};
use crate::print;

static WAKER: AtomicWaker = AtomicWaker::new();
static SCANCODE_QUEUE: OnceCell<ArrayQueue<u8>> = OnceCell::uninit();

// 仅可以在crate 也就是lib.rs中调用
pub (crate) fn add_scancode(scancode:u8){
    if let Ok(queue) = SCANCODE_QUEUE.try_get() {
        if let Err(_) = queue.push(scancode) {
            println!("WARNING: scancode queue full; dropping keyboard input");
        }else{
            WAKER.wake(); // 推送成功就唤醒 
        }
    } else {
        println!("WARNING: scancode queue uninitialized");
    }
}

pub struct ScancodeStream{
    _private:(), // 保证外部无法构建该结构 只能通过new
}

impl  ScancodeStream{

    pub fn new()->Self{
        // 确保只有一个ScancodeStream
        SCANCODE_QUEUE.try_init_once(||ArrayQueue::new(100)).expect("ScancodeStream::new should only be called once");
        ScancodeStream { _private: () }
    }
    
}

impl Stream for ScancodeStream{
    type Item = u8;

    fn poll_next(self: Pin<&mut Self>, cx: &mut Context) -> Poll<Option<u8>> {
        let queue = SCANCODE_QUEUE.try_get().expect("not initialized");

        if let Ok(scancode)= queue.pop(){
            return  Poll::Ready(Some(scancode));
        }

        // 先注册
        WAKER.register(&cx.waker());
        // 二次检查 ok就删除之前的唤醒器
        match queue.pop(){
            Ok(scancode)=>{
                WAKER.take();
                Poll::Ready(Some(scancode))
            }
            Err(crossbeam_queue::PopError) => Poll::Pending,
        }
    }   
}

pub async fn print_keypresses(){
    let mut scancodes = ScancodeStream::new();
    let mut keyboard = Keyboard::new(layouts::Us104Key, ScancodeSet1,
        HandleControl::Ignore);
    
    
    // 调用poll_next 从不返回 None 所以一直执行
    while let Some(scancode) = scancodes.next().await {
        if let Ok(Some(key_event)) = keyboard.add_byte(scancode) {
            if let Some(key) = keyboard.process_keyevent(key_event) {
                match key {
                    DecodedKey::Unicode(character) => print!("{}", character),
                    DecodedKey::RawKey(key) => print!("{:?}", key),
                }
            }
        }
    }
}