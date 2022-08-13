use rand::Rng;
use std::{cmp::Ordering, io};
// use std::io::{self,Write};

// const MAX_NUMBER: i64 = 7 + 9;
fn main() {
    println!("Guess the number!");
    /*
    do some comment
    */
    let secret_number = rand::thread_rng().gen_range(1, 101);
    println!("the secret_number is:{}", secret_number);
    let str = "123";
    let tup = (1, 23, 4);
    println!("len:{},{},{}", str.len(), tup.0, tup.1);

    let av = [3; 5];
    println!("{}", av[1]);

    loop {
        println!("Please input your number:");
        let mut guess = String::new();
        io::stdin()
            .read_line(&mut guess)
            .expect("Failed to read line");

        let guess: i64 = guess.trim().parse().expect("Please input a number");

        println!("You guessed:{}", guess);

        match guess.cmp(&secret_number) {
            Ordering::Less => println!("Too small!"),
            Ordering::Greater => println!("Too big!"),
            Ordering::Equal => {
                println!("You win");
                break;
            }
        }
    }

    anther_function(1);
    let res = plus_function(2);
    println!("result:{}", res);

    if res > 7 {
        println!("{}", 1);
    } else {
        println!("hah");
    }

    loop_test();

    owner_test();

    let mut s2 = String::from("evan");
    let r1 = &mut s2;
    //  let r3 = &s2;
    // let r2 = &mut s2;
    let length = owner_len(r1);
    // s2.push_str("hah");
    println!("{} len = {}", s2, length);

    let s = String::from("hello");

    let len = s.len();
    let slice = &s[3..len];
    let slice2 = &s[3..];

    println!("{},{}", slice, slice2);

    struct_test(String::from("123"), String::from("evan"));
}

fn anther_function(x: isize) {
    println!("{}", x);
}

// 显示return可以用语句
// 否则表达式
fn plus_function(x: i32) -> i32 {
    x + 1 // return x+1
}

fn loop_test() {
    let a = [10, 20, 30, 40, 50];

    for element in a.iter() {
        println!("the value is {}", element);
    }

    // 1 2 3
    for number in (1..4).rev() {
        println!("{}", number);
    }

    let mut str = "123";
    println!("{}", str);
    str = "234";

    println!("{}", str);
}

fn owner_test() {
    let s1 = String::from("Hello");
    let s2 = s1.clone();

    println!("{}", s1);
    println!("{}", s2);
}

fn owner_len(s: &mut String) -> usize {
    s.push_str(",nice");
    s.len()
}

// fn dangle() -> String {
//     let s = String::from("nice");

//     s
// }

// slice is immutable  so can't have a mutable
// fn slice_test() {
//     let mut s = String::from("hello,");
//     let rs1 = &s[..2];
//     s.push_str("world"); // like push_str(&mut self, string: &str)

//     println!("{}", s);
//     println!("{}", rs1);
// }

struct User {
    email: String,
    username: String,
    active: bool,
    sign_in_count: u64,
}

// struct User2 {
//     email: &str,
//     active: bool,
// }

fn struct_test(email: String, username: String) -> User {
    let u1 = User {
        email,
        username,
        active: true,
        sign_in_count: 1,
    };

    println!("{}", u1.email);

    let u2 = User {
        active: true,
        sign_in_count: 1,
        ..u1
    };

    println!("{}", u2.username);

    return u2;
}
