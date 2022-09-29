use std::io::{Read, Write};
use std::net::TcpListener;
fn main() {
    let listener = TcpListener::bind("127.0.0.1:8089").unwrap();
    println!("Running on port 8089...");

    for stream in listener.incoming() {
        let mut stream = stream.unwrap();
        let mut buffer = [0; 1024];

        if let Ok(len) = stream.read(&mut buffer) {
            stream.write(&mut buffer[..len]).unwrap();
            continue;
        }

        println!("error on read from  client...");
    }
}
