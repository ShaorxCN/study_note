use crossbeam::channel::{bounded, unbounded};
use pipviewer::{args::Args, read, stats, write};
use std::io::Result;
use std::thread;
fn main() -> Result<()> {
    let args = Args::parse();

    // 解构结构体
    let Args {
        infile,
        outfile,
        silent,
    } = args;

    // unbounded 用于read和stats通信 只是传输数据大小
    let (stats_tx, stats_rx) = unbounded();
    // bounded 同于read -> write
    let (write_tx, write_rx) = bounded(1024);

    let read_handle = thread::spawn(move || read::read_loop(&infile, stats_tx, write_tx));
    let stats_handle = thread::spawn(move || stats::stats_loop(silent, stats_rx));
    let write_handle = thread::spawn(move || write::write_loop(&outfile, write_rx));

    let read_io_result = read_handle.join().unwrap();
    let write_io_result = write_handle.join().unwrap();
    let stats_io_result = stats_handle.join().unwrap();

    read_io_result?;
    write_io_result?;
    stats_io_result?;

    Ok(())
}
