//! # config
//!
//! config是一系列工具的集合，
//! 这些工具被用来简化特定的计算操作[//! 这个就是作为包裹后续注释作为父层的注释]

// pub use将内层结构暴露再外面 直接使用 use config::mix 而不是use config::utils::mix
pub use self::kinds::PrimaryColor;
pub use self::kinds::SecondaryColor;
pub use self::utils::mix;

/// 三斜杠的注释会生成文档  可以使用markdown语法
/// cargo test也会将lib中的example作为测试
/// # Example
/// ```
/// let arg = 5;
/// let answer = config::add_one(arg);
///
/// assert_eq!(6,answer);
/// ```

pub fn add_one(x: i32) -> i32 {
    x + 1
}

pub mod kinds {
    /// RYB颜色模型的三原色
    pub enum PrimaryColor {
        Red,
        Yellow,
        Blue,
    }
    pub enum SecondaryColor {
        Orange,
        Green,
        Purple,
    }
}

/// crates.io注册申请令牌  然后执行
/// ```
/// cargo login xxxxxxxxxxx[令牌]
/// ```
/// 这个命令会让Cargo将你的API令牌存入～/.cargo/credentials文件中。
///
/// ```cargo publish``` 是发布命令
///
/// 必填项见项目Cargo.toml
///
/// 修改version重新publish就是新版本
///
/// cargo yank --vers xxxx 撤销xxx版本的代码  这样会阻止新的引用 老的管不了
/// cargo yank --vers xxxx --undo 回退撤销
pub mod utils {
    use crate::kinds::*;
    /// 将两种等量的原色混合生成调和色
    pub fn mix(c1: PrimaryColor, c2: PrimaryColor) -> SecondaryColor {}
}
