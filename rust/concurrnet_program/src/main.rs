use std::{
    sync::{mpsc, Arc, Mutex},
    thread,
    time::Duration,
};

fn main() {
    println!("=========channel start ==========");
    let handler = thread::spawn(|| {
        for i in 1..10 {
            println!("hi number {} from the spawned thread", i);
            thread::sleep(Duration::from_millis(1));
        }
    });
    // 阻塞当前线程  知道之前handler持有线程结束 这里就是等待spawned 结束再开始main的for
    // handler.join().unwrap();

    for i in 1..5 {
        println!("hi number {} from the main thread!", i);
        thread::sleep(Duration::from_millis(1));
    }

    // 阻塞当前线程  知道之前handler持有线程结束
    handler.join().unwrap();

    test_move_thread();

    use_channel();
    use_channel_send_msgs();
    println!("=========channel end===========");
    // test_forrange_with_thread();
    println!("=========mutex start ==========");
    use_mutex_single_thread();
    us_mutex_multiple_thread();
    println!("=========mutex end===========");
}

fn test_move_thread() {
    let v = vec![1, 2, 3];

    // 用move转移所有权到新起的线程里 这样就不用担心外部可能类似drop()或者自动Drop trait 手动drop回直接报错
    let handler = thread::spawn(move || {
        println!("{:?}", v);
    });

    handler.join().unwrap();
}

fn use_channel() {
    // mpsc multiple producer single consumer 多个发送端 一个接收端
    let (tx, rx) = mpsc::channel();

    thread::spawn(move || {
        let val = String::from("hi");
        tx.send(val).unwrap();
        // 并不是常规想象的发送值是复制 毕竟rust所有权。 但是感觉这个不合理？ 这里是不能尝试发送引用的
        // 因为生命周期的问题 没法确定 这边作用域结束可能就Drop了
        //println!("{}", val);
    });

    // try_recv()channel中有值则会返回值 反则返回Err但是不会阻塞
    let received = rx.recv().unwrap();
    println!("Got:{}", received);
}

fn use_channel_send_msgs() {
    let (tx, rx) = mpsc::channel();

    let tx1 = mpsc::Sender::clone(&tx);
    thread::spawn(move || {
        let vals = vec![
            String::from("hi"),
            String::from("from"),
            String::from("the"),
            String::from("thread"),
        ];

        for val in vals {
            tx1.send(val).unwrap();
            thread::sleep(Duration::from_secs(1));
        }
    });

    thread::spawn(move || {
        let vals = vec![
            String::from("more"),
            String::from("messages"),
            String::from("for"),
            String::from("you"),
        ];

        for val in vals {
            tx.send(val).unwrap();
            thread::sleep(Duration::from_secs(1));
        }
    });

    for received in rx {
        println!("Got {}", received);
    }
}

fn test_forrange_with_thread() {
    let vals = vec![
        String::from("hi"),
        String::from("from"),
        String::from("the"),
        String::from("thread"),
    ];

    // 显然这里因为所有权的问题 不会出现问题
    // 如果是引用呢 没有生命周期是会报错的
    for val in vals {
        thread::spawn(move || println!("{}", val));
    }

    thread::sleep(Duration::from_secs(10));
}

fn use_mutex_single_thread() {
    let m = Mutex::new(5);

    {
        // 阻塞当前线程 返回可变引用  默认自动解锁 在离开得时候  当然类似drop函数 也有Mutex::unlock(guard)
        let mut num = m.lock().unwrap();
        *num = 6;
    }

    println!("m = {:?}", m);
}

fn us_mutex_multiple_thread() {
    let counter = Arc::new(Mutex::new(0));
    let mut handlers = vec![];

    for _ in 0..10 {
        let counter = Arc::clone(&counter);
        let handler = thread::spawn(move || {
            let mut num = counter.lock().unwrap();
            *num += 1;
        });

        handlers.push(handler);
    }

    for handler in handlers {
        handler.join().unwrap();
    }

    println!("Result: {}", *counter.lock().unwrap());
}
