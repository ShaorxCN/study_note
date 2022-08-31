mod timer;
use std::{
    io::{self, Stderr, Write},
    thread,
    time::{Duration, Instant},
};

use self::timer::Timer;
use crossbeam::channel::Receiver;
use crossterm::{
    cursor, execute,
    style::{self, Color, PrintStyledContent, Stylize},
    terminal::{Clear, ClearType},
};
use std::io::Result;

pub trait TimeOutput {
    fn as_time(&self) -> String;
}

impl TimeOutput for u64 {
    fn as_time(&self) -> String {
        let (hours, left) = (*self / 3600, *self % 3600);
        let (minutes, seconds) = (left / 60, left % 60);
        format!("{}:{:02}:{:02}", hours, minutes, seconds)
    }
}
fn output_progress(stderr: &mut Stderr, bytes: usize, elapsed: String, rate: f64) {
    let bytes = style::style(format!("{} ", bytes)).with(Color::Red);
    let elapsed = style::style(elapsed).with(Color::Green);
    let rate = style::style(format!(" [{:.0}b/s]", rate)).with(Color::Blue);
    let _ = execute!(
        stderr,
        cursor::MoveToColumn(0),
        Clear(ClearType::CurrentLine),
        PrintStyledContent(bytes),
        PrintStyledContent(elapsed),
        PrintStyledContent(rate)
    );
    let _ = stderr.flush();
}

pub fn stats_loop(silent: bool, stats_rx: Receiver<usize>) -> Result<()> {
    let mut total_num = 0;
    let start = Instant::now();
    let mut timer = Timer::new();
    let mut stderr = io::stderr();
    let mut total_period = 0;
    loop {
        let num_bytes = stats_rx.recv().unwrap();
        timer.update();
        total_num += num_bytes;
        total_period += num_bytes;

        thread::sleep(Duration::from_millis(500));

        if !silent && timer.ready {
            timer.ready = false;
            let rate = total_period as f64 / timer.delta.as_secs_f64();
            total_period = 0;
            output_progress(
                &mut stderr,
                total_num,
                start.elapsed().as_secs().as_time(),
                rate,
            );
        }

        if num_bytes == 0 {
            break;
        }
    }

    if !silent {
        eprintln!();
    }

    Ok(())
}
