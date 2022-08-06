use std::{thread, time::Duration};

fn main() {
    println!("Hello, world!");

    // generate_workout(113, 3);

    let x = vec![1, 2, 3];
    let equal_to_x = move |z| z == x;
    //  println!("can't use x here:{:?}", x);

    let y = vec![1, 2, 3];
    assert!(equal_to_x(y));

    let v1 = vec![1, 2, 3];

    let v1_iter = v1.iter();

    for val in v1_iter {
        println!("Got:{}", val);
    }
}

struct Cacher<T>
where
    T: Fn(u32) -> u32,
{
    calculation: T,
    value: Option<u32>,
}

impl<T> Cacher<T>
where
    T: Fn(u32) -> u32,
{
    fn new(calculation: T) -> Cacher<T> {
        Cacher {
            calculation,
            value: None,
        }
    }

    fn value(&mut self, arg: u32) -> u32 {
        match self.value {
            Some(v) => v,
            None => {
                let v = (self.calculation)(arg);
                self.value = Some(v);
                v
            }
        }
    }
}

fn generate_workout(intensity: u32, random_number: u32) {
    // 使用时再计算
    // 也可以显式的标注参数和返回类型
    //  =|num:u32|->u32{...}
    // 单独语句可以没有花括号
    // =|num|x+1 但是分号还是要给的 因为前面是let 的语句
    // let expensive_closure = |num| {
    //     println!("calculating slowly...");
    //     thread::sleep(Duration::from_secs(2));
    //     num
    // };
    let mut c = Cacher::new(|num| {
        println!("calculating slowly...");
        thread::sleep(Duration::from_secs(2));
        num
    });
    if intensity < 25 {
        println!("Today, do {} pushups!", c.value(intensity));
        println!("Next, do {} situps!", c.value(intensity));
    } else {
        if random_number == 3 {
            println!("Take a break today! Remember to stay hydrated!");
        } else {
            println!("Today, run for {} minutes!", c.value(intensity));
        }
    }
}
