use crate::List::{Cons, Nil};
use std::ops::Deref;
use std::{cell::RefCell, rc::Rc};
enum List {
    // 如果直接是(i32,List) enum在检查大小时寻找最大 发现是一个递归  无法确定大小 报错
    Cons(i32, Box<List>),
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
    test_rc_list();
    test_ref_list();
}

fn hello(name: &str) {
    println!("Hello, {}!", name);
}

#[derive(Debug)]
enum RcList {
    Cons(i32, Rc<RcList>),
    Nil,
}
use crate::RcList::{Cons as RcCons, Nil as RcNil};

fn test_rc_list() {
    let a = Rc::new(RcCons(5, Rc::new(RcCons(10, Rc::new(RcNil)))));
    println!("count after creating a:{}", Rc::strong_count(&a));
    let b = RcCons(3, Rc::clone(&a));
    println!("count after linking b:{}", Rc::strong_count(&a));
    {
        let _c = RcCons(4, Rc::clone(&a));
        println!("count after linking c:{}", Rc::strong_count(&a));
    }
    println!("count after c goes out  :{}", Rc::strong_count(&a));
    println!("{:?}", b);
}

use crate::RefList::{Cons as RefCons, Nil as RefNil};
#[derive(Debug)]
enum RefList {
    Cons(Rc<RefCell<i32>>, Rc<RefList>),
    Nil,
}

fn test_ref_list() {
    let value = Rc::new(RefCell::new(5));
    // Rc 实现了deref 所以会自动解引用 cell::RefCell无
    // 所以这里的&value其实时可以自动解引用转换为RefCell的
    let a = Rc::new(RefCons(Rc::clone(&value), Rc::new(RefNil)));

    let b = RefCons(Rc::new(RefCell::new(6)), Rc::clone(&a));
    let c = RefCons(Rc::new(RefCell::new(10)), Rc::clone(&a));
    // 这里是自动解引用了
    *value.borrow_mut() += 10;

    println!("a after = {:?}", a);
    println!("b after = {:?}", b);
    println!("c after = {:?}", c);
}

use smart_pointer::TestList::{Cons as TCons, Nil as TNil};
#[test]
fn circle() {
    let a = Rc::new(TCons(5, RefCell::new(Rc::new(TNil))));

    println!("a initial rc count = {}", Rc::strong_count(&a));
    println!("a next item = {:?}", a.tail());

    let b = Rc::new(TCons(10, RefCell::new(Rc::clone(&a))));

    println!("a rc count after b creation = {}", Rc::strong_count(&a));
    println!("b initial rc count = {}", Rc::strong_count(&b));
    println!("b next item = {:?}", b.tail());

    // 循环引用
    if let Some(link) = a.tail() {
        *link.borrow_mut() = Rc::clone(&b);
    }

    println!("b rc count after changing a = {}", Rc::strong_count(&b));
    println!("a rc count after changing a = {}", Rc::strong_count(&a));
    // 这里循环引用了
    // println!("a next item = {:?}", a.tail());
}
