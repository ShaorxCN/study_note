fn print_type_of<T>(_: T) {
    println!("{}", core::any::type_name::<T>());
}

fn main() {
    println!("Hello, world!");
    let b = "basd哈";
    let c: i64 = 12;
    let f = String::from("basd哈");
    let e = &c;
    let d = &b;

    print_type_of(b);
    print_type_of(e);
    print_type_of(d);
    print_type_of(f);
}
