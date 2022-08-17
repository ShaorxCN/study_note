use hello_macro_derive::HelloMacro; // 导入宏
use router_macro::{make_hello, route}; // 导入宏
use std::{fmt, ops::Add, slice};
use supertrait::*; // 这里因为* 其实也导入了HelloMacro的trait

// 静态变量 不可变静态变量拥有固定地址 命名大写蛇模式
static HELLO_WORLD: &str = "Hello,world";
// 读写可变静态变量都是不安全的 都需要在unsafe代码块中
static mut COUNTER: u32 = 0;
make_hello!(evan);
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
    println!("{}", do_twice(add_one, 2));
    test_fn_pointer();
    test_proc_macro();
    index();

    // make_hello!(evan)生成
    hello_evan();
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

// fn test_DST() {
//     let s1: str = "Hello there!";
//     let s2: str = "How's it going?";
// }

fn add_one(x: i32) -> i32 {
    x + 1
}

fn do_twice(f: fn(i32) -> i32, arg: i32) -> i32 {
    f(arg) + f(arg)
}

#[derive(Debug)]
enum Status {
    Value(u32),
    Stop,
}

fn test_fn_pointer() {
    let list_of_numbers = vec![1, 2, 3];
    // let list_of_strings: Vec<String> = list_of_numbers.iter().map(|i| i.to_string()).collect();
    let list_of_strings: Vec<String> = list_of_numbers.iter().map(ToString::to_string).collect();
    println!("{:?}", list_of_numbers);
    // 0u32指定u32 否则默认i32?
    let list_of_statuses: Vec<Status> = (0u32..20).map(Status::Value).collect();
    println!("{:?}", list_of_statuses);
}

// export 表示引入包即可用
// $()中代表匹配模式 匹配到的内容将被替换
// 这expr 表示任何都可以匹配 然后命名为$x
// ,表示匹配代码后面可能有逗号分隔符
// *表示可以匹配0个或者多个之前的代码
// 看内容 先创建一个temp_vec
// $()*为每个匹配到的$()生成执行$()*中的代码 push
#[macro_export]
macro_rules! vec {
    ($( $x:expr ),* ) => {
           {
               let mut temp_vec = Vec::new();
            $(
                 temp_vec.push($x);
               )*
            temp_vec
           }
       };
}

// 用vec![1,2,3]举例 等价于
// {
//     let mut temp_vec = Vec::new();
//     temp_vec.push(1);
//     temp_vec.push(2);
//     temp_vec.push(3);
//     temp_vec
// }
#[derive(HelloMacro)]
struct Pancakes;

fn test_proc_macro() {
    Pancakes::hello_macro();
}

#[route(GET, "/")]
fn index() {
    println!("this is a route fn");
}
