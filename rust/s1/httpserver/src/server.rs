use http::httprequest::HttpRequest;

use crate::router::Router;
use std::{io::Read, net::TcpListener};
pub struct Server<'a> {
    socket_addr: &'a str,
    router: Router,
}

impl<'a> Server<'a> {
    pub fn new(addr: &'a str, r: Router) -> Self {
        Server {
            socket_addr: addr,
            router: r,
        }
    }

    pub fn run(&self) {
        let listener = TcpListener::bind(self.socket_addr).unwrap();

        println!("Server running on {}", self.socket_addr);

        for stream in listener.incoming() {
            let mut stream = stream.unwrap();
            println!("Connection established");

            let mut buffer = [0; 1024];
            if let Ok(len) = stream.read(&mut buffer) {
                let req_str = String::from_utf8(buffer[..len].to_vec()).unwrap();

                println!("req_str :{}", req_str);
                let req: HttpRequest = req_str.as_str().into();

                self.router.serve(&req, &mut stream);
            }
        }
    }
}
