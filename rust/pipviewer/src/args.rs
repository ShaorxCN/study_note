use clap::{App, Arg};
use std::env;
pub struct Args {
    pub infile: String,
    pub outfile: String,
    pub silent: bool,
}

impl Args {
    pub fn parse() -> Self {
        let matchs = App::new("pipviewer")
            .arg(Arg::with_name("infile").help("Read from a file instead of stdin"))
            .arg(
                Arg::with_name("outfile")
                    .short('o')
                    .long("outfile")
                    .takes_value(true)
                    .help("Write output to a file instead of stdout"),
            )
            .arg(Arg::with_name("silent").short('s').long("silent"))
            .get_matches();
        let infile = matchs.value_of("infile").unwrap_or_default().to_string();
        let outfile = matchs.value_of("outfile").unwrap_or_default().to_string();
        let silent = if matchs.is_present("silent") {
            true
        } else {
            !env::var("PV_SILENT").unwrap_or_default().is_empty()
        };
        Self {
            infile,
            outfile,
            silent,
        }
    }
}
