use async_await::{executor::*, timer::TimerFuture};
use std::time::Duration;
fn main() {
    let (executor, spawner) = new_executor_and_spawner();
    // 这里是将async的部分变成future所以内部还是阻塞
    spawner.spawn(async{
        println!("start");
        TimerFuture::new(Duration::new(2,0)).await;
    });

    spawner.spawn(async{
        println!("go");
        TimerFuture::new(Duration::new(1,0)).await;
        println!("done");
    });

    drop(spawner);
    executor.run();
}
