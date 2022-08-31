use crate::CHUNK_SIZE;
use crossbeam::channel::Sender;
use std::fs::File;
use std::io::{self, BufReader, Read, Result};

pub fn read_loop(infile: &str, stats_tx: Sender<usize>, write_rx: Sender<Vec<u8>>) -> Result<()> {
    // 指定输出文件或则和默认标准输出
    let mut reader: Box<dyn Read> = if !infile.is_empty() {
        Box::new(BufReader::new(File::open(infile)?))
    } else {
        Box::new(BufReader::new(io::stdin()))
    };

    // CHUNK_SIZE 长度的0数组
    let mut buffer = [0; CHUNK_SIZE];
    loop {
        let num_read = match reader.read(&mut buffer) {
            Ok(0) => break,
            Ok(x) => x,
            Err(_) => break,
        };

        let _ = stats_tx.send(num_read);

        //  fn from(s: &[T]) -> Vec<T>
        if write_rx.send(Vec::from(&buffer[..num_read])).is_err() {
            break;
        }
    }

    let _ = stats_tx.send(0);
    let _ = write_rx.send(Vec::new());
    Ok(())
}
