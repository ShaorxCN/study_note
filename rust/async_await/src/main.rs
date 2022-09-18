 use async_await::{executor::*, timer::TimerFuture, web_server::*};
// use std::time::Duration;

use async_std::net::TcpListener;
use futures::stream::StreamExt;

// fn main() {
    // let (executor, spawner) = new_executor_and_spawner();
    // spawner.spawn(async {
    //     println!("start");
    //     TimerFuture::new(Duration::new(2, 0)).await;
    // });

    // spawner.spawn(async {
    //     println!("go");
    //     TimerFuture::new(Duration::new(1, 0)).await;
    //     println!("done");
    // });

    // drop(spawner);
    // executor.run();
//  }


#[async_std::main]
async fn main() {
    let listener = TcpListener::bind("127.0.0.1:7878").await.unwrap();
    listener.incoming().for_each_concurrent(None,|tcpstream| async move{
        let tcpstream = tcpstream.unwrap();
        handle_connection(tcpstream).await;
    }).await;

}

