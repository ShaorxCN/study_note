use std::{
    io::{Read, Write},
    net::TcpStream,
    str,
};

fn main() {
    let mut stream = TcpStream::connect("127.0.0.1:8089").unwrap();
    let mut buf = [0; 1024];
    stream.write("Hello,evan".as_bytes()).unwrap();
    if let Ok(len) = stream.read(&mut buf) {
        println!(
            "Response from server:{:?}",
            str::from_utf8(&buf[..len]).unwrap()
        );
    } else {
        println!("error on read from  server...");
    }
}
