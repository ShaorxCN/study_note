use std::fmt::Debug;
use std::fmt::Display;
use std::result;

// use summary;
use trait_life::src_a;
use trait_life::Summary;

fn main() {
    let number_list = vec![34, 50, 25, 100, 65];
    let res = largest_with_ref(&number_list);
    println!("largest in {:?} is {}", number_list, res);

    let number_list = vec![102, 34, 6000, 89, 54, 2, 43, 8];

    let res = largest_with_ref(&number_list);
    println!("largest in {:?} is {}", number_list, res);

    let wont_work = Point { x: 5, y: 4.0 };
    let wont_work_sec = Point { x: 5, y: 4 };
    let wont_work_third = Point { x: 5, y: "hah" };
    println!("wont_work_sec.x = {}", wont_work_sec.x());

    // println!("wont_work.x = {}", wont_work.x()); // tag A
    println!("mixup = {:?}", wont_work.mixup(wont_work_third));

    // 这里因为在lib里pub use 了 summary::smy::* 所以可以直接使用  否则
    // trait_life::summary::smy::Tweet
    let tweet = trait_life::Tweet {
        username: String::from("horse_ebooks"),
        content: String::from("of course, as you probably already know, people"),
        reply: false,
        retweet: false,
    };

    println!("1 new tweet: {}", tweet.summarize());

    trait_life::gg::test(); // binary and lib d独立的crate

    let testpp = trait_life::TestP {
        content: String::from("horse_ebooks"),
    };
    // 作为方法是可以::调用的  但是不会自动解引用啥的
    println!("{}", trait_life::TestP::summarize(&testpp));
    println!("{}", testpp.summarize());

    src_a::aecho();
    notify(tweet, testpp);

    let string1 = String::from("longer");
    let result;
    {
        let string2 = String::from("xyz1234");
        result = longest_with_life(string1.as_str(), string2.as_str());
        println!("the longest is {}", result);
    }
    // 这里不行  调用 longest_with_life 会是按照string2的声明周期确认'a 所以生命周期不够
    //println!("the longest is {}", result);
}

// Copy保证list[0]可以直接使用
fn largest<T: PartialOrd + Copy>(list: &[T]) -> T {
    let mut largest = list[0];

    for &elem in list.iter() {
        if elem > largest {
            largest = elem;
        }
    }

    largest
}

fn largest_with_ref<T: PartialOrd>(list: &[T]) -> &T {
    let mut largest = &list[0];

    let mut index = 0;
    while index < list.len() {
        if list[index] > *largest {
            largest = &list[index]
        }

        index += 1;
    }

    largest
}

fn longest<'a>(x: &'a str, y: &'a str) -> &'a str {
    if x.len() > y.len() {
        x
    } else {
        y
    }
}
// fn largest<T>(list: &[T]) -> T {
//     let mut largest = list[0];

//     for &elem in list.iter() {
//         if elem > largest {
//             largest = elem;
//         }
//     }

//     largest
// }

// 生命周期 'a 即告诉编译器 x y和返回值拥有相同的生命周期 那么就是取交集
fn longest_with_life<'a>(x: &'a str, y: &'a str) -> &'a str {
    if x.len() > y.len() {
        x
    } else {
        y
    }
}

// 使用推导Debug
#[derive(Debug)]
struct Point<T, M> {
    x: T,
    y: M,
}

// impl后跟随<T,M>代表为泛型实现 编译器会知道T,M 是泛型标记而不是具体类型
// impl Potin(i32,i32) 则代表i32的实现
impl<T, M> Point<T, M> {
    fn x(&self) -> &T {
        &self.x
    }

    // 代表接受泛型参数
    fn mixup<V, W>(self, other: Point<V, W>) -> Point<T, W> {
        Point {
            x: self.x,
            y: other.y,
        }
    }
}

// 此处如果仅有下面这个实现 tag A的地方报错 因为根据类型没有具体实现 所以认为找不到.方法在i32 i32 Point的实现
// impl Point<i32, i32> {
//     fn x(&self) -> &i32 {
//         &self.x
//     }
// }

// pub fn notify(item: impl Summary) {
//     println!("Breaking news! {}", item.summarize());
// }

// 此处item 和 item2 可以是不同类型 相同trait
pub fn notify(item: impl Summary, _item2: impl Summary) {
    println!("Breaking news! {}", item.summarize());
}

// 这边 item 和 item2 必须是同类型
// pub fn notify<T: Summary>(item: T, item2: T) {
//     println!("Breaking news! {},{}", item.summarize(), item2.summarize());
// }

// pub fn notify2<T: Summary + Display>(item: T) {}

// 如果需要多个trait 那么可以使用where从句
// fn some_function<T, U>(t: T, u: U) -> impl Summary
// where
//     T: Display + Clone + Summary,
//     U: Clone + Debug,
// {
//     // 使用默认值
//     trait_life::Tweet::default()

//     // 如果返回的是trait 但是还是不能返回两个不同的类型 不行可以放在result 或者 option?
//     // if true {
//     //     trait_life::NewsArticle {
//     //         headline: String::from("Penguins win the Stanley Cup Championship!"),
//     //         location: String::from("Pittsburgh, PA, USA"),
//     //         author: String::from("Iceburgh"),
//     //         content: String::from(
//     //             "The Pittsburgh Penguins once again are the best
//     //         hockey team in the NHL.",
//     //         ),
//     //     }
//     // } else {
//     //     trait_life::Tweet {
//     //         username: String::from("horse_ebooks"),
//     //         content: String::from("of course, as you probably already know, people"),
//     //         reply: false,
//     //         retweet: false,
//     //     }
//     // }
// }

// 生命周期需要写在泛型前面
struct test_mul<'a, 'b, T, M> {
    x: T,
    y: M,
    name: &'a str,
    content: &'b str,
}

// 看代码即可  参考泛型
// 生命周期忽略原则
// 1.每个输入参数一个独立的声明周期 比如 入参x,y 那么就是x 'a  y'b
// 2.当仅有一个入参 那么'a 就赋予所有出参
// 3.多个入参 其中有&self 或者 &mut self 那么所有出参就是self的生命周期赋予

// 所有字面量有一个默认的static 生命周期 'static let s: &'static str = "I have a static lifetime.";
// 我们也可以声明  但是最好不要 因为一时确认是否他真的一直存活
// 二是 他是否真的需要一直存活

impl<'a, 'b, T, M> test_mul<'a, 'b, T, M> {
    fn level(&self) -> i32 {
        3
    }
}

// 同时使用泛型 生命周期和 trait约束
fn longest_with_an_announcement<'a, T>(x: &'a str, y: &'a str, ann: T) -> &'a str
where
    T: Display,
{
    println!("Announcement! {}", ann);
    if x.len() > y.len() {
        x
    } else {
        y
    }
}
