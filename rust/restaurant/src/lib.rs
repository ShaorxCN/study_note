mod front_of_house;
pub use crate::front_of_house::hosting;

pub fn eat_at_restaurant() {
    hosting::add_to_waitlist();
    hosting::add_to_waitlist();
    hosting::add_to_waitlist();
    // add_to_waitlist();
    // crate::test::add();

    // let mut meal = bb::Breakfast::summer("Rye");
    // meal.toast = String::from("Wheat");
    // println!("i'd like {} toast please", meal.toast);
    // bb::comment();
}

// {
//     use self::serving::back_of_house;

//     pub mod hosting {
//         pub fn add_to_waitlist() {}
//         fn seat_at_table() {}

//         use super::serving::back_of_house as bb;

//     }

//     mod serving {
//         fn take_order() {}
//         fn server_order() {}

//         pub mod back_of_house {
//             pub enum Appetizer {
//                 Soup,
//                 Salad,
//             }
//             pub struct Breakfast {
//                 pub toast: String,
//                 seasonal_fruit: String,
//             }

//             impl Breakfast {
//                 pub fn summer(toast: &str) -> Breakfast {
//                     Breakfast {
//                         toast: String::from(toast),
//                         seasonal_fruit: String::from("peachses"),
//                     }
//                 }
//             }

//             fn fix_incorrect_order() {
//                 cook_order();
//                 super::server_order();
//             }

//             fn cook_order() {}
//             pub fn comment() {}
//         }
//         fn take_payment() {}
//     }
// }

// mod test {
//     pub fn add() {}
// }
