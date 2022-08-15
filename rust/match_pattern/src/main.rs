fn main() {
    let fav_color: Option<&str> = None;
    let is_tuesday = false;

    let age: Result<u8, _> = "34".parse();

    if let Some(color) = fav_color {
        println!("Using your favorite color, {}, as the background", color);
    } else if is_tuesday {
        println!("Tuesday is green day!");
    } else if let Ok(age) = age {
        if age < 30 {
            println!("Using purple as the background color");
        } else {
            // out
            println!("Using orange as the background color");
        }
    } else {
        println!("Using blue as the background color");
    }

    test_while_let();
    test_pattern_named();
    test_pattern_multiple();
    test_pattern_range();
    test_pattern_deconstruction();
    foo(0, 4);
}

fn test_while_let() {
    let mut stack = Vec::new();
    stack.push(1);
    stack.push(2);
    stack.push(3);

    while let Some(top) = stack.pop() {
        println!("{}", top);
    }
}

fn test_for() {
    let v = vec!['a', 'b', 'c'];
    for (index, value) in v.iter().enumerate() {
        println!("{} is at index {}", value, index);
    }
}

fn test_pattern_named() {
    let x = Some(5);
    let y = 10;

    match x {
        Some(50) => println!("Got 50"),
        Some(y) => println!("Matched y={:?}", y), // match 新开作用域 所以这里得y是新变量 matched
        //  Some(n) if n == y => println!("Matched y={:?}", y), // 通过if 精确匹配Some(10) 这样就是进入default case
        _ => println!("Default case, x = {:?}", x), // 这里没有新申明x 所以还是外部得Some(5)
    }
    println!("at the end: x = {:?}, y = {:?}", x, y); // Some(5),10
}

fn test_pattern_multiple() {
    let x = 1;
    match x {
        1 | 2 => println!("one or two"),
        3 => println!("three"),
        _ => println!("anything"),
    }
}

fn test_pattern_range() {
    let x = 5;
    match x {
        1..=5 => println!("one through five"),

        _ => println!("something else"),
    };

    let x = 'c';
    match x {
        'a'..='j' => println!("early ASCII letter"),
        'k'..='z' => println!("late ASCII letter"),
        _ => println!("something else"),
    }
}

struct Point {
    x: i32,
    y: i32,
}
enum Color {
    Rgb(i32, i32, i32),
    Hsv(i32, i32, i32),
}
enum Message {
    Quit,
    Move { x: i32, y: i32 },
    Write(String),
    ChangeColor(Color),
}

enum Message2 {
    Hello { id: i32 },
}

fn test_pattern_deconstruction() {
    // 结构体得解构
    let p = Point { x: 0, y: 7 };

    // let Point { x: a, y: b } = p;
    // 自动创建和字段名同名得变量
    // 等价 x:x,y:y
    let Point { x, y } = p;
    assert_eq!(0, x);
    assert_eq!(7, y);

    match p {
        Point { x, y: 0 } => println!("On the x axis at {}", x),
        Point { x: 0, y } => println!("On the y axis at {}", y),
        // 剩余所有情况都在这 x!=0 y!=0
        Point { x, y } => println!("On neither axis: ({}, {})", x, y),
    }

    let msg = Message::ChangeColor(Color::Hsv(0, 160, 255));

    match msg {
        Message::Quit => {
            println!("The Quit variant has no data to destructure.")
        }
        Message::Move { x, y } => {
            println!("Move in the x direction {} and in the y direction {}", x, y);
        }
        Message::Write(text) => println!("Text message: {}", text),
        Message::ChangeColor(Color::Rgb(r, g, b)) => {
            println!("Change the color to red {}, green {}, and blue {}", r, g, b)
        }
        Message::ChangeColor(Color::Hsv(r, g, b)) => {
            println!(
                "Change the color to hue {}, saturation {}, and value {}",
                r, g, b
            );
        }

        _ => (),
    }

    let triple = (0, -2, 3);

    println!("Tell me about {:?}", triple);

    match triple {
        (0, y, z) => println!("First is `0`, `y` is {:?}, and `z` is {:?}", y, z),
        // 这个也能匹配 如果放到最上面就是输出这个得内容
        (0, ..) => println!("First is `1` and the rest doesn't matter"),
        _ => println!("It doesn't matter what they are"),
    }

    let array = [1, -2, 4];

    match array {
        [0, second, third] if second < 0 => {
            println!("array[0] = 0, array[1] = {}, array[2] = {}", second, third)
        }

        [1, _, third] if third < 5 => println!(
            "array[0] = 1, array[2] = {} and array[1] was ignored if third less than 5",
            third
        ),

        [1, _, third] if third > 5 => println!(
            "array[0] = 1, array[2] = {} and array[1] was ignored if third greater than 5",
            third
        ),

        [-1, second, ..] => println!(
            "array[0] = -1, array[1] = {} and all the other ones were ignored",
            second
        ),

        [3, second, tail @ ..] => println!(
            "array[0] = 3, array[1] = {} and the other elements were {:?}",
            second, tail
        ),

        [first, middle @ .., last] => println!(
            "array[0] = {}, middle = {:?}, array[2] = {}",
            first, middle, last
        ),
    }

    let reference = &4;

    match reference {
        // If `reference` is pattern matched against `&val`, it results
        // in a comparison like:
        // `&i32`
        // `&val`
        // ^ We see that if the matching `&`s are dropped, then the `i32`
        // should be assigned to `val`.
        &val => println!("Got a value via destructuring: {:?}", val),
    }

    // To avoid the `&`, you dereference before matching.
    match *reference {
        val => println!("Got a value via dereferencing: {:?}", val),
    }

    // What if you don't start with a reference? `reference` was a `&`
    // because the right side was already a reference. This is not
    // a reference because the right side is not one.
    let _not_a_reference = 3;

    // Rust provides `ref` for exactly this purpose. It modifies the
    // assignment so that a reference is created for the element; this
    // reference is assigned.
    let ref _is_a_reference = 3;

    // Accordingly, by defining 2 values without references, references
    // can be retrieved via `ref` and `ref mut`.
    let value = 5;
    let mut mut_value = 6;

    // Use `ref` keyword to create a reference.
    // 避免move 这样后面还可以使用value啥的
    match value {
        ref r => println!("Got a reference to a value: {:?}", r),
    }

    // 5
    println!("{}", value);

    // Use `ref mut` similarly.
    match mut_value {
        ref mut m => {
            // Got a reference. Gotta dereference it before we can
            // add anything to it.
            *m += 10;
            println!("We added 10. `mut_value`: {:?}", m);
        }
    }

    // 16
    println!("{}", mut_value);

    let msg = Message2::Hello { id: 3 };

    match msg {
        Message2::Hello {
            // 可以同名 类似下面的 id:@id3..=7
            // 前面得是匹配模式得条件 后面id是变量名
            id: id_variable @ 3..=7,
        } => {
            println!("Found an id in range: {}", id_variable)
        }
        Message2::Hello { id: 10..=12 } => {
            println!("Found an id in another range ",)
        }
        Message2::Hello { id } => {
            println!("Found some other id: {}", id)
        }
    }
}

fn foo(_: i32, y: i32) {
    println!("This code only uses the y parameter: {}", y);
}
