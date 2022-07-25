// 这边去找src_a/a.rs 或者src_a/a/mod.rs
pub mod a;
// 这里pub use aecho 相当于把a的aecho提升到src_a 可用 main里 use trait_life::src_a 直接src_a::acheo
// 否则 src_a::a::aecho();
pub use a::aecho;
