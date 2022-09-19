use async_std::io::{Read, Write};
use async_std::prelude::*;
use async_std::task;
use std::fs;
use std::marker::Unpin;
use std::time::Duration;

pub async fn handle_connection(mut stream: impl Read + Write + Unpin) {
    let mut buffer = [0; 1024];

    stream.read(&mut buffer).await.unwrap();
    let get = b"GET / HTTP/1.1\r\n";
    let sleep = b"GET /sleep HTTP/1.1\r\n";
    let (status_line, filename) = if buffer.starts_with(get) {
        ("HTTP/1.1 200 OK\r\n\r\n", "static/hello.html")
    } else if buffer.starts_with(sleep) {
        task::sleep(Duration::from_secs(5)).await;
        ("HTTP/1.1 200 OK\r\n\r\n", "static/hello.html")
    } else {
        ("HTTP/1.1 404 NOT FOUND\r\n\r\n", "static/404.html")
    };
    let contents = fs::read_to_string(filename).unwrap();

    let response = format!("{}{}", status_line, contents);

    stream.write(response.as_bytes()).await.unwrap();
    stream.flush().await.unwrap();
}



#[cfg(test)]
mod tests{
    use super::*;
    use futures::io::Error;
    use futures::task::{Context,Poll};
    use std::cmp::min;
    use std::pin::Pin;

    struct MockTcpStream{
        read_data:Vec<u8>,
        write_data:Vec<u8>,
    }


    impl Read for MockTcpStream{
        fn poll_read(self:Pin<&mut Self>,_:&mut Context,buf:&mut [u8])->Poll<Result<usize,Error>>{
            let size:usize = min(self.read_data.len(),buf.len());
            buf[..size].copy_from_slice(&self.read_data[..size]);
            Poll::Ready(Ok(size))
        }
    }

    impl Write for MockTcpStream{
        fn poll_write(mut self:Pin<&mut Self>,_:&mut Context,buf:&[u8])->Poll<Result<usize,Error>>{
            self.write_data = Vec::from(buf);
            Poll::Ready(Ok(buf.len()))
        }

        fn poll_flush(self:Pin<&mut Self>,_:&mut Context)->Poll<Result<(),Error>>{
            Poll::Ready(Ok(()))
        }

        fn poll_close(self:Pin<&mut Self>,_:&mut Context)->Poll<Result<(),Error>>{
            Poll::Ready(Ok(()))
        }
    }

    use std::marker::Unpin;
    impl Unpin for MockTcpStream{}

    use std::fs;

    #[async_std::test]
    async fn test_handle_connection() {
        let input_bytes = b"GET / HTTP/1.1\r\n";
        let mut contents = vec![0u8; 1024];
        contents[..input_bytes.len()].clone_from_slice(input_bytes);
        let mut stream = MockTcpStream {
            read_data: contents,
            write_data: Vec::new(),
        };

        handle_connection(&mut stream).await;

        let expected_contents = fs::read_to_string("static/hello.html").unwrap();
        let expected_response = format!("HTTP/1.1 200 OK\r\n\r\n{}", expected_contents);
        assert!(stream.write_data.starts_with(expected_response.as_bytes()));
    }


}