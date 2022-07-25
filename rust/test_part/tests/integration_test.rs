use test_part;
mod common;
#[test]
fn it_adds_two() {
    common::setup();
    assert_eq!(11, test_part::internal_fn());
}

//cargo test --test integration_test 指定集成测试的文件名
// 如果只包含main.rs的binary是无法使用tests下的集成测试的 所以一般main中都是简单启动 然后主要写在lib.rs
