fn main() {
    let rect1 = Rectangle {
        width: 30,
        height: 40,
    };
    println!("the area of {:#?} is {}", rect1, rect1.area());

    let c = Coin::Quarter(UsState::Bla);
    coin_test(c);
}

#[derive(Debug)]
struct Rectangle {
    width: u64,
    height: u64,
}

impl Rectangle {
    fn area(&self) -> u64 {
        self.width * self.height
    }
}

#[derive(Debug)]
enum UsState {
    Ala,
    Bla,
    Cla,
}

enum Coin {
    Penny,
    Nickel,
    Quarter(UsState),
}

fn coin_test(c: Coin) -> u32 {
    let mut count = 0;
    if let Coin::Quarter(UsState::Cla) = c {
        println!("get cla");
    } else {
        count += 1;
    }

    println!("{}", count);

    return count;
}
