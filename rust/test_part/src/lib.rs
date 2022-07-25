#[derive(Debug)]
pub struct Rectangle {
    length: u32,
    width: u32,
}

impl Rectangle {
    pub fn can_hold(&self, other: &Rectangle) -> bool {
        self.length > other.length && self.width > other.width
    }
}

pub fn add_two(a: i32) -> i32 {
    a + 2
}

pub fn greeting(name: &str) -> String {
    format!("Hello,{}", name)
}

pub trait Xixi {
    fn nice(&self) -> u32;
}

pub struct Guess {
    value: u32,
}

impl Guess {
    pub fn new(value: u32) -> Guess {
        if value < 1 || value > 100 {
            panic!("Guess value must be between 1 and 100,got {}", value);
        }

        Guess { value }
    }

    pub fn nice(&self) -> u32 {
        self.value + 1
    }
}

pub fn lele<T: Xixi>(v: T) -> u32 {
    println!("sdasdasdasdasdasds===================");
    v.nice() + 199
}

pub fn internal_fn() -> u32 {
    11
}

// 可以同名 所以impl trait 和 impl区分开来写
// 可以针对引用impl trait
impl Xixi for &Guess {
    // 自调用的self的 用x.xx 否则 x::xx
    fn nice(&self) -> u32 {
        self.value
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn it_works() {
        let result = 2 + 2;
        assert_eq!(result, 4);
    }

    #[test]
    fn larger_can_hold_smaller() {
        let larger = Rectangle {
            length: 8,
            width: 7,
        };
        let smaller = Rectangle {
            length: 5,
            width: 1,
        };
        assert!(larger.can_hold(&smaller));
    }

    #[test]
    fn it_adds_two() {
        assert_eq!(4, add_two(2));
    }
    #[test]
    fn greeting_test() {
        let result = greeting("evan");
        assert!(
            result.contains("evan"),
            "greeting did not contain name,value was `{}`",
            result
        )
    }

    #[test]
    #[should_panic(expected = "Guess value must be between ")]
    #[ignore]
    fn greater_than_100() {
        // 遇到第一个panic就停了
        let g = Guess::new(11);
        let vlaue = lele(&g);
        assert_eq!(121 + 199, vlaue);
        let res = g.nice();
        assert_eq!(121 + 1, res);
    }

    #[test]
    fn test_res() -> Result<(), String> {
        if 2 + 2 == 4 {
            Ok(())
        } else {
            Err(String::from("xixi"))
        }
    }

    #[test]
    fn internal_use() {
        // 可以测试 但是不能在cargo run main中调用
        assert_eq!(11, internal_fn());
    }
    // cargo test -- --test-threads=1 默认对线程 这个如果在有依赖的情况下使用
    // cargo test -- --nocapture  可以输出println!一类的
    // cargo test xxxx 支持匹配 cargo test re 那么 greater 匹配到 test_res greeting都能test
    // 一般mod 和 test mod放在一起 在 mod tests 上写#[cfg(test)] 这样就不会编译 只会cargo test 时候跑
}
