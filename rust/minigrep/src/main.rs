use std::{env, process};

use minigrep::Config;

fn main() {
    // 第一个是程序自己  |...|exp闭包  ...代表arguement
    let config = Config::new(env::args()).unwrap_or_else(|err| {
        eprintln!("Problem parsing arguments:{}", err);
        process::exit(1);
    });

    // if  let
    if let Err(e) = minigrep::run(config) {
        eprintln!("Application error: {}", e);
        process::exit(1);
    }
}
