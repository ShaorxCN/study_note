use std::{fmt, ops::Add, slice};
use supertrait::*;

// 静态变量 不可变静态变量拥有固定地址 命名大写蛇模式
static HELLO_WORLD: &str = "Hello,world";
// 读写可变静态变量都是不安全的 都需要在unsafe代码块中
static mut COUNTER: u32 = 0;

fn main() {
    test_raw_pointer();
    test_dangerous();
    test_extern();
    println!("name is {}", HELLO_WORLD);
    add_to_count(3);
    unsafe {
        println!("COUNTER:{}", COUNTER);
    }

    test_reload_add();
    test_ambiguity();
    test_supertrait();
    test_newtype_trait();
}

fn test_raw_pointer() {
    let mut num = 5;
    // 可以在正常代码中创建 但是只能在unsafe代码块中使用
    // 这里其实还是有效的指针
    let r1 = &num as *const i32;
    let r2 = &mut num as *mut i32;

    // 这里的未必有效
    let address = 0x012345usize;
    let r = address as *const i32;
    // 这里是无法使用的
    //println!("r1 is :{}", *r1);

    unsafe {
        println!("r1 is :{}", *r1); // 5
        println!("r1 is :{}", *r2); // 5
        *r2 = 10;
        println!("r1 is :{}", *r1); // 10
        println!("r1 is :{}", *r2); // 10
    }
}

fn test_dangerous() {
    let mut v = vec![1, 2, 3, 4, 5];
    let r = &mut v[..];

    let (a, b) = r.split_at_mut(3);

    assert_eq!(a, &mut [1, 2, 3]);
    assert_eq!(b, &mut [4, 5]);

    let (c, d) = split_at_mut(r, 3);
    println!("{:?},{:?}", c, d);
}

// fn split_at_mut(slice: &mut [i32], mid: usize) -> (&[i32], &[i32]) {
//     let len = slice.len();

//     assert!(mid <= len);

//     (&slice[..mid], &slice[mid..])
// }

fn split_at_mut(slice: &mut [i32], mid: usize) -> (&mut [i32], &mut [i32]) {
    let len = slice.len();
    let ptr = slice.as_mut_ptr();

    assert!(mid <= len);

    unsafe {
        (
            // 从ptr处创建一个含有mid个元素的切片
            slice::from_raw_parts_mut(ptr, mid),
            slice::from_raw_parts_mut(ptr.offset(mid as isize), len - mid),
        )
    }
}

// extern的代码都是不安全的 毕竟外部语言代码 有自己的规范
extern "C" {
    fn abs(input: i32) -> i32;
}

fn test_extern() {
    unsafe {
        println!("Absolute value of -3 according to C:{}", abs(-3));
    }
}

// 禁止在编译期间修改函数名字
// c中调用该函数
#[no_mangle]
pub extern "C" fn call_from_c() {
    println!("Just called a Rust function from C!");
}

fn add_to_count(inc: u32) {
    unsafe {
        COUNTER += inc;
    }
}

// unsafe trait Foo {}

// unsafe impl Foo for i32 {}

// 这是重载运算符
#[derive(Debug, PartialEq)]
struct Point {
    x: i32,
    y: i32,
}

//
impl Add for Point {
    type Output = Point;
    fn add(self, other: Point) -> Point {
        Point {
            x: self.x + other.x,
            y: self.y + other.y,
        }
    }
}

// 这里是默认实现
impl OutlinePrint for Point {}

impl fmt::Display for Point {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "({}, {})", self.x, self.y)
    }
}
fn test_reload_add() {
    assert_eq!(
        Point { x: 1, y: 0 } + Point { x: 2, y: 3 },
        Point { x: 3, y: 3 }
    );

    assert_eq!(Millimeters(100) + Meters(1), Millimeters(1100));
}

struct Dog;

trait Animal {
    fn baby_name() -> String;
}
impl Dog {
    fn baby_name() -> String {
        String::from("Spot")
    }
}

impl Animal for Dog {
    fn baby_name() -> String {
        String::from("puppy")
    }
}

fn test_ambiguity() {
    println!("A baby dog is called a {}", Dog::baby_name());
    println!(
        "A baby dog as annimal is called a {}",
        <Dog as Animal>::baby_name()
    );
}

trait OutlinePrint: fmt::Display {
    fn outline_print(&self) {
        let output = self.to_string();
        let len = output.len();
        println!("{}", "*".repeat(len + 4));
        println!("*{}*", " ".repeat(len + 2));
        println!("* {} *", output);
        println!("*{}*", " ".repeat(len + 2));
        println!("{}", "*".repeat(len + 4));
    }
}

fn test_supertrait() {
    let p = Point { x: 1, y: 0 };
    p.outline_print();
}

struct Wrapper(Vec<String>);
impl fmt::Display for Wrapper {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "[{}]", self.0.join(", "))
    }
}

fn test_newtype_trait() {
    let w = Wrapper(vec![String::from("hello"), String::from("world")]);
    println!("w = {}", w);
    // 报错
    // println!("w = {}", w.0)
}
