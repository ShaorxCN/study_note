extern crate proc_macro;
use proc_macro::TokenStream;

// attr是属性本身 这里就是route(xxx)中的xxx
// 后者就是后面的结构体或者函数啥的
#[proc_macro_attribute]
pub fn route(attr: TokenStream, item: TokenStream) -> TokenStream {
    println!("====={}=====", attr.to_string());
    println!("====={}====", item.to_string());
    item
}

#[proc_macro]
pub fn make_hello(item: TokenStream) -> TokenStream {
    let name = item.to_string();
    let hell = "Hello ".to_string() + name.as_ref();
    let fn_name =
        "fn hello_".to_string() + name.as_ref() + "(){ println!(\"" + hell.as_ref() + "\"); }";
    fn_name.parse().unwrap()
}
