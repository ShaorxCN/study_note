pub mod gg {
    pub fn test() {
        println!("123");
    }
}
// 这边找了src_a.rs
pub mod src_a;

// 先是找summary.rs 找不到就去找 summary/mod.rs
pub mod summary;

pub use summary::smy::*;
