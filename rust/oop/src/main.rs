use oop::{blog::*, gui::*};
struct SelectBox {
    width: u32,
    height: u32,
    options: Vec<String>,
}

impl Draw for SelectBox {
    fn draw(&self) {
        println!("draw selecBox");
    }
}

fn main() {
    let mut a = oop::AveragedCollection::new();
    a.add(1);
    a.add(3);
    a.add(10);
    a.add(14);

    println!("{}", a.average());

    println!("{:?}", a.remove());
    println!("{}", a.average());
    // private field 不可以访问
    // println!("{:?}", a.list);

    test_gui();

    test_blog();
}

fn test_gui() {
    let screen = Screen {
        components: vec![
            Box::new(SelectBox {
                width: 75,
                height: 10,
                options: vec![
                    String::from("Yes"),
                    String::from("Maybe"),
                    String::from("No"),
                ],
            }),
            Box::new(Button {
                width: 50,
                height: 10,
                label: String::from("OK"),
            }),
        ],
    };

    screen.run();
}

fn test_blog() {
    let mut blog = Post::new();

    blog.add_text("I ate a salad for lunch today");
    assert_eq!("", blog.content());
    blog.request_review();
    assert_eq!("", blog.content());
    blog.reject();
    assert_eq!("", blog.content());
    blog.approve();
    assert_eq!("", blog.content());
    blog.request_review();
    assert_eq!("", blog.content());
    blog.approve();
    blog.reject();
    assert_eq!("I ate a salad for lunch today", blog.content());
}
