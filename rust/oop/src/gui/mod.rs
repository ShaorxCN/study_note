pub trait Draw {
    fn draw(&self);
}

pub struct Screen {
    pub components: Vec<Box<dyn Draw>>,
}

impl Screen {
    pub fn run(&self) {
        for component in self.components.iter() {
            component.draw();
        }
    }
}
// 这里得写法 那么只能接受一种实现了Draw得参数类型
// pub struct Screen<T: Draw> {
//     pub components: Vec<T>,
// }

// impl<T> Screen<T>
// where
//     T: Draw,
// {
//     pub fn run(&self) {
//         for component in self.components.iter() {
//             component.draw();
//         }
//     }
// }

pub struct Button {
    pub width: u32,
    pub height: u32,
    pub label: String,
}

impl Draw for Button {
    fn draw(&self) {
        println!("draw a button");
    }
}

impl Button {
    fn draw(&self) {
        println!("this is common impl");
    }
}

#[cfg(test)]
#[test]
fn test_impl_trait() {
    let b = Button {
        width: 10,
        height: 10,
        label: String::from("123"),
    };

    Button::draw(&b);
    b.draw();
    test_draw(b);
}

fn test_draw<T>(t: T)
where
    T: Draw,
{
    t.draw();
}

#[test]
fn test_gui() {
    let screen = Screen {
        components: vec![Box::new(Button {
            width: 50,
            height: 10,
            label: String::from("OK"),
        })],
    };

    screen.run();
}
