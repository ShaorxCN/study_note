use crate::List::{Cons, Nil};
use std::ops::Deref;
enum List {
    // 如果直接是(i32,List) enum在检查大小时寻找最大 发现是一个递归  无法确定大小 报错
    Cons(i32, Box<List>),
    Nil,
}

#[derive(Debug)]
enum RcList {
    Cons(i32, Rc<RcList>),
    Nil,
}

struct MyBox<T: std::fmt::Debug>(T);

impl<T: std::fmt::Debug> MyBox<T> {
    fn new(x: T) -> MyBox<T> {
        MyBox(x)
    }
}

impl<T: std::fmt::Debug> Deref for MyBox<T> {
    type Target = T;

    fn deref(&self) -> &T {
        &self.0
    }
}

// 后创建的先Drop
impl<T: std::fmt::Debug> Drop for MyBox<T> {
    fn drop(&mut self) {
        println!("drop value:{:?}", self.0);
    }
}

fn main() {
    let b = Box::new(5);
    // 离开时栈上指针和堆上数据都会Drop
    println!("{}", b);

    let _list = Cons(1, Box::new(Cons(2, Box::new(Cons(3, Box::new(Nil))))));

    // let x = 5;
    // let y = &x;

    // let z = Box::new(x);

    // assert_eq!(5, x);
    // //  assert_eq!(5, y);
    // assert_eq!(5, *y);
    // assert_eq!(5, *z);

    let x = 5;

    let z = MyBox::new(x);

    assert_eq!(5, x);
    assert_eq!(5, *z);
    // 提前drop
    drop(z);
    let m = MyBox::new(String::from("Rust"));
    // 自动解引用转换   &T可以自动转换为T的deref
    // 所以这个就等价于 hello(&(*m)[..])
    hello(&m);

    // 当多个域需要持有同一个对象的时候 可以使用Rc<T>
    TestRcList();
}

fn hello(name: &str) {
    println!("Hello, {}!", name);
}

use crate::RcList::{Cons as RcCons, Nil as RcNil};
use std::rc::Rc;
fn TestRcList() {
    let a = Rc::new(RcCons(5, Rc::new(RcCons(10, Rc::new(RcNil)))));
    println!("count after creating a:{}", Rc::strong_count(&a));
    let b = RcCons(3, Rc::clone(&a));
    println!("count after linking b:{}", Rc::strong_count(&a));
    {
        let c = RcCons(4, Rc::clone(&a));
        println!("count after linking c:{}", Rc::strong_count(&a));
    }
    println!("count after c goes out  :{}", Rc::strong_count(&a));
    println!("{:?}", b);
}
