use std::ops::Add;
#[derive(Debug, PartialEq)]
pub struct Millimeters(pub u32);
#[derive(Debug, PartialEq)]
pub struct Meters(pub u32);
impl Add<Meters> for Millimeters {
    type Output = Millimeters;

    fn add(self, other: Meters) -> Millimeters {
        Millimeters(self.0 + (other.0 * 1000))
    }
}

pub trait HelloMacro {
    fn hello_macro();
}
